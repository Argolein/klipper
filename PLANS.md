# PLANS.md

## Objective
Investigate and test a likely Klipper USB-CDC communication issue on the
Duet3D Mini 5+ (SAME54P20A) connected over USB.

Observed failure: during a normal print, `bytes_retransmit` on the main MCU
starts rising after roughly 5-10 minutes, continues increasing, and eventually
the print crashes with `MCU 'mcu' shutdown: Timer too close`.

## Open questions
- Why does the SAME54/USB-CDC path still miss host ACK deadlines even though
  the measured `usb_cdc.c` task path shows only microsecond-scale max latency?
- Is the remaining issue caused by USB endpoint/bank state below
  `src/generic/usb_cdc.c`, host-side USB transport behavior, or a flow-control
  window that permits the 128-byte receive buffer to run full?
- Is `RECEIVE_WINDOW=USB_CDC_EP_BULK_OUT_SIZE` (64) a sufficient stable
  mitigation when combined with the current on-demand debug build?
- Can a PR be reduced to a small generic fix, or is this a Mini 5+ / SAME54
  board-specific workaround?
- Does the issue reproduce at idle with heaters/sensors active, or only during
  a normal print workload with dense `queue_step` traffic?

## Approved plan
- Apply a minimal SAME54/SAMD51 test patch that keeps CMCC enabled but disables
  data cache before enabling the cache controller.
- Build firmware for Duet3D Mini 5+:
  - Processor: SAME54P20
  - Clock: 25 MHz crystal
  - Bootloader: 16 KiB
  - Communication: USB
- Flash the Mini 5+ and run the same normal print workload.
- Acceptance check:
  - `bytes_retransmit` should not begin a continuous rise after the usual
    5-10 minute onset window.
  - Small isolated retransmits are acceptable; the failure signature is a
    monotonic rise until shutdown.
- The CMCC data-cache-only patch did not change behavior. Next approved
  investigation step is to add low-overhead USB-CDC debug counters in
  `src/atsamd/usbserial.c`.

## Implementation status
- [x] Read `PLANS-entwurf.md`
- [x] Reviewed `src/atsamd/usbserial.c`
- [x] Reviewed PR #7142 commit `3f72e519e`
- [x] Reviewed host retransmit logic in `klippy/chelper/serialqueue.c`
- [x] Identified CMCC data-cache coherency as the leading test hypothesis
- [x] Applied local test patch to `src/atsamd/samd51_clock.c`
- [x] Copy patch to Raspberry Pi
- [x] Build firmware on Raspberry Pi
- [x] Flash Duet3D Mini 5+ (second attempt successful)
- [x] Run reproduction print and evaluate `bytes_retransmit`
- [x] Add USB-CDC debug counters in `src/atsamd/usbserial.c`
- [x] Copy instrumented `src/atsamd/usbserial.c` to Raspberry Pi
- [x] Build and flash instrumented firmware
- [x] Capture `atsamd_usb` debug output during retransmit rise
- [x] Skip final crash debug blocks; `RECEIVE_WINDOW` test passed beyond prior
  crash/escalation window
- [x] Add first flow-control fix candidate: USB-CDC `RECEIVE_WINDOW`
- [x] Observe first positive test signal after `RECEIVE_WINDOW` patch
- [x] Observe test passing beyond prior crash/escalation window
- [x] Remove temporary USB debug instrumentation from `src/atsamd/usbserial.c`
- [x] Create clean PR branch `usb-fix-pr` with only `src/generic/usb_cdc.c`
  and no private markdown files
- [x] Confirm active firmware reports `RECEIVE_WINDOW=128`
- [x] Re-test after removing firmware debug code and observe retransmits return
- [x] Test conservative USB-CDC receive window
  `RECEIVE_WINDOW=USB_CDC_EP_BULK_OUT_SIZE` (64)
- [x] Confirm active firmware reports `RECEIVE_WINDOW=64`
- [x] Add host-side retransmit debug logging in
  `klippy/chelper/serialqueue.c`
- [x] Copy host-side retransmit debug patch to Pi
- [x] Restart Klipper so `c_helper.so` rebuilds
- [x] Capture `serialqueue retransmit ...` lines during the next retransmit
  burst
- [x] Determine remaining retransmits are timeout-driven, not duplicate
  ACK / NAK fast retransmits
- [x] Add host-side logging of the first timed-out message block prefix
- [x] Add low-noise MCU-side USB-CDC latency instrumentation for OUT, dispatch,
  and IN send paths
- [x] Flash the latency-instrumented firmware
- [x] Observe that the latency-instrumented firmware again masks the retransmit
  failure during the same print workload
- [x] Remove latency instrumentation from `src/generic/usb_cdc.c`
- [x] Restore USB-CDC `RECEIVE_WINDOW=sizeof(receive_buf)` (128) for root-cause
  testing
- [x] Add minimal atsamd USB descriptor/buffer memory barriers in
  `src/atsamd/usbserial.c`
- [x] Flash the barrier-only firmware test
- [x] Observe that the barrier-only firmware still reproduces retransmit bursts
  with `RECEIVE_WINDOW=128`
- [x] Add an explicit 16-byte alignment test for the atsamd USB descriptor
  table `usb_desc`
- [x] Flash the descriptor-alignment firmware test
- [x] Observe a strong positive signal: `mcu_rt` stayed flat at 9 through
  `print_time=1446.364` with `RECEIVE_WINDOW=128`
- [x] Isolate the candidate fix by removing the extra `__DMB()` barriers while
  keeping `usb_desc` explicitly aligned
- [x] Flash and test the alignment-only firmware candidate
- [x] Observe that alignment-only still reproduces retransmit bursts with
  `RECEIVE_WINDOW=128`
- [x] Restore the three `__DMB()` barriers for a combined
  alignment-plus-barrier candidate
- [x] Flash and test the combined `usb_desc __aligned(16)` plus `__DMB()`
  firmware candidate
- [x] Observe that the combined candidate can still reproduce retransmit bursts
  around `print_time=682-690`
- [x] Add a firmware/build identity marker before further A/B tests:
  `USB_CDC_DEBUG_BUILD=ondemand-20260510`
