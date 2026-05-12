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
- [x] Capture usbmon and host retransmit logs for a Mini 5+ retransmit burst
- [x] Correlate retransmitted host blocks with USB bus traffic
- [x] Implement USB-CDC early-ACK fork test: send the ACK for a valid block
  before dispatching the block's commands, and flush USB-IN immediately
- [x] Revert USB-CDC early-ACK fork test after it reproduced rapid
  `mcu_rt` growth
- [x] Copy the reverted `src/generic/usb_cdc.c` to the Pi, rebuild, and flash
  the Mini 5+ again so the bad early-ACK firmware is no longer active
- [x] Add lower-level atsamd Bulk-IN on-demand snapshot counters in
  `src/atsamd/usbserial.c`
- [x] Update `QUERY_USBCDC_DEBUG` helper output for the atsamd snapshot format
- [x] Copy atsamd snapshot build to Raspberry Pi, build, flash, and capture
  `QUERY_USBCDC_DEBUG RESET=0` during the next `mcu_rt` rise
- [x] Analyze atsamd Bulk-IN snapshot captured during `mcu_rt` growth
- [x] Add second atsamd Bulk-IN snapshot focused on opportunistic `TRFAIL1`
  sampling without enabling `TRFAIL1` interrupts
- [x] Build/flash `atsamd-trfail-20260510` and capture retransmit-phase
  snapshots
- [x] Analyze `atsamd-trfail-20260510` retransmit-phase snapshots
- [x] Add Bulk-OUT `STALL0` interrupt/snapshot instrumentation in
  `src/atsamd/usbserial.c`
- [x] Build/flash `atsamd-outstall-20260510` and test whether
  `stall0_count` rises with `mcu_rt`
- [x] Analyze first `atsamd-outstall-20260510` monitor output
- [x] Analyze extended `atsamd-outstall-20260510` monitor output
- [x] Add raw Bulk-OUT/Bulk-IN endpoint register snapshots and enable
  Bulk-OUT `STALL1` instrumentation
- [x] Build/flash `atsamd-epraw-20260510` and inspect `epout`/`epin` raw
  endpoint state during `mcu_rt` rise
- [x] Analyze first `atsamd-epraw-20260510` output
- [x] Analyze same-run `atsamd-epraw-20260510` usbmon plus monitor capture

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
- Firmware tests on the Pi were built with `arm-none-eabi-gcc 12.2.1`
  (`15:12.2.rel1-1`, 2022-12-05), host `gcc 12.2.0`, and GNU Make 4.3. The
  Mac has a newer local `arm-none-eabi-gcc 15.2.1`, but that is not the
  toolchain used for the tested Pi firmware builds unless explicitly copied or
  built there.
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
- usbmon capture from 2026-05-10 confirms the host OUT transfers for the
  retransmitted blocks complete quickly on the USB bus (roughly 95-524us), but
  the matching ACK sequence becomes visible on USB-IN only after about
  25.7-77.5ms for nearly all correlated retransmit blocks. This rules out
  Linux userspace/TTY read latency as the primary cause for those samples and
  points below Klipper host code: Mini 5+ USB device side, USB controller
  scheduling, endpoint service timing, or the Pi/USB transport path.
- The next fork test is USB-CDC early ACK. The ACK is now generated and pushed
  to USB-IN immediately after a valid message block is parsed, before
  `command_dispatch()` executes the block. This intentionally changes the
  normal ordering for USB-CDC only: the host may receive credit before all
  commands in the block have finished dispatching. The receive window remains
  conservative so the host still cannot build a deep backlog.
- Early ACK is rejected. Test print immediately reproduced continuous
  `mcu_rt` growth (`9 -> 938` within about one minute), so the local source was
  reverted to the prior receive-drain/fast-flush behavior without early ACK.
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
- Clean production diff later showed a short recovered retransmit burst:
  - `pt=1769.001 mcu_rt=70 rtseq=104973 rto=0.025`
  - `pt=1771.811 mcu_rt=192 rtseq=105344 rto=0.100`
  - `pt=1780.785 mcu_rt=315 rtseq=106093 rto=0.025`
  - Interpretation: the receive-side patch appears to mitigate the previous
    runaway retransmit storm, but it is not a complete fix. The remaining
    events are still consistent with occasional timeout-driven late ACKs.
    Additional evidence is needed before committing this as final.
