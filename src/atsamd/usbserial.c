// Hardware interface to USB on samd
//
// Copyright (C) 2018-2019  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <string.h> // memcpy
#include "autoconf.h" // CONFIG_MACH_SAMD21
#include "board/armcm_boot.h" // armcm_enable_irq
#include "board/io.h" // writeb
#include "board/irq.h" // irq_save
#include "board/misc.h" // timer_read_time
#include "board/usb_cdc.h" // usb_notify_ep0
#include "board/usb_cdc_ep.h" // USB_CDC_EP_BULK_IN
#include "command.h" // DECL_CONSTANT_STR
#include "compiler.h" // DIV_ROUND_CLOSEST
#include "internal.h" // enable_pclock
#include "sched.h" // DECL_INIT


/****************************************************************
 * USB transfer memory
 ****************************************************************/

static uint8_t __aligned(4) ep0out[USB_CDC_EP0_SIZE];
static uint8_t __aligned(4) ep0in[USB_CDC_EP0_SIZE];
static uint8_t __aligned(4) acmin[USB_CDC_EP_ACM_SIZE];
static uint8_t __aligned(4) bulkout[USB_CDC_EP_BULK_OUT_SIZE];
static uint8_t __aligned(4) bulkin[USB_CDC_EP_BULK_IN_SIZE];

// Convert 64, 32, 16, 8 sized buffer to 3, 2, 1, 0 for PCKSIZE.SIZE register
#define BSIZE(bufname) (__builtin_ctz(sizeof(bufname)) - 3)

static UsbDeviceDescriptor usb_desc[] = {
    [0] = { {
        {
            .ADDR.reg = (uint32_t)ep0out,
            .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(ep0out)),
        }, {
            .ADDR.reg = (uint32_t)ep0in,
            .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(ep0in)),
        },
    } },
    [USB_CDC_EP_ACM] = { {
        {
        }, {
            .ADDR.reg = (uint32_t)acmin,
            .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(acmin)),
        },
    } },
    [USB_CDC_EP_BULK_OUT] = { {
        {
            .ADDR.reg = (uint32_t)bulkout,
            .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(bulkout)),
        }, {
        },
    } },
    [USB_CDC_EP_BULK_IN] = { {
        {
        }, {
            .ADDR.reg = (uint32_t)bulkin,
            .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(bulkin)),
        },
    } },
};

#define EP0 USB->DEVICE.DeviceEndpoint[0]
#define EP_ACM USB->DEVICE.DeviceEndpoint[USB_CDC_EP_ACM]
#define EP_BULKOUT USB->DEVICE.DeviceEndpoint[USB_CDC_EP_BULK_OUT]
#define EP_BULKIN USB->DEVICE.DeviceEndpoint[USB_CDC_EP_BULK_IN]

static volatile uint32_t bulk_in_busy_returns, bulk_in_armed;
static volatile uint32_t bulk_in_handler_count, trcpt1_count;
static volatile uint32_t max_bk1rdy_to_trcpt1_us;
static volatile uint32_t trfail1_seen_count, trfail1_seen_busy_count;
static volatile uint32_t trfail1_while_armed;
static volatile uint32_t bulk_in_arm_time;
static volatile uint8_t bulk_in_armed_active, bulk_in_trfail1_seen_while_armed;
static volatile uint8_t max_bk1rdy_to_trcpt1_had_trfail1;
static volatile uint32_t bulk_out_stall0_count;
static volatile uint32_t bulk_out_stall1_count;
static volatile uint32_t bulk_out_stall0_first_pcksize;
static volatile uint8_t bulk_out_stall0_first_valid;
static volatile uint8_t bulk_out_stall0_first_epstatus;
static volatile uint8_t bulk_out_stall0_first_epintflag;
static volatile uint8_t bulk_out_stall0_first_epcfg;

DECL_CONSTANT_STR("USB_CDC_DEBUG_BUILD", "atsamd-epraw-20260510");

static void
bulk_in_note_armed(void)
{
    bulk_in_arm_time = timer_read_time();
    bulk_in_armed_active = 1;
    bulk_in_trfail1_seen_while_armed = 0;
    bulk_in_armed++;
}