- [x] Change host-side `MIN_RTO` from 25ms to 50ms for the timeout sensitivity
  test, without changing MCU firmware
- [x] Copy `klippy/chelper/serialqueue.c` to the Pi, restart Klipper so the
  C helper rebuilds, and test the same print workload
- [x] Observe that `MIN_RTO=0.050` still reproduces retransmit growth around
  `print_time=813-821`
- [x] Revert host-side `MIN_RTO` back to the default 25ms because increasing it
  is not a fix and did not meaningfully change the failure pattern
- [x] Add on-demand USB-CDC MCU counters and max-latency tracking without
  periodic firmware output
- [x] Add temporary Klipper host helper `QUERY_USBCDC_DEBUG` to request the
  MCU snapshot during a print
- [x] Flash the on-demand-debug firmware and add `[usbcdc_debug]` on the Pi
- [x] During the next `mcu_rt` rise, run `QUERY_USBCDC_DEBUG` and compare
  OUT wake/read/dispatch latency with IN wake/send/busy counters
- [x] Capture manual `QUERY_USBCDC_DEBUG RESET=0` snapshot during an `mcu_rt`
  rise
- [x] Add Pre-flight `in_max_busy_us` tracking to the on-demand USB-CDC debug
  build
- [x] Add host-side monitor script
  `scripts/monitor_usbcdc_debug.py` to auto-run
  `QUERY_USBCDC_DEBUG RESET=0` when `mcu_rt` changes
- [x] Run the Pre-flight build and capture `QUERY_USBCDC_DEBUG RESET=0`
  during the next `mcu_rt` rise
- [x] Decide whether to proceed with dual-bank bulk endpoints based on
  `in_max_busy_us`
- [x] Implement smaller USB-CDC receive-side fix candidate:
  conservative `RECEIVE_WINDOW`, process-before-read OUT task ordering,
  bounded OUT read/dispatch loop, and negative block-parse counters
- [x] Build/flash/test the `rxdrain-20260510` receive-side candidate and
  confirm whether `max_rpos`, `max_pop`, `error`, and `mcu_rt` improve
- [x] Confirm `rxdrain-20260510` with a second successful reproduction print
- [x] Prepare PR-clean production diff limited to `src/generic/usb_cdc.c`

## Decisions
- Treat PR #7142 as already present. It fixes the previous `PCKSIZE.SIZE`
  encoding overflow but does not fully explain the current reproduction.
- Do not treat `bytes_invalid=0` as proof that the MCU receives every host
  packet on time; it only shows that the host is not seeing malformed response
  blocks.
- Do not treat `retransmit_seq` near `send_seq` as proof that every packet was
  retransmitted. In `serialqueue.c`, `retransmit_seq` is set to current
  `send_seq` when a retransmit happens.
- Use a minimal first test: disable CMCC data cache on SAMX5/SAMD51/SAME54 and
  leave instruction cache enabled.
- Use the Raspberry Pi 4B Klipper host at `192.168.178.82`.
- Use actual Pi SSH user `voron`.
- CMCC data-cache disable alone does not fix the reproduced failure.
- The CMCC data-cache test patch was reverted locally before adding USB-CDC
  instrumentation, so the next firmware is baseline cache behavior plus debug
  counters.
- The next fix candidate enables existing host-side receive-window flow control
  for USB-CDC by exporting `RECEIVE_WINDOW=sizeof(receive_buf)` from
  `src/generic/usb_cdc.c`.
- `RECEIVE_WINDOW=sizeof(receive_buf)` (128) is a valid generic missing-window
  fix candidate, but it did not fully fix the Mini 5+ reproduction after
  temporary firmware debug code was removed.
- A conservative USB-CDC receive window of `USB_CDC_EP_BULK_OUT_SIZE` (64)
  reduces the retransmit growth compared with 128, but still does not eliminate
  the issue. Treat receive-window sizing as a mitigating factor, not the full
  root cause.
- Do not keep shrinking the receive window as the main strategy. The next
  useful evidence is whether retransmits are caused by duplicate ACK / NAK fast
  retransmit or by timeout.
- Host-side retransmit debug is preferred over more firmware USB debug because
  the previous firmware debug build likely masked the timing-sensitive failure.
- The remaining retransmits with `RECEIVE_WINDOW=64` are timeout-driven. The
  host has exactly one pending message block, usually 60-64 bytes, and the MCU
  does not ACK it within the current 25ms minimum RTO. This shifts suspicion
  away from duplicate ACK / stale ACK sequence handling.
- The next firmware instrumentation must be latency-triggered and sparse,
  because previous periodic firmware USB debug likely changed timing enough to
  mask the issue.
- The latency-triggered USB-CDC instrumentation also appears to mask the issue.
  Treat "no retransmits with debug enabled" as evidence for a Heisenbug
  (memory ordering, layout-sensitive memory corruption, or timing), not as a
  fix.
- The next strongest fix candidate is minimal memory barriers around the
  atsamd USB descriptor/buffer handoff in `src/atsamd/usbserial.c`. This should
  be tested after removing the latency instrumentation from
  `src/generic/usb_cdc.c`, while keeping host-side retransmit logging for
  observation.
- For root-cause testing, use `RECEIVE_WINDOW=sizeof(receive_buf)` (128), not
  the 64-byte mitigation, so the remaining Mini 5+ failure is not hidden by
  reduced host pressure.
- Minimal `__DMB()` barriers alone do not fix the Mini 5+ retransmit burst with
  `RECEIVE_WINDOW=128`; keep memory ordering as a possible factor but stop
  treating barriers alone as the leading fix.
- Test explicit `usb_desc` alignment next. This is a targeted layout hypothesis:
  the atsamd USB peripheral reads the descriptor table from SRAM via
  `DESCADD`, and debug builds may shift the table address/alignment enough to
  mask the failure.
- The descriptor-alignment test is the first strong positive signal that
  remains on `RECEIVE_WINDOW=128` without latency/debug masking. It is not yet
  final proof because the current test build also still includes the extra
  `__DMB()` barriers.
