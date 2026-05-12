voron@voron24:~/klipper $ python3 scripts/monitor_usbcdc_debug.py --command "QUERY_USBCDC_DEBUG RESET=0" --min-interval 5 | tee ~/epraw_monitor.log
connected to Klippy webhook socket /home/voron/printer_data/comms/klippy.sock
monitoring /home/voron/printer_data/logs/klippy.log for mcu: bytes_retransmit changes; command='QUERY_USBCDC_DEBUG RESET=0'
21:38:40 mcu_rt changed 9 -> 38; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-epraw-20260510
// outstall stall0_count=0 stall1_count=0 first_valid=0 first_epstatus=0 first_epintflag=0 first_epcfg=0 first_pcksize=0
// epout epcfg=3 epintenset=99 epintflag=0 epstatus=0 pck0=805306382 pck1=0
// epin epcfg=48 epintenset=3 epintflag=8 epstatus=6 pck0=0 pck1=805306373
// atsamd_usb bulk_in_busy_returns=453 bulk_in_armed=107918 bulk_in_handler_count=107917 trcpt1_count=107917 max_bk1rdy_to_trcpt1_us=761218 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=107190 trfail1_seen_busy_count=328 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=6 epintflag=8 pcksize=805306373
21:39:19 mcu_rt changed 38 -> 102; sending QUERY_USBCDC_DEBUG RESET=0
// USB_CDC_DEBUG_BUILD=atsamd-epraw-20260510
// outstall stall0_count=0 stall1_count=0 first_valid=0 first_epstatus=0 first_epintflag=0 first_epcfg=0 first_pcksize=0
// epout epcfg=3 epintenset=99 epintflag=0 epstatus=0 pck0=805306375 pck1=0
// epin epcfg=48 epintenset=3 epintflag=8 epstatus=4 pck0=0 pck1=805306373
// atsamd_usb bulk_in_busy_returns=502 bulk_in_armed=116101 bulk_in_handler_count=116100 trcpt1_count=116100 max_bk1rdy_to_trcpt1_us=761218 max_bk1rdy_to_trcpt1_had_trfail1=0 trfail1_seen_count=115300 trfail1_seen_busy_count=361 trfail1_while_armed=0 cur_trfail1=0 active=0 epstatus=4 epintflag=8 pcksize=805306373