- Added ACK-prioritization refinement to the clean production diff:
  - After `usb_bulk_out_task()` successfully dispatches one buffered command
    block, it now stops the current OUT task run and self-wakes if needed.
  - Rationale: `command_find_and_dispatch()` queues an ACK after dispatch; by
    returning sooner, the USB IN task can send that ACK before more OUT
    draining happens in the same task run.
  - Still keeps the conservative receive window and bounded read loop.
- ACK-prioritization alone did not prevent escalation:
  - `pt=1593.366 mcu_rt=6272 seq=120907/120907 rtseq=120905 rto=0.025`
  - `pt=1612.238 mcu_rt=8071 seq=122603/122603 rtseq=122536 rto=0.025`
  - The print then shut down with `Timer too close`.
  - Interpretation: the remaining Mini 5+ USB issue can still starve motion
    command delivery enough to hit the MCU timer deadline even though
    `buffer_time` stayed near ~0.9-1.0s immediately before shutdown.
- Added fork-specific host throttle in `klippy/chelper/serialqueue.c`:
  - For transports advertising a small `RECEIVE_WINDOW` (`<= MESSAGE_MAX`),
    limit host message blocks to 32 bytes instead of 64.
  - This keeps the existing stop-and-wait behavior from `RECEIVE_WINDOW=64`
    but increases ACK frequency and reduces how much `queue_step` traffic is
    affected by any one late ACK.
  - Large single commands are still allowed up to the normal 64-byte protocol
    maximum to avoid deadlocking config or uncommon commands.
- Added ACK-fast-flush in `src/generic/usb_cdc.c`:
  - Factored the USB bulk IN send path into `usb_bulk_in_send()`.
  - After `usb_bulk_out_task()` successfully dispatches a buffered command
    block, it now immediately tries to send queued USB-CDC response data
    (normally the ACK) before returning.
  - If the USB IN endpoint is busy, the queued response remains pending and
    the normal `usb_bulk_in_task()` wake path still handles it.
  - Rationale: distinguish MCU ACK-generation/IN-handoff delay from deeper
    USB host/device transport delay and reduce time between dispatch and ACK
    endpoint handoff.
- Re-enabled host-side retransmit debug only, leaving firmware USB debug off:
  - Logs `serialqueue retransmit ...` with `cause`, `pending`, retransmitted
    bytes, `receive_window`, active `msg_max`, RTO state, and first pending
    message bytes.
  - Purpose: verify whether ACK-fast-flush plus 32-byte blocks still produces
    timeout retransmits, and whether retransmitted blocks are actually smaller
    under the fork throttle.
- `output.md` retransmit debug analysis:
  - Captured retransmits are still `cause=timeout`, usually `pending=1`, with
    `receive_window=64`.
  - Retransmitted blocks are still mostly 59-64 bytes (`bytes=59..64`,
    `first=59..64`) and the lines do not contain `msg_max=32`.
  - Interpretation: this capture was not from the current host helper build
    containing the `msg_max` debug field and 32-byte throttle. It shows the
    previous behavior: one full-sized outstanding block timing out, not the new
    small-block throttle behavior.
- Updated `output.md` analysis after the current host helper was active:
  - Retransmits are still `cause=timeout`, not NAK fast retransmits.
  - `pending=1`, `receive_window=64`, and `msg_max=32` are confirmed.
  - Retransmitted blocks are now small (`bytes=27..33`, `first=27..33`), so
    the 32-byte host throttle is working.
  - Some blocks still retransmit twice (`rto=0.025` then `rto=0.050`) before
    recovery.
  - Interpretation: the remaining issue is not host overfeeding the MCU and
    not oversized message blocks. The host is effectively stop-and-wait on one
    small block, but the ACK for that block is still not visible within the
    25ms RTO. The next useful evidence is USB-level timing via usbmon.