- The extra `__DMB()` barriers were removed again for the isolation test because
  the barrier-only build already reproduced the failure. The active candidate
  is now explicit descriptor-table alignment only.
- Alignment-only reproduced the failure again around `print_time=640-675` with
  rapid `mcu_rt` growth. Since barrier-only also failed, the leading candidate
  is now the combination: explicit descriptor-table alignment plus memory
  barriers at the CPU/USB handoff points.
- The combined alignment-plus-barrier candidate later reproduced the failure
  around `print_time=682-690`. The earlier run to `print_time=1566` was a
  strong but non-conclusive positive signal in a sporadic failure mode. Do not
  treat one long stable partial print as proof of fix.
- Because current host debug shows timeout-driven retransmits with usually one
  pending block, a host-side `MIN_RTO` sensitivity test is now useful. If a
  50-100ms minimum RTO prevents the retransmit storm, the remaining issue is
  likely late ACK/response under load rather than persistent packet loss.
- The 128-window combined firmware candidate also produced timeout retransmits
  with `pending=2` and `bytes=122-124`, which is consistent with the larger
  receive window allowing two outstanding message blocks. The first RTO
  sensitivity step is `MIN_RTO=0.050`.
- `MIN_RTO=0.050` still reproduced the failure and did not meaningfully change
  the pattern. Increasing host retransmit timeout is not a solution for this
  issue. Restore the default `MIN_RTO=0.025`.
- The next debug build must avoid periodic `output()` spam. It records counters
  and maximum observed latencies in the USB-CDC hot path and reports them only
  when the host sends `get_usbcdc_debug`.
- Manual snapshot during retransmit rise:
  - `in notify=115542 wake=101875 task=102881 send=57498 busy=1003 bytes=453935 max_wake_us=140 max_send_us=5 max_queued=72`
  - `out notify=97078 wake=86430 task=86430 read=43215 empty=43215 bytes=2273346 max_wake_us=45 max_read_us=6`
  - `dispatch max_dispatch_us=33 max_total_us=36 max_rpos=127 max_pop=64`
- Interpretation: the measured MCU USB-CDC task path is not showing
  millisecond-level starvation. Maximum observed OUT wake/read/dispatch and IN
  wake/send durations are far below the 25ms host retransmit timeout. This
  shifts suspicion away from MCU scheduler/task latency and toward lower-level
  USB transfer completion, ACK response visibility/loss, or host USB transport
  behavior.
- Because `max_rpos=127` with `RECEIVE_WINDOW=128`, the receive buffer did run
  essentially full during the failing workload. This revives the conservative
  receive-window hypothesis: even if 128 is the literal buffer size, it may be
  too aggressive for USB-CDC packet/bank timing on this path. The next
  diagnostic test should set `RECEIVE_WINDOW=USB_CDC_EP_BULK_OUT_SIZE` (64)
  again while keeping the on-demand snapshot counters.
- Before implementing dual-banked bulk endpoints, run the Pre-flight
  `in_max_busy_us` measurement and classify the result using `dual-bank.md`.
  Do not start the dual-bank code change until that measurement is captured.
- Pre-flight result during `mcu_rt` growth:
  - `in max_busy_us=177`, with `in_busy=3806` and `in_send=240700`.
  - `out max_rpos=127`, `max_pop=70`.
  - This is a negative Pre-flight result for the IN-busy hypothesis because
    177us is far below the `<5000us` stop threshold in `dual-bank.md`.
    Therefore, do not implement the dual-banked bulk endpoint plan as the main
    fix path unless new evidence contradicts this measurement.
- Next fix candidate after the negative Dual-Bank Pre-flight:
  - Keep the focus on USB-CDC receive-side flow control and buffer drainage.
  - Change USB-CDC `RECEIVE_WINDOW` from the full 128-byte receive buffer to a
    conservative window with one USB packet of headroom, likely
    `sizeof(receive_buf) - USB_CDC_EP_BULK_OUT_SIZE` (64).
  - Rework `usb_bulk_out_task()` to process any already-buffered complete
    message block before reading another 64-byte USB OUT packet, then use a
    small bounded loop to read/process at most a couple of packets per task
    run. The goal is to send ACKs earlier and prevent `receive_pos` from
    repeatedly reaching 127.
  - Treat `max_pop=70` as a warning sign: normal valid message blocks are at
    most `MESSAGE_MAX=64`, so values above 64 likely come from the resync/error
    discard path. The next build should record whether `command_find_block()`
    returns negative and what `pop_count` was.
- Implemented receive-side candidate in `src/generic/usb_cdc.c`:
  - `USB_CDC_DEBUG_BUILD=rxdrain-20260510`.
  - `RECEIVE_WINDOW=sizeof(receive_buf) - USB_CDC_EP_BULK_OUT_SIZE` (64).
  - `usb_bulk_out_task()` now tries to process buffered data before reading
    another USB OUT packet, with a bound of two dispatches and two reads per
    task run.
  - `usbcdc_debug_dispatch` now reports `error` and `max_error_pop`.
- `rxdrain-20260510` completed the reproduction print successfully:
  - Final monitor line: `pt=3004.818 mcu_rt=9 seq=230621/230621 rtseq=2
    srtt=0.000 rto=0.025 | ebb_rt=9 inv=0 buf=1.081 stall=0 load=0.00`.
  - Final debug snapshot:
    `in max_wake_us=84 max_send_us=7 max_queued=75 max_busy_us=83`,
    `out max_wake_us=14 max_read_us=6`,
    `dispatch max_dispatch_us=36 max_total_us=39 max_rpos=64 max_pop=64
    error=0 max_error_pop=0`.
  - Interpretation: the receive-side candidate prevented the RX buffer from
    reaching the previous `max_rpos=127` failure condition and no longer
    produced retransmit growth during this full run.
