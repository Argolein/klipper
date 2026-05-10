#!/usr/bin/env python3
# Monitor Klipper retransmits and request USB-CDC debug snapshots.
#
# Copyright (C) 2026
#
# This file may be distributed under the terms of the GNU GPLv3 license.

import argparse
import errno
import json
import os
import re
import select
import socket
import sys
import time


STATS_RE = re.compile(r"^Stats ")


def expand_socket_paths(paths):
    expanded = []
    for path in paths:
        path = os.path.expanduser(path)
        if path not in expanded:
            expanded.append(path)
    return expanded


def connect_webhook(paths, timeout):
    deadline = None if timeout is None else time.monotonic() + timeout
    while True:
        last_error = None
        for path in paths:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                sock.connect(path)
            except OSError as e:
                last_error = e
                sock.close()
                if e.errno in (errno.ENOENT, errno.ECONNREFUSED):
                    continue
                raise
            sock.setblocking(False)
            print("connected to Klippy webhook socket %s" % (path,), flush=True)
            return sock
        if deadline is not None and time.monotonic() >= deadline:
            paths_s = ", ".join(paths)
            raise RuntimeError(
                "Unable to connect to any Klippy webhook socket: %s"
                " (last error: %s)" % (paths_s, last_error)
            )
        time.sleep(0.25)


class WebhookClient:
    def __init__(self, paths, timeout):
        self.sock = connect_webhook(paths, timeout)
        self.next_id = 1
        self.partial = b""

    def send_request(self, method, params=None):
        req = {
            "id": self.next_id,
            "method": method,
            "params": params or {},
        }
        self.next_id += 1
        data = json.dumps(req, separators=(",", ":")).encode() + b"\x03"
        self.sock.setblocking(True)
        try:
            self.sock.sendall(data)
        finally:
            self.sock.setblocking(False)
        return req["id"]

    def subscribe_output(self):
        self.send_request(
            "gcode/subscribe_output",
            {"response_template": {"key": "gcode_response"}},
        )

    def run_gcode(self, script):
        return self.send_request("gcode/script", {"script": script})

    def process_socket(self):
        try:
            data = self.sock.recv(4096)
        except BlockingIOError:
            return []
        if not data:
            raise RuntimeError("Klippy webhook socket closed")
        parts = data.split(b"\x03")
        parts[0] = self.partial + parts[0]
        self.partial = parts.pop()
        messages = []
        for part in parts:
            if not part:
                continue
            messages.append(json.loads(part.decode()))
        return messages


class LogTail:
    def __init__(self, path, from_start):
        self.path = path
        self.fp = open(path, "r", encoding="utf-8", errors="replace")
        if not from_start:
            self.fp.seek(0, os.SEEK_END)

    def fileno(self):
        return self.fp.fileno()

    def read_available(self):
        while True:
            line = self.fp.readline()
            if not line:
                break
            yield line.rstrip("\n")


class RetransmitMonitor:
    def __init__(self, args):
        self.args = args
        self.mcu_re = re.compile(
            r"\b%s:\s.*?\bbytes_retransmit=(\d+)"
            % (re.escape(args.mcu),)
        )
        self.last_rt = None
        self.last_trigger_time = 0.0
        self.trigger_count = 0

    def handle_line(self, line, webhook):
        if not STATS_RE.match(line):
            return False
        match = self.mcu_re.search(line)
        if match is None:
            return False
        mcu_rt = int(match.group(1))
        if self.last_rt is None:
            self.last_rt = mcu_rt
            return False
        if mcu_rt == self.last_rt:
            return False

        old_rt = self.last_rt
        self.last_rt = mcu_rt
        now = time.monotonic()
        if now - self.last_trigger_time < self.args.min_interval:
            print(
                "%s mcu_rt changed %d -> %d; skipped due to min interval"
                % (time.strftime("%H:%M:%S"), old_rt, mcu_rt),
                flush=True,
            )
            return False

        self.last_trigger_time = now
        self.trigger_count += 1
        print(
            "%s mcu_rt changed %d -> %d; sending %s"
            % (time.strftime("%H:%M:%S"), old_rt, mcu_rt, self.args.command),
            flush=True,
        )
        webhook.run_gcode(self.args.command)
        if self.args.once:
            return True
        if self.args.max_triggers and self.trigger_count >= self.args.max_triggers:
            return True
        return False


