voron@voron24:~/klipper $ python3 scripts/monitor_usbcdc_debug.py --command "QUERY_USBCDC_DEBUG RESET=0" --min-interval 5 | tee ~/outstall_monitor.log
connected to Klippy webhook socket /home/voron/printer_data/comms/klippy.sock
monitoring /home/voron/printer_data/logs/klippy.log for mcu: bytes_retransmit changes; command='QUERY_USBCDC_DEBUG RESET=0'
21:15:11 mcu_rt changed 9 -> 71; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-outstall-20260510
// outstall stall0_count=0 first_valid=0 first_epstatus=0 first_epintflag=0 first_epcfg=0 first_pcksize=0
// atsamd_usb bulk_in_busy_returns=308 bulk_in_armed=125358 bulk_in_handler_count=125357 trcpt1_count=125357 max_bk1rdy_to_trcpt1_us=242921 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=124814 trfail1_seen_busy_count=236 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306373
21:15:15 mcu_rt changed 71 -> 101; skipped due to min interval
21:15:28 mcu_rt changed 101 -> 131; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-outstall-20260510
// outstall stall0_count=0 first_valid=0 first_epstatus=0 first_epintflag=0 first_epcfg=0 first_pcksize=0
// atsamd_usb bulk_in_busy_returns=313 bulk_in_armed=128767 bulk_in_handler_count=128766 trcpt1_count=128766 max_bk1rdy_to_trcpt1_us=242921 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=128215 trfail1_seen_busy_count=240 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
21:15:28 mcu_rt changed 131 -> 161; skipped due to min interval
21:15:32 mcu_rt changed 161 -> 215; skipped due to min interval
21:15:34 mcu_rt changed 215 -> 275; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-outstall-20260510
// outstall stall0_count=0 first_valid=0 first_epstatus=0 first_epintflag=0 first_epcfg=0 first_pcksize=0
// atsamd_usb bulk_in_busy_returns=314 bulk_in_armed=130179 bulk_in_handler_count=130178 trcpt1_count=130178 max_bk1rdy_to_trcpt1_us=242921 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=129625 trfail1_seen_busy_count=241 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306384
21:15:36 mcu_rt changed 275 -> 339; skipped due to min interval
21:15:39 mcu_rt changed 339 -> 399; skipped due to min interval
21:15:41 mcu_rt changed 399 -> 459; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-outstall-20260510
// outstall stall0_count=0 first_valid=0 first_epstatus=0 first_epintflag=0 first_epcfg=0 first_pcksize=0
// atsamd_usb bulk_in_busy_returns=317 bulk_in_armed=131629 bulk_in_handler_count=131628 trcpt1_count=131628 max_bk1rdy_to_trcpt1_us=242921 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=131068 trfail1_seen_busy_count=244 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306383
