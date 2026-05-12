# On-demand USB-CDC debug query helper
#
# Copyright (C) 2026
#
# This file may be distributed under the terms of the GNU GPLv3 license.


class UsbCdcDebug:
    def __init__(self, config):
        self.printer = config.get_printer()
        self.mcu_name = config.get('mcu', 'mcu')
        self.mcu = None
        self.query_cmd = None
        self.query_type = None
        self.last_in = None
        self.last_out = None
        self.last_outstall = None
        self.last_epout = None
        self.last_epin = None
        self.build = None
        self.printer.register_event_handler("klippy:connect",
                                            self.handle_connect)
        gcode = self.printer.lookup_object('gcode')
        gcode.register_command("QUERY_USBCDC_DEBUG",
                               self.cmd_QUERY_USBCDC_DEBUG,
                               desc=self.cmd_QUERY_USBCDC_DEBUG_help)
    def handle_connect(self):
        mcu_name = self.mcu_name
        if mcu_name != 'mcu' and not mcu_name.startswith('mcu '):
            mcu_name = 'mcu ' + mcu_name
        self.mcu = self.printer.lookup_object(mcu_name)
        constants = self.mcu.get_constants()
        self.build = constants.get('USB_CDC_DEBUG_BUILD')
        if self.mcu.try_lookup_command("get_usbcdc_debug reset=%c") is None:
            self.query_cmd = None
            self.query_type = None
            return
        self.query_cmd = self._try_lookup_query(
            "usbcdc_debug_atsamd bulk_in_busy_returns=%u bulk_in_armed=%u"
            " bulk_in_handler_count=%u trcpt1_count=%u"
            " max_bk1rdy_to_trcpt1_us=%u"
            " max_bk1rdy_to_trcpt1_had_trfail1=%c"
            " trfail1_seen_count=%u trfail1_seen_busy_count=%u"
            " trfail1_while_armed=%u cur_trfail1=%c active=%c"
            " epstatus=%u epintflag=%u pcksize=%u")
        if self.query_cmd is not None:
            self.query_type = "atsamd"
            self._register_response(self.handle_debug_outstall,
                                    "usbcdc_debug_outstall")
            self._register_response(self.handle_debug_epout,
                                    "usbcdc_debug_epout")
            self._register_response(self.handle_debug_epin,
                                    "usbcdc_debug_epin")
            return
        self.query_cmd = self._try_lookup_query(
            "usbcdc_debug_dispatch max_dispatch_us=%u max_total_us=%u"
            " max_rpos=%u max_pop=%u error=%u max_error_pop=%u")
        if self.query_cmd is not None:
            self.query_type = "generic"
            self._register_response(self.handle_debug_in, "usbcdc_debug_in")
            self._register_response(self.handle_debug_out, "usbcdc_debug_out")
            return
        self.query_type = None
    def _try_lookup_query(self, respformat):
        try:
            return self.mcu.lookup_query_command(
                "get_usbcdc_debug reset=%c", respformat)
        except Exception:
            return None
    def _register_response(self, cb, msg):
        if hasattr(self.mcu, 'register_response'):
            self.mcu.register_response(cb, msg)
        else:
            self.mcu._serial.register_response(cb, msg)
    def handle_debug_in(self, params):
        self.last_in = params
    def handle_debug_out(self, params):
        self.last_out = params
    def handle_debug_outstall(self, params):
        self.last_outstall = params
    def handle_debug_epout(self, params):
        self.last_epout = params
    def handle_debug_epin(self, params):
        self.last_epin = params
    cmd_QUERY_USBCDC_DEBUG_help = "Query MCU USB-CDC debug counters"
    def cmd_QUERY_USBCDC_DEBUG(self, gcmd):
        if self.query_cmd is None:
            gcmd.respond_info("USB-CDC debug command is not available")
            return
        reset = gcmd.get_int('RESET', 0, minval=0, maxval=1)
        self.last_in = self.last_out = self.last_outstall = None
        self.last_epout = self.last_epin = None
        result = self.query_cmd.send([reset])
        lines = ["USB_CDC_DEBUG_BUILD=%s" % (self.build,)]
        if self.query_type == "atsamd":
            if self.last_outstall is not None:
                p = self.last_outstall
                lines.append(
                    "outstall stall0_count=%d stall1_count=%d first_valid=%d"
                    " first_epstatus=%d first_epintflag=%d first_epcfg=%d"
                    " first_pcksize=%d" % (
                        p['stall0_count'], p['stall1_count'],
                        p['first_valid'],
                        p['first_epstatus'], p['first_epintflag'],
                        p['first_epcfg'], p['first_pcksize']))
            if self.last_epout is not None:
                p = self.last_epout
                lines.append(
                    "epout epcfg=%d epintenset=%d epintflag=%d"
                    " epstatus=%d pck0=%d pck1=%d" % (
                        p['epcfg'], p['epintenset'], p['epintflag'],
                        p['epstatus'], p['pck0'], p['pck1']))
            if self.last_epin is not None:
                p = self.last_epin
                lines.append(
                    "epin epcfg=%d epintenset=%d epintflag=%d"
                    " epstatus=%d pck0=%d pck1=%d" % (
                        p['epcfg'], p['epintenset'], p['epintflag'],
                        p['epstatus'], p['pck0'], p['pck1']))
            p = result
            lines.append(
                "atsamd_usb bulk_in_busy_returns=%d bulk_in_armed=%d"
                " bulk_in_handler_count=%d trcpt1_count=%d"
                " max_bk1rdy_to_trcpt1_us=%d"
                " max_bk1rdy_to_trcpt1_had_trfail1=%d"
                " trfail1_seen_count=%d trfail1_seen_busy_count=%d"
                " trfail1_while_armed=%d cur_trfail1=%d active=%d"
                " epstatus=%d epintflag=%d pcksize=%d" % (
                    p['bulk_in_busy_returns'], p['bulk_in_armed'],
                    p['bulk_in_handler_count'], p['trcpt1_count'],
                    p['max_bk1rdy_to_trcpt1_us'],
                    p['max_bk1rdy_to_trcpt1_had_trfail1'],
                    p['trfail1_seen_count'], p['trfail1_seen_busy_count'],
                    p['trfail1_while_armed'], p['cur_trfail1'],
                    p['active'], p['epstatus'], p['epintflag'],
                    p['pcksize']))
            if reset:
                lines.append("counters reset after snapshot")
            gcmd.respond_info("\n".join(lines))
            return
        if self.last_in is not None:
            p = self.last_in
            lines.append(
                "in notify=%d wake=%d task=%d send=%d busy=%d bytes=%d"
                " max_wake_us=%d max_send_us=%d max_queued=%d"
                " max_busy_us=%d" % (
                    p['notify'], p['wake'], p['task'], p['send'], p['busy'],
                    p['bytes'], p['max_wake_us'], p['max_send_us'],
                    p['max_queued'], p['max_busy_us']))
        if self.last_out is not None:
            p = self.last_out
            lines.append(
                "out notify=%d wake=%d task=%d read=%d empty=%d bytes=%d"
                " max_wake_us=%d max_read_us=%d" % (
                    p['notify'], p['wake'], p['task'], p['read'], p['empty'],
                    p['bytes'], p['max_wake_us'], p['max_read_us']))
        lines.append(
            "dispatch max_dispatch_us=%d max_total_us=%d max_rpos=%d"
            " max_pop=%d error=%d max_error_pop=%d" % (
                result['max_dispatch_us'], result['max_total_us'],
                result['max_rpos'], result['max_pop'],
                result['error'], result['max_error_pop']))
        if reset:
            lines.append("counters reset after snapshot")
        gcmd.respond_info("\n".join(lines))


def load_config(config):
    return UsbCdcDebug(config)