- `rxdrain-20260510` also completed a second reproduction print successfully:
  - Final monitor line: `pt=5784.102 mcu_rt=9 seq=452942/452942 rtseq=2
    srtt=0.000 rto=0.025 | ebb_rt=9 inv=0 buf=1.275 stall=0 load=0.01`.
  - Final debug snapshot:
    `in max_wake_us=122 max_send_us=6 max_queued=74 max_busy_us=116`,
    `out max_wake_us=19 max_read_us=6`,
    `dispatch max_dispatch_us=37 max_total_us=40 max_rpos=64 max_pop=64
    error=0 max_error_pop=0`.
  - Interpretation: two full reproduction prints now pass with no retransmit
    growth and no receive parser errors. Treat the receive-side flow-control
    and drainage candidate as the leading fix.
- PR-clean production diff prepared:
  - Kept only `src/generic/usb_cdc.c` changes.
  - Removed `USB_CDC_DEBUG_BUILD`, `get_usbcdc_debug`, and all debug counters
    from the production diff.
  - Removed local debug changes from `klippy/chelper/serialqueue.c` and
    `src/atsamd/usbserial.c`.
  - Left private untracked investigation helpers/documents in the working tree
    for local reference only; do not commit them.
- Historical RepRapFirmware SD-card/printing hiccups on Duet 3 Mini/Mini 5+
  may be relevant as a class of timing/peripheral-latency issue, but not as
  evidence that USB and SD share the same external bus. In Klipper the board is
  not streaming G-code from the Duet SD card, so this is mainly useful as
  context for SAME54/Mini timing sensitivity.

## Findings So Far

### Confirmed from user report
- Board: Duet3D Mini 5+ with SAME54P20A.
- Connection: USB-CDC.
- Host: Raspberry Pi 4B running Moonraker OS / Klipper.
- Issue is sporadic in onset but repeatable in pattern.
- During a normal print, the main MCU retry counter starts rising after about
  5-10 minutes.
- Once it starts, the counter keeps climbing until crash.
- Example before visible rise:
  - `mcu_rt=9`
  - `inv=0`
  - `ebb_rt=9`
  - `print_time=1141.478`
  - `buffer=1.917`
  - `stall=1`
  - `load=0.54`
- Later in the same pattern:
  - `mcu_rt=1217`
  - `inv=0`
  - `ebb_rt=9`
  - `print_time=1302.182`
  - `buffer=1.519`
  - `stall=1`
  - `load=0.10`
- EBB toolboard on the same Pi remains stable while the Mini 5+ counter rises.

### Code observations
- `src/atsamd/usbserial.c` stores USB descriptors and USB endpoint buffers in
  normal SRAM:
  - `usb_desc`
  - `ep0out`
  - `ep0in`
  - `acmin`
  - `bulkout`
  - `bulkin`
- `src/atsamd/usbserial.c` gives the descriptor table to the USB peripheral via
  `USB->DEVICE.DESCADD.reg = (uint32_t)usb_desc`.
- The USB peripheral writes OUT packet data and descriptor state directly into
  SRAM. The CPU then reads that memory in `usb_read_packet()`.
- `src/atsamd/samd51_clock.c` previously enabled the CMCC cache controller with
  `CMCC->CTRL.reg = 1` without disabling data cache.
- Microchip's CMCC has a Data Cache Disable bit, `CMCC_CFG_DCDIS`.

### Leading hypothesis
The SAME54 USB peripheral and CPU are sharing SRAM buffers without explicit
cache maintenance. If CMCC data cache is enabled, the CPU may read stale USB
packet data or stale USB descriptor fields after the USB peripheral has updated
SRAM. That can produce valid-looking but temporally wrong Klipper messages,
causing ACK/retransmit instability without host-side `bytes_invalid`.

This fits the observed pattern:
- `bytes_invalid=0` can remain true because data is not necessarily random bit
  corruption.
- The EBB remains healthy because it uses a different MCU/USB implementation.
- The issue can begin only after minutes because it depends on cache line and
  SRAM layout interactions.
- Different compilers can change timing/layout, changing when the issue starts.

### Cache patch test result
The patched firmware was flashed successfully and tested in a normal print. The
failure reproduced with the same signature:

```text
tick   pt=1246.075 mcu_rt=9     seq=50837/50837 rtseq=2     srtt=0.000 rto=0.025 | ebb_rt=9 inv=0
change pt=1301.277 mcu_rt=499   seq=53472/53472 rtseq=53435 srtt=0.000 rto=0.025 | ebb_rt=9 inv=0
change pt=1340.030 mcu_rt=1110  seq=55195/55195 rtseq=55195 srtt=0.000 rto=0.100 | ebb_rt=9 inv=0
change pt=1363.530 mcu_rt=12632 seq=56242/56248 rtseq=56248 srtt=0.000 rto=0.200 | ebb_rt=9 inv=0
change pt=1369.822 mcu_rt=18009 seq=56572/56572 rtseq=56572 srtt=0.001 rto=0.100 | ebb_rt=9 inv=0
```

Interpretation:
- The pure CMCC data-cache hypothesis is weakened.
- The failure still looks local to the Mini 5+ USB path because `EBBCan`
  remains stable and `bytes_invalid` remains zero.
- The most useful next instrumentation point is the atsamd USB-CDC boundary:
  endpoint ready/busy state, transfer-complete interrupt counts, RX packet
  byte counts, and whether `usb_send_bulk_in()` repeatedly returns busy while
  ACKs are queued.

### USB-CDC instrumentation added
Local file changed: `src/atsamd/usbserial.c`.

Counters added:
- Bulk-IN sends, busy returns, transmitted bytes, IN IRQs.
- Bulk-OUT reads, empty polls, received bytes, OUT IRQs.
- EP0 IRQs, reset IRQs, configuration count.
- Last observed endpoint status, endpoint interrupt flags, and descriptor
  `PCKSIZE` for Bulk-IN and Bulk-OUT.

The firmware emits low-frequency `#output` messages every 15 seconds only if
the tracked USB counters changed:

```text
atsamd_usb bi s=... busy=... bytes=... irq=... st=... fl=...
atsamd_usb bo r=... empty=... bytes=... irq=... st=... fl=...
atsamd_usb ctl ep0=... reset=... cfg=... bip=... bop=...
```

Expected use:
- Start the normal print.
- Keep the existing low-noise host monitor running.
- In a second shell, watch firmware debug output with:

```bash
tail -n 0 -F ~/printer_data/logs/klippy.log | grep --line-buffered 'atsamd_usb'
```

Interpretation focus:
- If Bulk-IN `busy` rises sharply near `mcu_rt` onset, the MCU is trying to
  send ACK/response data but the USB IN endpoint is not clearing quickly.
- If Bulk-OUT `empty` rises abnormally or OUT IRQ/read counts stall, the host
  traffic is not being consumed normally.
- If IRQ counts stop moving while status bits show ready/busy, suspect
  endpoint interrupt handling or bank state.

### Instrumented run partial result
The retransmit counter began rising while USB counters continued moving.

Sample before and during the rise:

```text
atsamd_usb bi s=65112 busy=1176 bytes=655110 irq=65111 st=6 fl=10
atsamd_usb bo r=36508 empty=36464 bytes=1201473 irq=36508 st=0 fl=0
change pt=754.422 mcu_rt=32 seq=35903/35907 rtseq=35881
change pt=777.150 mcu_rt=1319 seq=37028/37028 rtseq=37020
atsamd_usb bi s=66285 busy=1217 bytes=666091 irq=66283 st=134 fl=8
atsamd_usb bo r=37246 empty=37206 bytes=1235481 irq=37246 st=0 fl=0
change pt=793.587 mcu_rt=4365 seq=37814/37814 rtseq=37770
atsamd_usb bi s=69810 busy=1352 bytes=698962 irq=69809 st=6 fl=10
atsamd_usb bo r=39469 empty=39434 bytes=1337233 irq=39469 st=1 fl=0
tick pt=853.713 mcu_rt=4365 seq=40602/40602 rtseq=37770
```

Interpretation:
- Bulk-IN IRQ count tracks Bulk-IN send count closely; the IN endpoint is not
  obviously stuck.
- Bulk-IN `busy` rises slowly, not explosively at retransmit onset.
- Bulk-OUT read count equals Bulk-OUT IRQ count in the samples; OUT IRQ/read
  handling is not obviously falling behind.
- Bulk-OUT `PCKSIZE` byte counts vary in plausible packet-size ranges.
- This weakens the "atsamd endpoint state got stuck" hypothesis and shifts
  suspicion toward timing/scheduler/host retransmit interaction or an ACK
  generation/processing delay above the raw USB endpoint layer.

### Fix candidate: USB-CDC receive window
UART and CAN serial paths export a `RECEIVE_WINDOW` constant to the host:

```text
src/generic/serial_irq.c: DECL_CONSTANT("RECEIVE_WINDOW", RX_BUFFER_SIZE)
src/generic/canserial.c: DECL_CONSTANT("RECEIVE_WINDOW", ARRAY_SIZE(CanData.receive_buf))
```

`src/generic/usb_cdc.c` has a 128-byte `receive_buf` but previously exported
no `RECEIVE_WINDOW`. Without this constant, the host only limits outstanding
traffic by `MAX_PENDING_BLOCKS` rather than by the MCU receive buffer capacity.
That can create phase-like bursts where ACKs are delayed enough to trigger
host retransmits even though raw USB endpoints keep moving.

Local patch:

```c
static struct task_wake usb_bulk_out_wake;
static uint8_t receive_buf[128], receive_pos;

DECL_CONSTANT("RECEIVE_WINDOW", sizeof(receive_buf));
```

Expected effect:
- Host should throttle outstanding USB-CDC traffic based on the MCU receive
  buffer.
- If this is the issue, `mcu_rt` should stop rising in bursts during normal
  print load, or the bursts should become much smaller and not cascade.
- USB debug counters can stay enabled during the next test to confirm endpoint
  behavior remains normal.

Initial test signal after flashing the `RECEIVE_WINDOW` patch:

```text
tick pt=1319.895 mcu_rt=9 seq=62554/62554 rtseq=2 srtt=0.000 rto=0.025 | ebb_rt=9 inv=0 buf=1.389 stall=1 load=0.02
tick pt=1500.110 mcu_rt=9 seq=71072/71072 rtseq=2 srtt=0.000 rto=0.025 | ebb_rt=9 inv=0 buf=1.484 stall=1 load=0.12
```

Interpretation:
- This is a strong positive intermediate result. In the prior failed run,
  retransmits had already started around `pt=1301` and reached `mcu_rt=18009`
  by about `pt=1369`.
- The patched run reached `pt=1500.110` with `mcu_rt=9`, so it has clearly
  passed the previous escalation/crash region under the same reproduction
  pattern.
- This was later reclassified as a false positive / timing-masked run after
  removing the temporary firmware USB debug code.

Follow-up after removing firmware debug code:

```text
RECEIVE_WINDOW=128
change pt=1371.246 mcu_rt=67   seq=40260/40260 rtseq=40229 srtt=0.000 rto=0.025
change pt=1391.363 mcu_rt=1024 seq=40931/40931 rtseq=40915 srtt=0.000 rto=0.025
change pt=1540.533 mcu_rt=3217 seq=46940/46940 rtseq=46914 srtt=0.000 rto=0.025
```

Interpretation:
- Exporting `RECEIVE_WINDOW=128` is not sufficient to fix the Mini 5+
  reproduction.
- The earlier stable run likely had timing changed by temporary firmware debug
  instrumentation.
- Constant `rto=0.025` suggests duplicate ACK / NAK fast retransmit is more
  likely than timeout-driven retransmit.

Conservative receive-window test:

```c
DECL_CONSTANT("RECEIVE_WINDOW", USB_CDC_EP_BULK_OUT_SIZE);
```

The active firmware reported:

```text
RECEIVE_WINDOW=64
```

Observed result:

```text
tick   pt=550.853 mcu_rt=9   seq=27970/27970 rtseq=2     srtt=0.000 rto=0.025
change pt=551.883 mcu_rt=137 seq=28004/28004 rtseq=27989 srtt=0.000 rto=0.025
change pt=559.981 mcu_rt=202 seq=28276/28276 rtseq=28272 srtt=0.000 rto=0.025
tick   pt=620.331 mcu_rt=202 seq=30227/30227 rtseq=28272 srtt=0.000 rto=0.025
change pt=670.979 mcu_rt=267 seq=31526/31526 rtseq=31525 srtt=0.000 rto=0.025
change pt=805.630 mcu_rt=389 seq=35720/35720 rtseq=35709 srtt=0.000 rto=0.025
tick   pt=865.979 mcu_rt=389 seq=37714/37714 rtseq=35709 srtt=0.000 rto=0.025
```

Interpretation:
- `64` is less dramatic than `128` and the connection can recover between
  bursts.
- The remaining issue is not fully solved by receive-window sizing.
- Since `MESSAGE_MAX=64` already includes the 2-byte header and 3-byte trailer,
  setting the window to `58` is unlikely to be a meaningful next step.

### Host-side retransmit debug added
Local file changed: `klippy/chelper/serialqueue.c`.

The patch logs one line per retransmit:

```text
serialqueue retransmit name=... cause=nak|timeout bytes=... pending=...
  first=... send_seq=... receive_seq=... retransmit_seq=...
  last_ack_seq=... ignore_nak_seq=... nak_seq=... nak_len=...
  need_ack_bytes=... last_ack_bytes=... receive_window=...
  srtt=... rto=... first_msg=...
```

Expected use on the Pi:

```bash
tail -n 0 -F ~/printer_data/logs/klippy.log | grep --line-buffered 'serialqueue retransmit'
```

Next evidence needed:
- If `cause=nak`, the remaining issue is duplicate ACK / NAK fast retransmit.
- If `cause=timeout`, the remaining issue is delayed/lost responses or host/USB
  scheduling latency.
- Compare the retransmit debug line with the nearby low-noise monitor output.

Captured result with `RECEIVE_WINDOW=64`:

```text
serialqueue retransmit name=serialq mcu cause=timeout bytes=64 pending=1 first=64 send_seq=41954 receive_seq=41953 retransmit_seq=2 last_ack_seq=41953 ignore_nak_seq=2 need_ack_bytes=63 last_ack_bytes=63 receive_window=64 srtt=0.000 rto=0.025
serialqueue retransmit name=serialq mcu cause=timeout bytes=65 pending=1 first=65 send_seq=42089 receive_seq=42088 retransmit_seq=41954 last_ack_seq=42088 ignore_nak_seq=41954 need_ack_bytes=64 last_ack_bytes=61 receive_window=64 srtt=0.000 rto=0.025
serialqueue retransmit name=serialq mcu cause=timeout bytes=65 pending=1 first=65 send_seq=42089 receive_seq=42088 retransmit_seq=42089 last_ack_seq=42088 ignore_nak_seq=42089 need_ack_bytes=64 last_ack_bytes=61 receive_window=64 srtt=0.000 rto=0.050
```

Nearby monitor output:

```text
change pt=1002.250 mcu_rt=73  seq=41977/41978 rtseq=41954 srtt=0.000 rto=0.025
change pt=1004.503 mcu_rt=267 seq=42152/42153 rtseq=42110 srtt=0.000 rto=0.025
change pt=1009.285 mcu_rt=389 seq=42574/42575 rtseq=42555 srtt=0.000 rto=0.025
```

Interpretation:
- Relevant retransmits are timeouts, not duplicate ACK / NAK fast retransmits.
- `pending=1` and `receive_window=64` show the host is not flooding the MCU
  with multiple unacked blocks during these events.
- Timed-out blocks are close to full size (`bytes=64/65`, where the extra byte
  is the retransmit sync prefix).
- The MCU eventually catches up, but not within the 25ms minimum RTO.
- Next host-side debug should identify the timed-out message block type/content
  before returning to firmware-side instrumentation.

Follow-up local change:
- `serialqueue.c` now includes `first_msg=<hex>` with the full first timed-out
  pending message block.
- After copying this updated file to the Pi and restarting Klipper, the next
  `serialqueue retransmit ...` lines should show the message prefix. Use that
  to determine whether the same command/block type repeatedly times out.

Captured 16-byte prefix samples before full-block logging was enabled:

```text
first_msg=3f:16:16:04:83:cd:3b:06:ed:57:16:07:87:bb:0e:02
first_msg=3e:1e:16:07:a1:af:08:01:00:16:04:83:ce:47:06:eb
first_msg=0e:19:2f:00:05:ea:13:e8:2d:cc:0a:dc:cf:7e
first_msg=3e:17:16:04:81:97:57:25:ff:02:16:04:80:f6:54:3c
first_msg=3a:12:0e:01:00:16:07:8b:a4:1c:02:fe:89:35:16:04
first_msg=3c:1c:16:04:82:cf:14:1b:fc:3d:16:08:81:df:af:52
```

Interpretation:
- Timed-out blocks are not byte-identical.
- The first byte is the Klipper message-block length and the second byte is the
  sequence byte. The first command id starts at byte 3.
- The first command ids vary across samples, so this is probably not one single
  bad command. Full-block dumps plus the MCU dictionary are needed for proper
  decoding.

Full-block samples show many repeated `0x16` command ids with different OIDs:

```text
first_msg=3e:13:16:08:a9:fa:6d:02:ff:96:2d:16:0a:a9:fa:6d:02:ff:96:2d:16:0b:a9:fa:6d:02:ff:96:2d:16:0c:a9:fa:6d:02:ff:96:2d:16:07:a6:65:81:2b:7e:16:04:aa:06:81:49:7f:16:07:a4:75:81:1e:7f:cf:2a:7e
first_msg=3c:13:16:07:83:88:28:04:b5:15:16:04:8c:9f:78:01:00:16:04:88:f6:3f:02:fe:d9:62:16:07:85:f2:01:01:00:16:07:89:e5:1a:01:00:16:04:87:81:12:02:00:15:07:00:16:07:90:9c:69:01:00:aa:e0:7e
```

Interpretation:
- These blocks are dominated by repeated `0x16` commands, which strongly
  suggests high-rate motion/step queue traffic, likely `queue_step`, but this
  must be confirmed against the exact Pi `out/klipper.dict`.