static void
bulk_in_sample_trfail1(uint8_t epintflag, uint8_t epstatus, uint8_t busy_sample)
{
    if (!(epintflag & USB_DEVICE_EPINTFLAG_TRFAIL1))
        return;
    if (busy_sample) {
        trfail1_seen_busy_count++;
    } else {
        trfail1_seen_count++;
        if (epstatus & USB_DEVICE_EPSTATUS_BK1RDY)
            trfail1_while_armed++;
    }
    if (bulk_in_armed_active && (epstatus & USB_DEVICE_EPSTATUS_BK1RDY))
        bulk_in_trfail1_seen_while_armed = 1;
    EP_BULKIN.EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRFAIL1;
}

static void
bulk_in_note_trcpt1(uint8_t epstatus)
{
    trcpt1_count++;
    if (epstatus & USB_DEVICE_EPSTATUS_BK1RDY)
        return;
    if (!bulk_in_armed_active)
        return;
    uint32_t delta = timer_read_time() - bulk_in_arm_time;
    uint32_t us = DIV_ROUND_CLOSEST(delta, CONFIG_CLOCK_FREQ / 1000000);
    if (us > max_bk1rdy_to_trcpt1_us) {
        max_bk1rdy_to_trcpt1_us = us;
        max_bk1rdy_to_trcpt1_had_trfail1 =
            bulk_in_trfail1_seen_while_armed;
    }
    bulk_in_armed_active = 0;
    bulk_in_trfail1_seen_while_armed = 0;
}

static void
bulk_out_note_stall(uint8_t epintflag)
{
    if (epintflag & USB_DEVICE_EPINTFLAG_STALL0)
        bulk_out_stall0_count++;
    if (epintflag & USB_DEVICE_EPINTFLAG_STALL1)
        bulk_out_stall1_count++;
    if (bulk_out_stall0_first_valid)
        return;
    bulk_out_stall0_first_valid = 1;
    bulk_out_stall0_first_epstatus = EP_BULKOUT.EPSTATUS.reg;
    bulk_out_stall0_first_epintflag = epintflag;
    bulk_out_stall0_first_epcfg = EP_BULKOUT.EPCFG.reg;
    bulk_out_stall0_first_pcksize =
        usb_desc[USB_CDC_EP_BULK_OUT].DeviceDescBank[0].PCKSIZE.reg;
}

struct usb_ep_debug {
    uint8_t epcfg, epintenset, epintflag, epstatus;
    uint32_t pck0, pck1;
};

static void
usb_get_ep_debug(uint_fast8_t ep, struct usb_ep_debug *d)
{
    UsbDeviceEndpoint *ude = &USB->DEVICE.DeviceEndpoint[ep];
    d->epcfg = ude->EPCFG.reg;
    d->epintenset = ude->EPINTENSET.reg;
    d->epintflag = ude->EPINTFLAG.reg;
    d->epstatus = ude->EPSTATUS.reg;
    d->pck0 = usb_desc[ep].DeviceDescBank[0].PCKSIZE.reg;
    d->pck1 = usb_desc[ep].DeviceDescBank[1].PCKSIZE.reg;
}