def is_usbcdc_debug_response(response):
    return (
        "USB_CDC_DEBUG_BUILD=" in response
        or "max_busy_us=" in response
        or "max_error_pop=" in response
        or "USB-CDC debug command" in response
    )


def print_webhook_messages(messages, show_all_output=False):
    for msg in messages:
        params = msg.get("params")
        if isinstance(params, dict) and "response" in params:
            response = params["response"]
            if show_all_output or is_usbcdc_debug_response(response):
                print(response, flush=True)
        elif "error" in msg:
            print("webhook error: %s" % (msg["error"],), file=sys.stderr,
                  flush=True)


def main():
    parser = argparse.ArgumentParser(
        description="Monitor mcu bytes_retransmit and query USB-CDC debug data."
    )
    parser.add_argument(
        "--log", default=os.path.expanduser("~/printer_data/logs/klippy.log"),
        help="klippy.log path (default: %(default)s)",
    )
    parser.add_argument(
        "--socket", action="append",
        help=("Klippy webhooks Unix socket. Can be given multiple times."
              " Defaults to ~/printer_data/comms/klippy.sock and"
              " /tmp/klippy_uds."),
    )
    parser.add_argument(
        "--mcu", default="mcu",
        help="MCU stats object to monitor (default: %(default)s)",
    )
    parser.add_argument(
        "--command", default="QUERY_USBCDC_DEBUG RESET=0",
        help="G-Code command to run on retransmit change (default: %(default)s)",
    )
    parser.add_argument(
        "--min-interval", type=float, default=5.0,
        help="minimum seconds between automatic queries (default: %(default)s)",
    )
    parser.add_argument(
        "--once", action="store_true",
        help="exit after the first automatic query",
    )
    parser.add_argument(
        "--max-triggers", type=int, default=0,
        help="exit after this many queries; 0 means unlimited (default: 0)",
    )
    parser.add_argument(
        "--from-start", action="store_true",
        help="read the log from the beginning instead of tailing new lines",
    )
    parser.add_argument(
        "--show-all-output", action="store_true",
        help="print all subscribed G-Code output, not just USB-CDC debug output",
    )
    parser.add_argument(
        "--connect-timeout", type=float, default=30.0,
        help="seconds to wait for Klippy socket connection (default: %(default)s)",
    )
    args = parser.parse_args()

    socket_paths = expand_socket_paths(args.socket or [
        "~/printer_data/comms/klippy.sock",
        "/tmp/klippy_uds",
    ])
    tail = LogTail(args.log, args.from_start)
    webhook = WebhookClient(socket_paths, args.connect_timeout)
    webhook.subscribe_output()
    monitor = RetransmitMonitor(args)

    print(
        "monitoring %s for %s: bytes_retransmit changes; command=%r"
        % (args.log, args.mcu, args.command),
        flush=True,
    )

    poller = select.poll()
    poller.register(webhook.sock.fileno(), select.POLLIN | select.POLLHUP)

    while True:
        for fd, event in poller.poll(250):
            if fd == webhook.sock.fileno():
                if event & select.POLLHUP:
                    raise RuntimeError("Klippy webhook socket closed")
                print_webhook_messages(
                    webhook.process_socket(), args.show_all_output)
        for line in tail.read_available():
            if monitor.handle_line(line, webhook):
                print_webhook_messages(
                    webhook.process_socket(), args.show_all_output)
                return


if __name__ == "__main__":
    main()