- The issue is now best described as intermittent ACK timeout of normal
  motion command blocks. It is not caused by multiple outstanding blocks
  (`pending=1`) and not by duplicate ACK / NAK fast retransmit.

Pi dictionary confirmation:

```text
0x11 config_digital_out oid=%c pin=%u value=%c default_value=%c max_duration=%u
0x12 stepper_stop_on_trigger oid=%c trsync_oid=%c
0x13 stepper_get_position oid=%c
0x15 set_next_step_dir oid=%c dir=%c
0x16 queue_step oid=%c interval=%u count=%hu add=%hi
0x17 config_stepper oid=%c step_pin=%c dir_pin=%c invert_step=%c step_pulse_ticks=%u
0x19 endstop_home oid=%c clock=%u sample_ticks=%u sample_count=%c rest_ticks=%u pin_value=%c trsync_oid=%c trigger_reason=%c
0x1c trsync_set_timeout oid=%c clock=%u
0x1e config_trsync oid=%c
```

Conclusion:
- `0x16` is confirmed as `queue_step`.
- The repeated `0x16` entries in timed-out blocks are ordinary stepper motion
  commands for multiple stepper OIDs.
- The remaining failure is now narrowed to intermittent timeout/late ACK of
  normal motion traffic over Mini 5+ SAME54 USB-CDC.

### USB-CDC latency instrumentation added
Local file changed: `src/generic/usb_cdc.c`.

The firmware patch keeps `RECEIVE_WINDOW=USB_CDC_EP_BULK_OUT_SIZE` and adds
latency-triggered `#output` diagnostics. It only emits when one of the measured
USB-CDC path segments exceeds `USB_CDC_LATENCY_WARN_US=10000`.

Output formats:

```text
usbcdc_lat out wake_us=... read_us=... dispatch_us=... total_us=... rpos=... pop=... read=... dispatch=...
usbcdc_lat in wake_us=... send_us=... queued=... max=... ret=...
```

Interpretation:
- `out wake_us` high: USB OUT notification happened, but the USB OUT task did
  not run promptly. This points to task starvation / timer or stepper interrupt
  load.
- `out dispatch_us` high: the MCU task ran, but command dispatch of the
  received block took too long. This points to command/step scheduling cost.
- `in wake_us` high: the ACK/response was queued, but the USB IN task did not
  run promptly. This points to task starvation on the response side.
- `in send_us` high or low/negative `ret`: the USB IN endpoint handoff is slow
  or busy.
- No `usbcdc_lat` lines during host timeout bursts: none of these measured
  firmware segments exceeded 10ms; next step would be lower threshold or
  atsamd endpoint-level timing.

Expected live monitor:

```bash
tail -n 0 -F ~/printer_data/logs/klippy.log | grep --line-buffered -E 'serialqueue retransmit|usbcdc_lat'
```

Follow-up result:

```text
tick pt=1238.752 mcu_rt=9 seq=82379/82379 rtseq=2 srtt=0.000 rto=0.025 | ebb_rt=9 inv=0 buf=1.126 stall=0 load=0.40
```

Interpretation:
- The latency-instrumented firmware again ran beyond the expected onset window
  without reproducing the retransmit rise.
- This is not proof that the instrumented code fixes the issue. It strongly
  suggests the instrumentation changes memory layout, compiler ordering, or
  timing enough to mask the underlying failure.
- A padding-only test can help probe memory-layout sensitivity, but it is not
  decisive because a stable padding build could also be caused by code layout
  or timing changes.
- A more actionable next test is to remove the latency instrumentation and add
  only explicit memory barriers around the atsamd USB descriptor/buffer handoff
  in `src/atsamd/usbserial.c`.

### Local test patch
Changed `src/atsamd/samd51_clock.c`:

```c
// Enable instruction cache only. The USB peripheral accesses SRAM buffers
// directly, so data cache can leave usbserial.c with stale packet data.
CMCC->CTRL.reg = 0;
while (CMCC->SR.reg & CMCC_SR_CSTS)
    ;
CMCC->CFG.reg |= CMCC_CFG_DCDIS;
CMCC->CTRL.reg = CMCC_CTRL_CEN;
```

## Test Commands

Track retransmit changes during/after a print:

```bash
perl -ne '
  if (/^Stats / && /\bmcu: .*?bytes_retransmit=(\d+).*?bytes_invalid=(\d+).*?send_seq=(\d+) receive_seq=(\d+) retransmit_seq=(\d+)/) {
    next if $1 == $last;
    print "$.: mcu_rt=$1 inv=$2 send=$3 recv=$4 retransmit_seq=$5\n";
    $last = $1;
  }
' ~/printer_data/logs/klippy.log
```

Low-noise live monitor for the reproduction print. It prints only on first
sample, retransmit/invalid counter changes, or a 60 second heartbeat:

```bash
tail -n 0 -F ~/printer_data/logs/klippy.log | perl -MTime::HiRes=time -ne '
  next unless /^Stats / && /\bmcu: /;

  my ($mcu)     = /\bmcu: .*?bytes_retransmit=(\d+)/;
  next unless defined $mcu;
  my ($invalid) = /\bmcu: .*?bytes_invalid=(\d+)/;
  my ($ebb)     = /\bEBBCan: .*?bytes_retransmit=(\d+)/;
  my ($buffer)  = /\bbuffer_time=([0-9.]+)/;
  my ($stall)   = /\bprint_stall=(\d+)/;
  my ($load)    = /\bsysload=([0-9.]+)/;
  my ($srtt)    = /\bmcu: .*?srtt=([0-9.]+)/;
  my ($rto)     = /\bmcu: .*?rto=([0-9.]+)/;
  my ($sseq)    = /\bmcu: .*?send_seq=(\d+)/;
  my ($recvseq) = /\bmcu: .*?receive_seq=(\d+)/;
  my ($rtseq)   = /\bmcu: .*?retransmit_seq=(\d+)/;
  my ($ptime)   = /\bprint_time=([0-9.]+)/;

  $_ = defined($_) ? $_ : "-" for ($invalid, $ebb, $buffer, $stall, $load,
                                  $srtt, $rto, $sseq, $recvseq, $rtseq, $ptime);

  my $now = time;
  my $changed = !defined($last_mcu) || $mcu != $last_mcu
      || $invalid ne $last_invalid || $ebb ne $last_ebb;
  my $heartbeat = !defined($last_print) || $now - $last_print >= 60;
  next unless $changed || $heartbeat;

  my $why = !defined($last_mcu) ? "start" : $changed ? "change" : "tick";
  printf "%s pt=%-8s mcu_rt=%-7s seq=%s/%s rtseq=%s srtt=%s rto=%s | ebb_rt=%s inv=%s buf=%s stall=%s load=%s\n",
      $why, $ptime, $mcu, $recvseq, $sseq, $rtseq, $srtt, $rto,
      $ebb, $invalid, $buffer, $stall, $load;

  $last_mcu = $mcu;
  $last_invalid = $invalid;
  $last_ebb = $ebb;
  $last_print = $now;
'
```