- Historical RepRapFirmware SD-card/printing hiccups on Duet 3 Mini/Mini 5+
  may be relevant as a class of timing/peripheral-latency issue, but not as
  evidence that USB and SD share the same external bus. In Klipper the board is
  not streaming G-code from the Duet SD card, so this is mainly useful as
  context for SAME54/Mini timing sensitivity.
- The next atsamd measurement should stay read-only/observational: count
  Bulk-IN Bank 1 busy returns, successful Bulk-IN armings, Bulk-IN `TRCPT1`
  completions, and the maximum time from arming `BK1RDY` to the next `TRCPT1`.
  Do not change USB interrupt service order until this snapshot is captured.
- atsamd Bulk-IN snapshot during `mcu_rt` growth is negative for the
  BK1RDY-to-TRCPT1 delay hypothesis:
  - `mcu_rt` rose from `9` to `1789`.
  - `bulk_in_armed` and `trcpt1_count` stayed exactly equal in every captured
    snapshot (`162741/162741` through `187011/187011`).
  - `max_bk1rdy_to_trcpt1_us` stayed flat at `151us`, far below the
    25ms/50ms retransmit timeouts.
  - `bulk_in_busy_returns` rose only slowly (`436 -> 499`) across roughly
    24k additional Bulk-IN armings.
  - Interpretation: the USB device sees Bulk-IN Bank 1 complete promptly; the
    remaining ACK invisibility is not explained by BK1RDY remaining busy or
    missing TRCPT1 completions.
  - Notable open clue: snapshots consistently show `epintflag=8`, which maps
    to Bulk-IN `TRFAIL1` in the SAME54 headers. Determine whether this is
    normal/sticky for this controller or a meaningful failure signal before
    changing endpoint service behavior.
- Do not enable Bulk-IN `TRFAIL1` interrupts. On SAME54 USB-CDC Bulk-IN this
  may fire for normal idle NAK/poll behavior and risks masking the timing bug.
  Instead, sample and clear sticky `TRFAIL1` opportunistically at two existing
  execution points:
  - Bulk-IN handler entry.
  - `usb_send_bulk_in()` busy branch when `BK1RDY` is already set.
- Interpret `trfail1_seen_count` and `trfail1_seen_busy_count` as sticky
  observations at sample points, not as exact hardware event rates.
- For the next run, capture an idle baseline before printing: run
  `QUERY_USBCDC_DEBUG RESET=1` three times around 30 seconds apart, then use
  the retransmit monitor during the print. Compare `trfail1_seen_count`,
  `trfail1_seen_busy_count`, `trfail1_while_armed`, and
  `max_bk1rdy_to_trcpt1_had_trfail1` against that baseline.
- `atsamd-trfail-20260510` retransmit-phase result is negative for the
  TRFAIL1-during-armed-transfer hypothesis:
  - During `mcu_rt` growth from `9` to at least `4616`,
    `bulk_in_armed`, `bulk_in_handler_count`, and `trcpt1_count` stayed equal
    at every snapshot.
  - `max_bk1rdy_to_trcpt1_us` stayed at `149us`.
  - `max_bk1rdy_to_trcpt1_had_trfail1=0`, `trfail1_while_armed=0`,
    `cur_trfail1=0`, and `active=0` in every retransmit-phase snapshot.
  - `trfail1_seen_count` tracks nearly every Bulk-IN handler sample, which is
    consistent with normal/sticky idle NAK observation.
  - `trfail1_seen_busy_count` rises slowly but never marks the current or max
    armed transfer as having seen TRFAIL1 while `BK1RDY` remained set.
  - Interpretation: TRFAIL1 is most likely expected idle/sticky noise in this
    workload, not the cause of delayed ACK visibility. The remaining issue is
    outside the measured atsamd Bulk-IN bank ready/completion/error flag path.
- usbmon correlation changed the leading hypothesis: the original Bulk-OUT
  transfers for retransmitted blocks appear to complete with status `-32`
  (`-EPIPE`), which means the device sent a USB STALL handshake. That is a
  stronger explanation than late ACK generation: the MCU never accepted the
  original OUT block, so the host waits for RTO and retransmits.