void
command_get_usbcdc_debug(uint32_t *args)
{
    irqstatus_t flag = irq_save();
    uint32_t busy = bulk_in_busy_returns, armed = bulk_in_armed;
    uint32_t handler = bulk_in_handler_count, trcpt1 = trcpt1_count;
    uint32_t max_us = max_bk1rdy_to_trcpt1_us;
    uint32_t trfail1 = trfail1_seen_count;
    uint32_t trfail1_busy = trfail1_seen_busy_count;
    uint32_t trfail1_armed = trfail1_while_armed;
    uint8_t active = bulk_in_armed_active;
    uint8_t cur_trfail1 = bulk_in_trfail1_seen_while_armed;
    uint8_t max_trfail1 = max_bk1rdy_to_trcpt1_had_trfail1;
    uint8_t epstatus = EP_BULKIN.EPSTATUS.reg;
    uint8_t epintflag = EP_BULKIN.EPINTFLAG.reg;
    uint32_t pcksize =
        usb_desc[USB_CDC_EP_BULK_IN].DeviceDescBank[1].PCKSIZE.reg;
    uint32_t out_stall0 = bulk_out_stall0_count;
    uint32_t out_stall1 = bulk_out_stall1_count;
    uint32_t out_stall0_first_pcksize = bulk_out_stall0_first_pcksize;
    uint8_t out_stall0_valid = bulk_out_stall0_first_valid;
    uint8_t out_stall0_epstatus = bulk_out_stall0_first_epstatus;
    uint8_t out_stall0_epintflag = bulk_out_stall0_first_epintflag;
    uint8_t out_stall0_epcfg = bulk_out_stall0_first_epcfg;
    struct usb_ep_debug epout, epin;
    usb_get_ep_debug(USB_CDC_EP_BULK_OUT, &epout);
    usb_get_ep_debug(USB_CDC_EP_BULK_IN, &epin);
    if (args[0]) {
        bulk_in_busy_returns = 0;
        bulk_in_armed = 0;
        bulk_in_handler_count = 0;
        trcpt1_count = 0;
        max_bk1rdy_to_trcpt1_us = 0;
        trfail1_seen_count = 0;
        trfail1_seen_busy_count = 0;
        trfail1_while_armed = 0;
        bulk_in_trfail1_seen_while_armed = 0;
        max_bk1rdy_to_trcpt1_had_trfail1 = 0;
        bulk_out_stall0_count = 0;
        bulk_out_stall1_count = 0;
        bulk_out_stall0_first_valid = 0;
        bulk_out_stall0_first_epstatus = 0;
        bulk_out_stall0_first_epintflag = 0;
        bulk_out_stall0_first_epcfg = 0;
        bulk_out_stall0_first_pcksize = 0;
    }
    irq_restore(flag);
    sendf("usbcdc_debug_outstall stall0_count=%u stall1_count=%u first_valid=%c"
          " first_epstatus=%u first_epintflag=%u first_epcfg=%u"
          " first_pcksize=%u",
          out_stall0, out_stall1, out_stall0_valid, out_stall0_epstatus,
          out_stall0_epintflag, out_stall0_epcfg, out_stall0_first_pcksize);
    sendf("usbcdc_debug_epout epcfg=%u epintenset=%u epintflag=%u"
          " epstatus=%u pck0=%u pck1=%u",
          epout.epcfg, epout.epintenset, epout.epintflag, epout.epstatus,
          epout.pck0, epout.pck1);
    sendf("usbcdc_debug_epin epcfg=%u epintenset=%u epintflag=%u"
          " epstatus=%u pck0=%u pck1=%u",
          epin.epcfg, epin.epintenset, epin.epintflag, epin.epstatus,
          epin.pck0, epin.pck1);
    sendf("usbcdc_debug_atsamd bulk_in_busy_returns=%u bulk_in_armed=%u"
          " bulk_in_handler_count=%u trcpt1_count=%u"
          " max_bk1rdy_to_trcpt1_us=%u"
          " max_bk1rdy_to_trcpt1_had_trfail1=%c"
          " trfail1_seen_count=%u trfail1_seen_busy_count=%u"
          " trfail1_while_armed=%u cur_trfail1=%c active=%c"
          " epstatus=%u epintflag=%u pcksize=%u",
          busy, armed, handler, trcpt1, max_us, max_trfail1, trfail1,
          trfail1_busy, trfail1_armed, cur_trfail1, active, epstatus,
          epintflag, pcksize);
}
DECL_COMMAND(command_get_usbcdc_debug, "get_usbcdc_debug reset=%c");

