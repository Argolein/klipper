voron@voron24:~/klipper $ query_usbcdc 'QUERY_USBCDC_DEBUG RESET=1'
sleep 30
query_usbcdc 'QUERY_USBCDC_DEBUG RESET=1'
sleep 30
query_usbcdc 'QUERY_USBCDC_DEBUG RESET=1'
B:29.3 /29.0 T0:204.0 /200.0
B:29.3 /29.0 T0:204.2 /200.0
B:29.3 /29.0 T0:204.1 /200.0
B:29.3 /29.0 T0:204.0 /200.0
B:29.4 /29.0 T0:203.9 /200.0
^C
B:29.4 /29.0 T0:203.6 /200.0
B:29.3 /29.0 T0:203.1 /200.0
B:29.3 /29.0 T0:202.7 /200.0
B:29.4 /29.0 T0:202.3 /200.0
B:29.4 /29.0 T0:201.8 /200.0
B:29.3 /29.0 T0:201.5 /200.0
^C
B:29.2 /29.0 T0:200.7 /200.0
B:29.2 /29.0 T0:200.4 /200.0
^CTraceback (most recent call last):
  File "<stdin>", line 26, in <module>
KeyboardInterrupt



python3 scripts/monitor_usbcdc_debug.py --command "QUERY_USBCDC_DEBUG RESET=0" --min-interval 5
connected to Klippy webhook socket /home/voron/printer_data/comms/klippy.sock
monitoring /home/voron/printer_data/logs/klippy.log for mcu: bytes_retransmit changes; command='QUERY_USBCDC_DEBUG RESET=0'
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=76 bulk_in_armed=28577 bulk_in_handler_count=28577 trcpt1_count=28577 max_bk1rdy_to_trcpt1_us=105 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=28451 trfail1_seen_busy_count=57 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306373
// counters reset after snapshot
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=0 bulk_in_armed=1 bulk_in_handler_count=1 trcpt1_count=1 max_bk1rdy_to_trcpt1_us=43 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=1 trfail1_seen_busy_count=0 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306407
// counters reset after snapshot
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=0 bulk_in_armed=1 bulk_in_handler_count=1 trcpt1_count=1 max_bk1rdy_to_trcpt1_us=34 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=1 trfail1_seen_busy_count=0 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306398
// counters reset after snapshot
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=0 bulk_in_armed=1 bulk_in_handler_count=1 trcpt1_count=1 max_bk1rdy_to_trcpt1_us=39 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=1 trfail1_seen_busy_count=0 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306398
// counters reset after snapshot
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=0 bulk_in_armed=1 bulk_in_handler_count=1 trcpt1_count=1 max_bk1rdy_to_trcpt1_us=40 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=1 trfail1_seen_busy_count=0 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306398
// counters reset after snapshot
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=0 bulk_in_armed=1 bulk_in_handler_count=1 trcpt1_count=1 max_bk1rdy_to_trcpt1_us=40 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=1 trfail1_seen_busy_count=0 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306398
// counters reset after snapshot
20:32:56 mcu_rt changed 9 -> 71; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=300 bulk_in_armed=112849 bulk_in_handler_count=112849 trcpt1_count=112849 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=112360 trfail1_seen_busy_count=225 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306373
20:35:29 mcu_rt changed 71 -> 99; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=345 bulk_in_armed=144758 bulk_in_handler_count=144758 trcpt1_count=144758 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=144189 trfail1_seen_busy_count=264 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306373
20:35:34 mcu_rt changed 99 -> 114; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=346 bulk_in_armed=145672 bulk_in_handler_count=145672 trcpt1_count=145672 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=145100 trfail1_seen_busy_count=265 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306373
20:37:21 mcu_rt changed 114 -> 178; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=439 bulk_in_armed=168498 bulk_in_handler_count=168498 trcpt1_count=168498 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=167783 trfail1_seen_busy_count=330 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
20:37:25 mcu_rt changed 178 -> 234; skipped due to min interval
20:37:42 mcu_rt changed 234 -> 261; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=451 bulk_in_armed=173264 bulk_in_handler_count=173264 trcpt1_count=173264 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=172530 trfail1_seen_busy_count=339 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
20:37:51 mcu_rt changed 261 -> 293; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=459 bulk_in_armed=175250 bulk_in_handler_count=175250 trcpt1_count=175250 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=174504 trfail1_seen_busy_count=345 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
20:37:57 mcu_rt changed 293 -> 353; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=460 bulk_in_armed=176511 bulk_in_handler_count=176511 trcpt1_count=176511 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=175763 trfail1_seen_busy_count=346 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306383
20:37:59 mcu_rt changed 353 -> 414; skipped due to min interval
20:38:00 mcu_rt changed 414 -> 445; skipped due to min interval
20:38:01 mcu_rt changed 445 -> 476; skipped due to min interval
20:38:04 mcu_rt changed 476 -> 536; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=462 bulk_in_armed=178301 bulk_in_handler_count=178301 trcpt1_count=178301 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=177551 trfail1_seen_busy_count=347 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306386
20:38:05 mcu_rt changed 536 -> 566; skipped due to min interval
20:38:08 mcu_rt changed 566 -> 598; skipped due to min interval
20:38:12 mcu_rt changed 598 -> 627; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=467 bulk_in_armed=179995 bulk_in_handler_count=179995 trcpt1_count=179995 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=179238 trfail1_seen_busy_count=350 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306373
20:38:17 mcu_rt changed 627 -> 659; skipped due to min interval
20:38:18 mcu_rt changed 659 -> 719; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=468 bulk_in_armed=181227 bulk_in_handler_count=181227 trcpt1_count=181227 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=180467 trfail1_seen_busy_count=351 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306383
20:38:21 mcu_rt changed 719 -> 748; skipped due to min interval
20:38:22 mcu_rt changed 748 -> 814; skipped due to min interval
20:38:23 mcu_rt changed 814 -> 844; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=469 bulk_in_armed=182476 bulk_in_handler_count=182476 trcpt1_count=182476 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=181714 trfail1_seen_busy_count=352 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
20:38:24 mcu_rt changed 844 -> 874; skipped due to min interval
20:38:25 mcu_rt changed 874 -> 936; skipped due to min interval
20:38:26 mcu_rt changed 936 -> 966; skipped due to min interval
20:38:27 mcu_rt changed 966 -> 1084; skipped due to min interval
20:38:29 mcu_rt changed 1084 -> 1144; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=470 bulk_in_armed=183738 bulk_in_handler_count=183738 trcpt1_count=183738 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=182974 trfail1_seen_busy_count=353 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306373
20:38:32 mcu_rt changed 1144 -> 1176; skipped due to min interval
20:38:33 mcu_rt changed 1176 -> 1235; skipped due to min interval
20:38:34 mcu_rt changed 1235 -> 1265; skipped due to min interval
20:38:35 mcu_rt changed 1265 -> 1363; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=471 bulk_in_armed=185065 bulk_in_handler_count=185065 trcpt1_count=185065 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=184299 trfail1_seen_busy_count=354 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306373
20:38:36 mcu_rt changed 1363 -> 1425; skipped due to min interval
20:38:38 mcu_rt changed 1425 -> 1455; skipped due to min interval
20:38:39 mcu_rt changed 1455 -> 1486; skipped due to min interval
20:38:40 mcu_rt changed 1486 -> 1516; skipped due to min interval
20:38:41 mcu_rt changed 1516 -> 1574; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=475 bulk_in_armed=186433 bulk_in_handler_count=186433 trcpt1_count=186433 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=185661 trfail1_seen_busy_count=357 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306373
20:38:42 mcu_rt changed 1574 -> 1695; skipped due to min interval
20:38:43 mcu_rt changed 1695 -> 1760; skipped due to min interval
20:38:44 mcu_rt changed 1760 -> 1820; skipped due to min interval
20:38:45 mcu_rt changed 1820 -> 1935; skipped due to min interval
20:41:05 mcu_rt changed 1935 -> 1999; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=590 bulk_in_armed=217881 bulk_in_handler_count=217881 trcpt1_count=217881 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=216935 trfail1_seen_busy_count=434 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306373
20:41:31 mcu_rt changed 1999 -> 2029; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=599 bulk_in_armed=223541 bulk_in_handler_count=223541 trcpt1_count=223541 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=222578 trfail1_seen_busy_count=442 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306383
20:41:40 mcu_rt changed 2029 -> 2044; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=600 bulk_in_armed=225433 bulk_in_handler_count=225433 trcpt1_count=225433 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=224468 trfail1_seen_busy_count=443 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306383
20:41:42 mcu_rt changed 2044 -> 2104; skipped due to min interval
20:41:48 mcu_rt changed 2104 -> 2134; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=603 bulk_in_armed=227238 bulk_in_handler_count=227238 trcpt1_count=227238 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=226269 trfail1_seen_busy_count=445 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
20:41:50 mcu_rt changed 2134 -> 2194; skipped due to min interval
20:41:53 mcu_rt changed 2194 -> 2227; skipped due to min interval
20:42:05 mcu_rt changed 2227 -> 2318; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=608 bulk_in_armed=230969 bulk_in_handler_count=230969 trcpt1_count=230969 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=229994 trfail1_seen_busy_count=448 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306384
20:42:06 mcu_rt changed 2318 -> 2382; skipped due to min interval
20:42:07 mcu_rt changed 2382 -> 2442; skipped due to min interval
20:42:09 mcu_rt changed 2442 -> 2502; skipped due to min interval
20:42:11 mcu_rt changed 2502 -> 2532; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=611 bulk_in_armed=232290 bulk_in_handler_count=232290 trcpt1_count=232290 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=231310 trfail1_seen_busy_count=450 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306373
20:42:13 mcu_rt changed 2532 -> 2562; skipped due to min interval
20:42:15 mcu_rt changed 2562 -> 2647; skipped due to min interval
20:42:16 mcu_rt changed 2647 -> 2707; skipped due to min interval
20:42:18 mcu_rt changed 2707 -> 2736; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=612 bulk_in_armed=233827 bulk_in_handler_count=233827 trcpt1_count=233827 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=232845 trfail1_seen_busy_count=451 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306383
20:42:21 mcu_rt changed 2736 -> 2766; skipped due to min interval
20:42:22 mcu_rt changed 2766 -> 2858; skipped due to min interval
20:42:24 mcu_rt changed 2858 -> 2920; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=614 bulk_in_armed=235184 bulk_in_handler_count=235184 trcpt1_count=235184 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=234198 trfail1_seen_busy_count=453 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
20:42:25 mcu_rt changed 2920 -> 3018; skipped due to min interval
20:42:28 mcu_rt changed 3018 -> 3084; skipped due to min interval
20:42:30 mcu_rt changed 3084 -> 3114; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=616 bulk_in_armed=236450 bulk_in_handler_count=236450 trcpt1_count=236450 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=235460 trfail1_seen_busy_count=455 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
20:42:31 mcu_rt changed 3114 -> 3168; skipped due to min interval
20:42:32 mcu_rt changed 3168 -> 3234; skipped due to min interval
20:42:33 mcu_rt changed 3234 -> 3327; skipped due to min interval
20:42:34 mcu_rt changed 3327 -> 3445; skipped due to min interval
20:42:38 mcu_rt changed 3445 -> 3528; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=618 bulk_in_armed=238226 bulk_in_handler_count=238226 trcpt1_count=238226 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=237232 trfail1_seen_busy_count=457 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306373
20:42:42 mcu_rt changed 3528 -> 3621; skipped due to min interval
20:42:43 mcu_rt changed 3621 -> 3681; skipped due to min interval
20:42:44 mcu_rt changed 3681 -> 3713; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=621 bulk_in_armed=239536 bulk_in_handler_count=239536 trcpt1_count=239536 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=238538 trfail1_seen_busy_count=459 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306373
20:42:45 mcu_rt changed 3713 -> 3743; skipped due to min interval
20:42:47 mcu_rt changed 3743 -> 3801; skipped due to min interval
20:42:49 mcu_rt changed 3801 -> 3929; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=623 bulk_in_armed=240639 bulk_in_handler_count=240639 trcpt1_count=240639 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=239636 trfail1_seen_busy_count=461 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306386
20:42:50 mcu_rt changed 3929 -> 4140; skipped due to min interval
20:42:51 mcu_rt changed 4140 -> 4196; skipped due to min interval
20:42:52 mcu_rt changed 4196 -> 4253; skipped due to min interval
20:42:53 mcu_rt changed 4253 -> 4283; skipped due to min interval
20:42:54 mcu_rt changed 4283 -> 4349; skipped due to min interval
20:42:56 mcu_rt changed 4349 -> 4375; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=626 bulk_in_armed=242146 bulk_in_handler_count=242146 trcpt1_count=242146 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=241137 trfail1_seen_busy_count=464 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306373
20:42:57 mcu_rt changed 4375 -> 4437; skipped due to min interval
20:42:58 mcu_rt changed 4437 -> 4490; skipped due to min interval
20:46:08 mcu_rt changed 4490 -> 4550; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-trfail-20260510
// atsamd_usb bulk_in_busy_returns=734 bulk_in_armed=282879 bulk_in_handler_count=282879 trcpt1_count=282879 max_bk1rdy_to_trcpt1_us=149 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=281697 trfail1_seen_busy_count=545 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306383
20:46:13 mcu_rt changed 4550 -> 4616; skipped due to min interval