- Next measurement is Bulk-OUT `STALL0`, not more Bulk-IN diagnostics. Enable
  only `USB_DEVICE_EPINTENSET_STALL0` on `EP_BULKOUT`; do not enable `STALL1`
  unless `STALL0` fails to correlate. In the ISR, only count the event, capture
  the first-stall snapshot, clear the interrupt flag, and leave endpoint state
  untouched.
- First `atsamd-outstall-20260510` monitor output is not enough to confirm the
  usbmon `-EPIPE` interpretation from the MCU side:
  - During `mcu_rt` growth from `9` to `459`, every snapshot reported
    `outstall stall0_count=0 first_valid=0`.
  - This conflicts with the prior usbmon interpretation if the same failure
    mode was active, because a Bulk-OUT STALL0 interrupt should have produced
    a nonzero count.
  - Possible explanations: this run did not include a usbmon-confirmed
    `-EPIPE`, the SAME54 condition that Linux reports as `-EPIPE` does not set
    `EPINTFLAG.STALL0`, the relevant interrupt is not `STALL0`, or the current
    instrumentation is not sampling the right endpoint/bank condition.
  - Do not conclude from this output alone that the usbmon `-EPIPE` finding was
    wrong. Next evidence should be from the same run: usbmon plus
    `outstall_monitor.log`, or broaden the instrumentation to include `STALL1`
    and raw Bulk-OUT endpoint status snapshots on query.
- Extended `atsamd-outstall-20260510` monitor output repeated the same negative
  MCU-side result over a larger retransmit rise:
  - `mcu_rt` rose from `459` to `1257`.
  - `outstall stall0_count=0 first_valid=0` in every snapshot.
  - Therefore, this firmware did not observe a Bulk-OUT `STALL0` interrupt
    during the monitored retransmit rise. If a same-run usbmon trace still
    shows `C Bo -32`, the device-side signal is not `EPINTFLAG.STALL0` as
    currently instrumented.
- Next raw endpoint snapshot broadens the observation without changing endpoint
  state:
  - Build marker: `USB_CDC_DEBUG_BUILD=atsamd-epraw-20260510`.
  - Bulk-OUT now enables both `STALL0` and `STALL1` interrupt bits, but the
    ISR still only counts/snapshots/clears the interrupt flag.
  - `QUERY_USBCDC_DEBUG` now emits `epout ...` and `epin ...` raw register
    lines containing `EPCFG`, `EPINTENSET`, `EPINTFLAG`, `EPSTATUS`, and both
    descriptor-bank `PCKSIZE` values. This verifies whether `STALL0`/`STALL1`
    are actually enabled, whether sticky flags are present at query time, and
    whether bank descriptor state looks abnormal during retransmit growth.
- First `atsamd-epraw-20260510` output verifies the instrumentation and is
  negative for MCU-observed Bulk-OUT STALL flags during the sampled rise:
  - `mcu_rt` rose from `9` to `102`.
  - `epout epintenset=99` (`0x63`) confirms `TRCPT0`, `TRCPT1`, `STALL0`,
    and `STALL1` interrupts are enabled on Bulk-OUT.
  - `outstall stall0_count=0 stall1_count=0 first_valid=0`.
  - `epout epintflag=0 epstatus=0`, so no sticky `STALL0/STALL1`, no
    `STALLRQ0/1`, no `BK0RDY/BK1RDY`, and no abnormal status bits were present
    at query time.
  - Bulk-OUT Bank0 `PCKSIZE` values decoded as size class 3 (64-byte endpoint)
    with current byte counts `14` and `7`; Multi Packet Size was zero.
  - Interpretation: this confirms the firmware instrumentation is enabled, but
    it still does not observe a SAME54 Bulk-OUT STALL condition during the
    sampled retransmit rise. Same-run usbmon is now required to determine
    whether Linux still reports `C Bo -32` in this build/run.
