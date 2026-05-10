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
        self.last_in = None
        self.last_out = None
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
            return
        self.query_cmd = self.mcu.lookup_query_command(
            "get_usbcdc_debug reset=%c",
            "usbcdc_debug_dispatch max_dispatch_us=%u max_total_us=%u"
            " max_rpos=%u max_pop=%u error=%u max_error_pop=%u")
        self._register_response(self.handle_debug_in, "usbcdc_debug_in")
        self._register_response(self.handle_debug_out, "usbcdc_debug_out")
    def _register_response(self, cb, msg):
        if hasattr(self.mcu, 'register_response'):
            self.mcu.register_response(cb, msg)
        else:
            self.mcu._serial.register_response(cb, msg)
    def handle_debug_in(self, params):
        self.last_in = params
    def handle_debug_out(self, params):
        self.last_out = params
    cmd_QUERY_USBCDC_DEBUG_help = "Query MCU USB-CDC debug counters"
    def cmd_QUERY_USBCDC_DEBUG(self, gcmd):
        if self.query_cmd is None:
            gcmd.respond_info("USB-CDC debug command is not available")
            return
        reset = gcmd.get_int('RESET', 0, minval=0, maxval=1)
        self.last_in = self.last_out = None
        dispatch = self.query_cmd.send([reset])
        lines = ["USB_CDC_DEBUG_BUILD=%s" % (self.build,)]
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
                dispatch['max_dispatch_us'], dispatch['max_total_us'],
                dispatch['max_rpos'], dispatch['max_pop'],
                dispatch['error'], dispatch['max_error_pop']))
        if reset:
            lines.append("counters reset after snapshot")
        gcmd.respond_info("\n".join(lines))


def load_config(config):
    return UsbCdcDebug(config)