static int_fast8_t
usb_write_packet(uint32_t ep, uint32_t bank, const void *data, uint_fast8_t len)
{
    // Check if there is room for this packet
    UsbDeviceEndpoint *ude = &USB->DEVICE.DeviceEndpoint[ep];
    uint8_t sts = ude->EPSTATUS.reg;
    uint8_t bkrdy = (bank ? USB_DEVICE_EPSTATUS_BK1RDY
                     : USB_DEVICE_EPSTATUS_BK0RDY);
    if (sts & bkrdy) {
        if (ep == USB_CDC_EP_BULK_IN && bank == 1) {
            irqstatus_t flag = irq_save();
            uint8_t epintflag = EP_BULKIN.EPINTFLAG.reg;
            uint8_t epstatus = EP_BULKIN.EPSTATUS.reg;
            bulk_in_busy_returns++;
            bulk_in_sample_trfail1(epintflag, epstatus, 1);
            irq_restore(flag);
        }
        return -1;
    }
    // Copy the packet to the given buffer
    UsbDeviceDescBank *uddb = &usb_desc[ep].DeviceDescBank[bank];
    memcpy((void*)uddb->ADDR.reg, data, len);
    // Inform the USB hardware of the available packet
    uint32_t pcksize = uddb->PCKSIZE.reg;
    uint32_t c = pcksize & ~USB_DEVICE_PCKSIZE_BYTE_COUNT_Msk;
    uddb->PCKSIZE.reg = c | USB_DEVICE_PCKSIZE_BYTE_COUNT(len);
    if (ep == USB_CDC_EP_BULK_IN && bank == 1) {
        irqstatus_t flag = irq_save();
        bulk_in_note_armed();
        ude->EPSTATUSSET.reg = bkrdy;
        irq_restore(flag);
    } else {
        ude->EPSTATUSSET.reg = bkrdy;
    }
    return len;
}

static int_fast8_t
usb_read_packet(uint32_t ep, uint32_t bank, void *data, uint_fast8_t max_len)
{
    // Check if there is a packet ready
    UsbDeviceEndpoint *ude = &USB->DEVICE.DeviceEndpoint[ep];
    uint8_t sts = ude->EPSTATUS.reg;
    uint8_t bkrdy = (bank ? USB_DEVICE_EPSTATUS_BK1RDY
                     : USB_DEVICE_EPSTATUS_BK0RDY);
    if (!(sts & bkrdy))
        return -1;
    // Copy the packet to the given buffer
    UsbDeviceDescBank *uddb = &usb_desc[ep].DeviceDescBank[bank];
    uint32_t pcksize = uddb->PCKSIZE.reg;
    uint32_t c = pcksize & USB_DEVICE_PCKSIZE_BYTE_COUNT_Msk;
    if (c > max_len)
        c = max_len;
    memcpy(data, (void*)uddb->ADDR.reg, c);
    // Notify the USB hardware that the space is now available
    ude->EPSTATUSCLR.reg = bkrdy;
    return c;
}


/****************************************************************
 * Interface
 ****************************************************************/

int_fast8_t
usb_read_bulk_out(void *data, uint_fast8_t max_len)
{
    return usb_read_packet(USB_CDC_EP_BULK_OUT, 0, data, max_len);
}

int_fast8_t
usb_send_bulk_in(void *data, uint_fast8_t len)
{
    return usb_write_packet(USB_CDC_EP_BULK_IN, 1, data, len);
}

int_fast8_t
usb_read_ep0(void *data, uint_fast8_t max_len)
{
    return usb_read_packet(0, 0, data, max_len);
}

int_fast8_t
usb_read_ep0_setup(void *data, uint_fast8_t max_len)
{
    return usb_read_ep0(data, max_len);
}

int_fast8_t
usb_send_ep0(const void *data, uint_fast8_t len)
{
    return usb_write_packet(0, 1, data, len);
}

void
usb_stall_ep0(void)
{
    EP0.EPSTATUSSET.reg = USB_DEVICE_EPSTATUS_STALLRQ(3);
}

static uint8_t set_address;

void
usb_set_address(uint_fast8_t addr)
{
    writeb(&set_address, addr | USB_DEVICE_DADD_ADDEN);
    usb_send_ep0(NULL, 0);
}

void
usb_set_configure(void)
{
    EP_ACM.EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE1(4);

    EP_BULKOUT.EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE0(3);
    EP_BULKOUT.EPINTENSET.reg = (
        USB_DEVICE_EPINTENSET_TRCPT0 | USB_DEVICE_EPINTENSET_TRCPT1
        | USB_DEVICE_EPINTENSET_STALL0 | USB_DEVICE_EPINTENSET_STALL1);

    EP_BULKIN.EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE1(3);
    EP_BULKIN.EPINTENSET.reg = (
        USB_DEVICE_EPINTENSET_TRCPT0 | USB_DEVICE_EPINTENSET_TRCPT1);
}


/****************************************************************
 * Setup and interrupts
 ****************************************************************/