- Same-run `atsamd-epraw-20260510` usbmon plus monitor capture corrects the
  `C Bo -32` interpretation:
  - The current usbmon trace contains exactly three `C Bo:1:057:2 -32`
    completions.
  - Each `-32` is immediately preceded by `S Co:1:002:0 s 23 08 ...`, a hub
    class control transfer to USB device `1:002`, not to the Mini 5+
    application device `1:057`.
  - `s 23 08` is the hub Transaction Translator clear-buffer path for a
    full-speed device behind a high-speed hub. Therefore the host error path is
    hub/TT related, not a directly observed SAME54 endpoint `STALL0/STALL1`
    interrupt/status condition.
  - In the same time window, the MCU monitor still reports
    `stall0_count=0 stall1_count=0 first_valid=0`, with `epout epintflag=0`
    and `epout epstatus=0` in the available snapshots.
  - The retransmit timing remains the same: the failed OUT URB completes with
    `-32` roughly 431-486us after submit, and Klipper resubmits the same block
    about 25.2-25.7ms later.
  - New leading direction: verify and simplify the physical USB topology. Test
    the Mini 5+ directly on a Pi root port or a different high-quality hub/cable
    path, and capture `lsusb -t` to see whether the Mini 5+ and EBB devices are
    sharing the same Transaction Translator.

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
  - Read the current `PLANS.md` handoff and reviewed
    `src/atsamd/usbserial.c`, `src/generic/usb_cdc.c`,
    `klippy/extras/usbcdc_debug.py`, and `scripts/monitor_usbcdc_debug.py`.
  - Added on-demand lower-level atsamd Bulk-IN snapshot counters in
    `src/atsamd/usbserial.c`:
    `bulk_in_busy_returns`, `bulk_in_armed`, `trcpt1_count`,
    `max_bk1rdy_to_trcpt1_us`, active state, endpoint status, endpoint
    interrupt flags, and Bulk-IN descriptor `PCKSIZE`.
  - Added `USB_CDC_DEBUG_BUILD=atsamd-intrcpt-20260510`.
  - Reused the existing MCU command name `get_usbcdc_debug reset=%c`, now
    returning `usbcdc_debug_atsamd ...` for this atsamd-only measurement.
  - Updated `klippy/extras/usbcdc_debug.py` so `QUERY_USBCDC_DEBUG RESET=0`
    can display either the new atsamd snapshot or the older generic
    `usbcdc_debug_dispatch` snapshot.
  - Updated `scripts/monitor_usbcdc_debug.py` so the automatic retransmit
    monitor prints `atsamd_usb ...` responses.
  - Ran Python bytecode checks for `klippy/extras/usbcdc_debug.py` and
    `scripts/monitor_usbcdc_debug.py`.
  - Ran `git diff --check` for `src/atsamd/usbserial.c`,
    `klippy/extras/usbcdc_debug.py`, and `scripts/monitor_usbcdc_debug.py`.
  - Reviewed `output.md` captured from the Pi during a retransmit escalation.
  - Classified the atsamd Bulk-IN snapshot as negative for the
    BK1RDY-to-TRCPT1-delay hypothesis: `bulk_in_armed == trcpt1_count`
    throughout, `max_bk1rdy_to_trcpt1_us=151`, and `bulk_in_busy_returns`
    rose only slowly while `mcu_rt` climbed.
  - Added the second atsamd snapshot version:
    `USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510`.
  - Added `bulk_in_handler_count`, `trfail1_seen_count`,
    `trfail1_seen_busy_count`, `trfail1_while_armed`, `cur_trfail1`, and
    `max_bk1rdy_to_trcpt1_had_trfail1`.
  - Sampled and cleared sticky Bulk-IN `TRFAIL1` at Bulk-IN handler entry and
    in the Bulk-IN busy branch only; did not enable `TRFAIL1` interrupts.
  - Updated `klippy/extras/usbcdc_debug.py` for the expanded
    `usbcdc_debug_atsamd` response.
  - Ran Python bytecode checks and `git diff --check` for the changed debug
    files.
  - Reviewed the new `output.md` from the Pi with
    `USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510`.
  - Classified the TRFAIL1-focused measurement as negative: `trfail1_while_armed`
    and `max_bk1rdy_to_trcpt1_had_trfail1` stayed zero while `mcu_rt` rose.
  - Added `USB_CDC_DEBUG_BUILD=atsamd-outstall-20260510`.
  - Enabled Bulk-OUT `STALL0` interrupt instrumentation only.
  - Added `usbcdc_debug_outstall` response with `stall0_count` and the first
    STALL snapshot: `first_epstatus`, `first_epintflag`, `first_epcfg`, and
    Bulk-OUT Bank0 `first_pcksize`.
  - Updated `klippy/extras/usbcdc_debug.py` and
    `scripts/monitor_usbcdc_debug.py` so `QUERY_USBCDC_DEBUG` prints the
    `outstall ...` line.
  - Ran Python bytecode checks and `git diff --check` for the changed debug
    files.
  - Reviewed `output2.md` from the first `atsamd-outstall-20260510` test.
    It showed `stall0_count=0` while `mcu_rt` rose from `9` to `459`.
  - Reviewed `output3.md`, which extended the same result:
    `stall0_count=0` while `mcu_rt` rose from `459` to `1257`.
  - Added `USB_CDC_DEBUG_BUILD=atsamd-epraw-20260510`.
  - Added Bulk-OUT `STALL1` enable/counting alongside `STALL0`.
  - Added raw endpoint responses:
    - `usbcdc_debug_epout epcfg=... epintenset=... epintflag=...
      epstatus=... pck0=... pck1=...`
    - `usbcdc_debug_epin epcfg=... epintenset=... epintflag=...
      epstatus=... pck0=... pck1=...`
  - Updated `klippy/extras/usbcdc_debug.py` and
    `scripts/monitor_usbcdc_debug.py` for the new raw endpoint lines.
  - Ran Python bytecode checks and `git diff --check` for the changed debug
    files.
  - Reviewed `output4.md`; `epout epintenset=99` confirmed Bulk-OUT
    `STALL0|STALL1` instrumentation was enabled, but `stall0_count=0`,
    `stall1_count=0`, `epout epintflag=0`, and `epout epstatus=0` during
    `mcu_rt` growth.
  - Reviewed same-run `usbmon_epraw_run.txt.gz`,
    `usbmon_epraw_epipe.txt.gz`, and `epraw_monitor.log.gz`.
  - Found exactly three current `C Bo:1:057:2 -32` completions, each
    immediately preceded by `S Co:1:002:0 s 23 08 ...` to the upstream USB hub,
    while the MCU-side monitor still reported `stall0_count=0` and
    `stall1_count=0`.
  - Reclassified the current `-32` evidence from "direct device endpoint STALL"
    to a hub Transaction Translator clear-buffer/error path for a full-speed
    device behind a high-speed hub.