Find first nonzero retransmit window in a log:

```bash
LOG=~/printer_data/logs/klippy.log

line=$(perl -ne '
  if (/^Stats / && /\bmcu: .*?bytes_retransmit=(\d+)/ && $1 > 0) {
    print $.;
    exit;
  }
' "$LOG")

echo "first nonzero bytes_retransmit line: $line"
start=$(( line > 120 ? line - 120 : 1 ))
end=$(( line + 40 ))
sed -n "${start},${end}p" "$LOG"
```

## Raspberry Pi / Flash Notes
- Klipper Raspberry Pi 4B IP: `192.168.178.82`
- SSH user: `voron`
- Build on the Pi using the Pi's Klipper checkout, then flash from there.
- Do not commit this investigation file automatically.

### Flash attempt 2026-05-09
Command run on Pi:

```bash
make flash FLASH_DEVICE=/dev/ttyACM0
```

Observed output:

```text
Flashing out/klipper.bin to /dev/ttyACM0
Entering bootloader on /dev/serial/by-path/platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.4:1.0
Device reconnect on /dev/serial/by-path/platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.4:1.0
lib/bossac/bin/bossac -U -p /dev/serial/by-path/platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.4:1.0 --offset=0x4000 -b -R -w out/klipper.bin -v
SAM-BA operation failed
Failed to flash to /dev/ttyACM0: Error running bossac
```

Device after the failed attempt:

```text
/dev/serial/by-id/usb-Duet3D_Duet3-Mini5+_AE5C301C533346484E202020FF183523-if00 -> ../../ttyACM0
```

Interpretation: that `by-id` name looks like the normal Klipper/Duet3D CDC
application device, not strong evidence that the board is currently in the
BOSSA/SAM-BA bootloader.

### Successful flash 2026-05-09
Command run on Pi:

```bash
make flash FLASH_DEVICE=/dev/serial/by-id/usb-Duet3D_Duet3-Mini5+_AE5C301C533346484E202020FF183523-if00
```

Observed output:

```text
Write 38344 bytes to flash (75 pages)
Verify 38344 bytes of flash
Verify successful
Set boot flash true
```

Interpretation: patched firmware was successfully written and verified.

## Handoff
- Agent: Codex
- Date: 2026-05-10
- Completed this session:
  - Read `PLANS.md` and `dual-bank.md`.
  - Added Pre-flight `in_max_busy_us` tracking to `src/generic/usb_cdc.c`.
  - Updated `klippy/extras/usbcdc_debug.py` to print `max_busy_us`.
  - Added `scripts/monitor_usbcdc_debug.py`, which watches
    `mcu: bytes_retransmit` in `klippy.log` and sends
    `QUERY_USBCDC_DEBUG RESET=0` through Klipper's webhooks socket when it
    changes.
  - Updated the monitor script to try the modern Moonraker/Klipper socket
    path `~/printer_data/comms/klippy.sock` before the older
    `/tmp/klippy_uds` path.
  - Classified the Pre-flight snapshot from the Pi as negative:
    `max_busy_us=177`, far below the `<5000us` stop threshold.
  - Implemented the smaller USB-CDC receive-side candidate in
    `src/generic/usb_cdc.c`: conservative 64-byte receive window,
    process-before-read OUT task ordering, bounded two-read/two-dispatch loop,
    and parse-error/max-error-pop counters.
  - Updated `klippy/extras/usbcdc_debug.py` for the new
    `usbcdc_debug_dispatch error=... max_error_pop=...` fields.
  - Observed a full reproduction print complete with `mcu_rt=9`,
    `max_rpos=64`, `max_pop=64`, `error=0`, and `max_error_pop=0`.
  - Observed a second full reproduction print complete with the same clean
    signature: `mcu_rt=9`, `max_rpos=64`, `max_pop=64`, `error=0`, and
    `max_error_pop=0`.
  - Reduced the production diff to `src/generic/usb_cdc.c` only.
  - Ran `python3 -m py_compile klippy/extras/usbcdc_debug.py`.
  - Ran `python3 -m py_compile scripts/monitor_usbcdc_debug.py`.
  - Ran `git diff --check -- src/generic/usb_cdc.c klippy/extras/usbcdc_debug.py`.
  - Ran `git diff --check -- scripts/monitor_usbcdc_debug.py`.
- Stopped at:
  - Production diff is cleanly limited to `src/generic/usb_cdc.c`. Private
    untracked investigation files remain in the working tree and should not be
    committed.
- Next step:
  - Build/flash the clean production diff on the Pi once to confirm it still
    connects and prints without the debug instrumentation. Then prepare commit
    message/PR text.
- Open blockers:
  - Need one confirmation print with the clean production diff before commit/PR.
- Decisions made this session:
  - Do not start the dual-bank implementation from `dual-bank.md`; the
    Pre-flight measurement was negative (`max_busy_us=177`).
  - Test receive-side flow-control/drainage before revisiting lower-level USB
    endpoint architecture.
  - Two clean reproduction prints are enough to proceed to a clean production
    patch for review.

## Notes
- `PLANS-entwurf.md` remains as the original investigation draft.
- This file is the active session log.