void
USB_Handler(void)
{
    uint8_t s = USB->DEVICE.INTFLAG.reg;
    if (s & USB_DEVICE_INTFLAG_EORST) {
        USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_EORST;
        // Enable endpoint 0 irqs
        EP0.EPINTENSET.reg = (
            USB_DEVICE_EPINTENSET_TRCPT0 | USB_DEVICE_EPINTENSET_TRCPT1
            | USB_DEVICE_EPINTENSET_RXSTP);
    }

    uint16_t ep = USB->DEVICE.EPINTSMRY.reg;
    if (ep & (1<<0)) {
        uint8_t sts = EP0.EPINTFLAG.reg;
        EP0.EPINTFLAG.reg = sts;
        if (set_address && sts & USB_DEVICE_EPINTFLAG_TRCPT1) {
            // Apply address after last "in" message transmitted
            USB->DEVICE.DADD.reg = set_address;
            set_address = 0;
        }
        usb_notify_ep0();
    }
    if (ep & (1<<USB_CDC_EP_BULK_OUT)) {
        uint8_t sts = EP_BULKOUT.EPINTFLAG.reg;
        if (sts & (USB_DEVICE_EPINTFLAG_STALL0
                   | USB_DEVICE_EPINTFLAG_STALL1))
            bulk_out_note_stall(sts);
        EP_BULKOUT.EPINTFLAG.reg = sts;
        if (sts & (USB_DEVICE_EPINTFLAG_TRCPT0
                   | USB_DEVICE_EPINTFLAG_TRCPT1))
            usb_notify_bulk_out();
    }
    if (ep & (1<<USB_CDC_EP_BULK_IN)) {
        uint8_t sts = EP_BULKIN.EPINTFLAG.reg;
        uint8_t epstatus = EP_BULKIN.EPSTATUS.reg;
        bulk_in_handler_count++;
        bulk_in_sample_trfail1(sts, epstatus, 0);
        EP_BULKIN.EPINTFLAG.reg = sts & ~USB_DEVICE_EPINTFLAG_TRFAIL1;
        if (sts & USB_DEVICE_EPINTFLAG_TRCPT1)
            bulk_in_note_trcpt1(epstatus);
        usb_notify_bulk_in();
    }
}

DECL_CONSTANT_STR("RESERVE_PINS_USB", "PA24,PA25");

void
usbserial_init(void)
{
    // configure usb clock
    enable_pclock(USB_GCLK_ID, ID_USB);
    // configure USBD+ and USBD- pins
    uint32_t ptype = CONFIG_MACH_SAMD21 ? 'G' : 'H';
    gpio_peripheral(GPIO('A', 24), ptype, 0);
    gpio_peripheral(GPIO('A', 25), ptype, 0);
    uint32_t trim = GET_FUSE(USB_FUSES_TRIM);
    uint32_t transp = GET_FUSE(USB_FUSES_TRANSP);
    uint32_t transn = GET_FUSE(USB_FUSES_TRANSN);
    USB->DEVICE.PADCAL.reg = (USB_PADCAL_TRIM(trim) | USB_PADCAL_TRANSP(transp)
                              | USB_PADCAL_TRANSN(transn));
    // Enable USB in device mode
    USB->DEVICE.CTRLA.reg = USB_CTRLA_ENABLE;
    USB->DEVICE.DESCADD.reg = (uint32_t)usb_desc;
    EP0.EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE0(1) | USB_DEVICE_EPCFG_EPTYPE1(1);
    EP_ACM.EPCFG.reg = USB_DEVICE_EPCFG_EPTYPE1(4);
    USB->DEVICE.CTRLB.reg = 0;
    // enable irqs
    USB->DEVICE.INTENSET.reg = USB_DEVICE_INTENSET_EORST;
#if CONFIG_MACH_SAMD21
    armcm_enable_irq(USB_Handler, USB_IRQn, 1);
#elif CONFIG_MACH_SAMX5
    armcm_enable_irq(USB_Handler, USB_0_IRQn, 1);
    armcm_enable_irq(USB_Handler, USB_1_IRQn, 1);
    armcm_enable_irq(USB_Handler, USB_2_IRQn, 1);
    armcm_enable_irq(USB_Handler, USB_3_IRQn, 1);
#endif
}
DECL_INIT(usbserial_init);