- Stopped at:
  - Raw endpoint instrumentation is active and did not observe MCU-side
    Bulk-OUT STALL flags, while same-run usbmon shows hub/TT control transfers
    immediately before each current `C Bo -32`.
- Next step:
  - Capture the Pi USB topology with `lsusb -t` and test a physical topology
    change: connect the Mini 5+ directly to a Pi root port, or move it to a
    different high-quality hub/cable path, then repeat the same print/monitor
    check to see whether hub/TT `s 23 08` plus `C Bo -32` disappears.
- Open blockers:
  - Need USB topology details and one direct-port/different-hub A/B test to
    determine whether the remaining failure is caused by the current hub/TT
    path rather than the SAME54 USB device-layer code.
- Decisions made this session:
  - Instrument first and keep the next patch observational only. Do not change
    USB interrupt service order until the Bulk-IN BK1RDY-to-TRCPT1 snapshot is
    captured.
  - Include `bulk_in_armed` as a denominator even though the requested minimal
    counters were `bulk_in_busy_returns` and `trcpt1_count`; otherwise
    `trcpt1_count` is hard to interpret.
  - Treat current same-run `C Bo -32` as hub/Transaction-Translator related
    until topology testing proves otherwise; it does not correspond to the
    SAME54 `STALL0/STALL1` instrumentation in this build.

## Notes
- `PLANS-entwurf.md` remains as the original investigation draft.
- This file is the active session log.
