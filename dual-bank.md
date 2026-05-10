# dual-bank.md

Implementierungsplan: Dual-Banked Bulk-Endpoints im atsamd USB-Treiber
zur Behebung des Mini 5+ USB-CDC `bytes_retransmit`-Issues.

Bezug: `PLANS.md` (Investigation-Log). Dieser Plan ist die konkrete
Umsetzung der dort identifizierten Architektur-Hypothese (Single-Bank
Bulk-Endpoints sind der Engpass, der die `in_busy`-Retries und die
Heisenbug-Sensitivität erzeugt).

---

## Ziel

`src/atsamd/usbserial.c` so umbauen, dass die Bulk-OUT- und Bulk-IN-
Endpoints beide HW-Bänke (Ping-Pong) der SAME54/SAMD51 USB-Peripherie
nutzen. Dadurch:

- `in_busy` Retries werden auf den Fall reduziert, dass beide IN-Bänke
  gleichzeitig auf Host-Polling warten (statt nach jeder einzelnen Bank).
- Bulk-OUT NAKs auf der Wire halbieren sich im Mittel.
- Die "Heisenbug"-Sensitivität (jede Code-/Layout-Änderung verschiebt das
  Failure-Window) sollte deutlich abnehmen, weil das System nicht mehr
  pro Paket am Ein-Puffer-Limit operiert.

Out-of-scope: EP0 und ACM-Endpoint bleiben Single-Bank.

---

## Pre-flight: Hypothese hart bestätigen vor dem Umbau

Bevor der Architekturumbau gemacht wird, muss bewiesen werden, dass die
fehlenden 25 ms tatsächlich im IN-busy-Pfad verloren gehen. Sonst löst
Dual-Bank das Problem nicht.

### Schritt 0a — Stuck-Busy Dauer messen

Lokale Erweiterung in `src/generic/usb_cdc.c` zu den bestehenden
On-Demand-Countern:

- Wenn `usb_send_bulk_in()` `-1` (busy) zurückgibt: aktuellen Timestamp
  in `usb_bulk_in_busy_start` festhalten, falls noch nicht gesetzt.
- Wenn `usb_send_bulk_in()` `>0` zurückgibt und `usb_bulk_in_busy_start`
  gesetzt war: `now - busy_start` berechnen, gegen `in_max_busy_us`
  vergleichen und maximieren. `busy_start` zurücksetzen.
- `in_max_busy_us` zur `get_usbcdc_debug` Antwort hinzufügen.

Ergänzungen: keine periodischen Outputs, kein neuer IRQ-Pfad. Nur ein
zusätzliches Feld in `struct usb_cdc_debug_s` und zwei Zeilen in
`usb_bulk_in_task()`.

### Schritt 0b — Reproduktionslauf mit dem Pre-flight-Build

Auf dem Pi:
1. Patch nach Pi kopieren, Firmware bauen, Mini 5+ flashen.
2. Normalen Druck starten.
3. Bei `mcu_rt`-Anstieg: `QUERY_USBCDC_DEBUG RESET=0` aufrufen.

### Schritt 0c — Entscheidung

- **`in_max_busy_us` >= 20000** (>=20 ms in einem einzelnen
  Stuck-Fenster) → Dual-Bank-Hypothese bestätigt. Mit Schritt 1 fortfahren.
- **`in_max_busy_us` < 5000** (alle Stucks bleiben unter 5 ms) →
  Dual-Bank löst das Hauptproblem nicht. Plan stoppen, anderen
  Pfad suchen (s. "Wenn die Pre-flight-Messung negativ ist" am Ende).
- Bereich dazwischen: Dual-Bank wahrscheinlich Teil-Verbesserung,
  aber nicht alleinige Ursache. Trotzdem fortfahren, aber Erwartung
  anpassen — `RECEIVE_WINDOW=64` müsste evtl. als Sicherheit bleiben.

---

## Architekturentscheidungen

### Bank-Reihenfolge: strikt FIFO

Klipper-Antworten dürfen **nicht** in falscher Reihenfolge auf der Wire
landen, sonst zerlegt es das Sequence-Tracking auf dem Host. Daher:
beide Bänke werden in fester Reihenfolge benutzt (Bank 0 → Bank 1 →
Bank 0 → …). Die HW alterniert automatisch DATA0/DATA1 — das ist
korrekt, solange wir die Banks in der richtigen Reihenfolge auf RDY
setzen.

Software hält pro Bulk-Endpoint einen `next_bank`-Index (0/1). Beim
Schreiben/Lesen wird **immer** diese Bank versucht. Wenn die nicht frei
ist, gibt der Aufruf `-1` zurück (busy). Es wird **nicht** auf die
andere Bank ausgewichen, weil das die Reihenfolge brechen würde.

### Wann ist eine Bank frei

- IN-Endpoint: Bank ist beschreibbar wenn `BKxRDY=0`. CPU schreibt,
  setzt `BKxRDY=1`, HW sendet, HW löscht `BKxRDY=0`.
- OUT-Endpoint: Bank ist lesbar wenn `BKxRDY=1`. HW schreibt, setzt
  `BKxRDY=1`, CPU liest, CPU löscht `BKxRDY=0`.

Diese Semantik ist heute schon in `usb_write_packet()` /
`usb_read_packet()` korrekt implementiert — nur eben jeweils nur für
eine Bank. Die Funktionen müssen so bleiben, der State liegt im
Aufrufer.

### Pufferlayout

Heute:
```c
static uint8_t __aligned(4) bulkout[USB_CDC_EP_BULK_OUT_SIZE];
static uint8_t __aligned(4) bulkin[USB_CDC_EP_BULK_IN_SIZE];
```

Neu:
```c
static uint8_t __aligned(4) bulkout[2][USB_CDC_EP_BULK_OUT_SIZE];
static uint8_t __aligned(4) bulkin [2][USB_CDC_EP_BULK_IN_SIZE];
```

`__aligned(4)` ist ausreichend (4-Byte für 32-bit Adressbus). Die
USB-Peripherie hat keine strikteren Alignment-Anforderungen für
Endpoint-Buffers (die `__aligned(16)` Tests betrafen `usb_desc`, nicht
die Datenpuffer).

### `usb_desc[]` Initialisierung

Beide `DeviceDescBank[0]` und `DeviceDescBank[1]` für `BULK_OUT` und
`BULK_IN` müssen befüllt sein:

```c
[USB_CDC_EP_BULK_OUT] = { {
    {
        .ADDR.reg = (uint32_t)bulkout[0],
        .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(bulkout[0])),
    }, {
        .ADDR.reg = (uint32_t)bulkout[1],
        .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(bulkout[1])),
    },
} },
[USB_CDC_EP_BULK_IN] = { {
    {
        .ADDR.reg = (uint32_t)bulkin[0],
        .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(bulkin[0])),
    }, {
        .ADDR.reg = (uint32_t)bulkin[1],
        .PCKSIZE.reg = USB_DEVICE_PCKSIZE_SIZE(BSIZE(bulkin[1])),
    },
} },
```

`BSIZE(bulkout[0])` und `BSIZE(bulkout)` sind identisch (gleiche
Element-Größe), aber `BSIZE(bulkout[0])` macht die Intention explizit.

### EP0 / ACM unverändert

`ep0out`, `ep0in`, `acmin` bleiben Single-Buffer, Single-Bank. Diese
Endpoints sind nicht der Engpass und der Setup-/Control-Pfad ist
empfindlich. Scope-Creep vermeiden.

---

## Konkrete Code-Änderungen

### 1. `src/atsamd/usbserial.c` — Pufferdefinitionen

Ersetzen:
```c
static uint8_t __aligned(4) bulkout[USB_CDC_EP_BULK_OUT_SIZE];
static uint8_t __aligned(4) bulkin[USB_CDC_EP_BULK_IN_SIZE];
```
durch die `[2][...]` Variante (s. oben).

### 2. `src/atsamd/usbserial.c` — `usb_desc[]`

Beide Bänke der Bulk-Endpoints initialisieren (s. oben).

`usb_desc __aligned(16)` aus dem Test-Patch beibehalten — schadet nie,
hat keinen Code-Smell und reduziert Layout-Heisenbug-Risiko.

### 3. `src/atsamd/usbserial.c` — Bank-Tracking-State

Direkt unter den Pufferdefinitionen:
```c
static uint8_t bulkout_bank;  // nächste OUT-Bank zum Lesen
static uint8_t bulkin_bank;   // nächste IN-Bank zum Beschreiben
```

Beide bei jedem `usb_set_configure()` auf 0 zurücksetzen (Reset-sicher).

### 4. `src/atsamd/usbserial.c` — `usb_read_bulk_out()` / `usb_send_bulk_in()`

```c
int_fast8_t
usb_read_bulk_out(void *data, uint_fast8_t max_len)
{
    uint8_t bank = bulkout_bank;
    int_fast8_t ret = usb_read_packet(USB_CDC_EP_BULK_OUT, bank,
                                      data, max_len);
    if (ret >= 0)
        bulkout_bank = bank ^ 1;
    return ret;
}

int_fast8_t
usb_send_bulk_in(void *data, uint_fast8_t len)
{
    uint8_t bank = bulkin_bank;
    int_fast8_t ret = usb_write_packet(USB_CDC_EP_BULK_IN, bank,
                                       data, len);
    if (ret >= 0)
        bulkin_bank = bank ^ 1;
    return ret;
}
```

`usb_read_packet()` und `usb_write_packet()` selbst bleiben **unverändert**.
Sie behandeln eine einzelne Bank korrekt — nur die Auswahl, welche Bank,
liegt jetzt im Aufrufer und toggelt nach jedem Erfolg.

### 5. `src/atsamd/usbserial.c` — `usb_set_configure()`

Vor dem Konfigurieren der Endpoints `bulkout_bank = 0; bulkin_bank = 0;`
setzen. Wichtig, damit ein USB-Reset während des Betriebs den State
sauber neu initialisiert.

Außerdem `EPSTATUSCLR` für beide Bänke setzen, damit nach einem Reset
keine alten BKxRDY-Bits stehenbleiben (besonders BK1RDY auf IN, das
ein Geister-Paket triggern könnte):

```c
EP_BULKOUT.EPSTATUSCLR.reg = USB_DEVICE_EPSTATUS_BK0RDY
                           | USB_DEVICE_EPSTATUS_BK1RDY;
EP_BULKIN.EPSTATUSCLR.reg  = USB_DEVICE_EPSTATUS_BK0RDY
                           | USB_DEVICE_EPSTATUS_BK1RDY;
```

### 6. `src/generic/usb_cdc.c` — Mehrere Pakete pro Task-Lauf

Aktuell verarbeitet `usb_bulk_in_task()` **ein** Paket pro Wake. Mit
zwei verfügbaren Bänken nutzt das die zweite Bank nur indirekt
(Self-Notify am Ende). Sauberer ist eine kleine Loop:

In `usb_bulk_in_task()`: nach erfolgreichem `usb_send_bulk_in()`, wenn
noch `transmit_pos > 0`, **direkt** noch ein Paket aus dem Puffer
versuchen, statt nur per `usb_notify_bulk_in()` selbst zu wecken.
Maximal 2 Iterationen pro Task-Lauf (entspricht den 2 Bänken). Danach
normale Task-Wake-Logik.

Analog in `usb_bulk_out_task()`: nach erfolgreichem
`usb_read_bulk_out()` direkt ein zweites Read versuchen, falls noch
Platz im `receive_buf` ist. Auch hier: max. 2 Iterationen.

Der Grund für die 2er-Begrenzung: nicht in der Task verhungern lassen
(andere Tasks müssen laufen), aber die Bank-Pipeline füllen.

Dieser Punkt ist **wichtig** — ohne ihn nutzt der Umbau die zweite Bank
nur opportunistisch über den `usb_notify_*`-Pfad mit ~140 µs Verzögerung
pro Paket, was den Großteil des erwarteten Gewinns auffrisst.

### 7. `src/generic/usb_cdc.c` — Pre-flight-Counter beibehalten oder entfernen

Entscheidung beim Merge:
- Wenn der `in_max_busy_us` Counter aus Schritt 0a den Fix bestätigt
  hat, kann er wieder entfernt werden (gehört nicht in einen sauberen
  PR).
- Alternativ: in einem separaten lokalen Branch behalten für spätere
  Regression-Tests, aber **nicht** im PR mitsenden.

### 8. `src/generic/usb_cdc.c` — `RECEIVE_WINDOW`

Auf `sizeof(receive_buf)` (128) lassen. Wenn der Dual-Bank-Fix wirkt,
sollte das volle Fenster keine Probleme mehr machen. Falls doch, ist
das ein Hinweis dass noch was fehlt.

### 9. `USB_CDC_DEBUG_BUILD` Marker

Auf neuen Wert setzen, z.B. `dual-bank-20260511`, damit der aktive
Build eindeutig identifizierbar ist.

---

## Was **nicht** geändert wird

- `usb_read_packet()` / `usb_write_packet()` — Single-Bank-Logik bleibt
  korrekt.
- `USB_Handler()` IRQ-Routing — die `usb_notify_*` Aufrufe sind schon
  Bank-agnostisch.
- EP0 / EP_ACM Pfade.
- `samd51_clock.c` / CMCC-Konfiguration — wurde im Verlauf der
  Investigation als nicht-ursächlich eingestuft.
- Host-Seite (`klippy/chelper/serialqueue.c`, `MIN_RTO`).

---

## Test- und Validierungsplan

### Phase A: Build & Boot

1. Patch lokal anwenden, in der Mac-Umgebung erstmal nur Compile-Check
   (kein Flash). Build muss durchlaufen für SAMX5.
2. Patch nach Pi kopieren, dort `make clean && make` mit der Mini 5+
   Konfiguration.
3. Flash. `/dev/serial/by-id/...` muss wie bisher erscheinen, Klipper
   muss connecten, `USB_CDC_DEBUG_BUILD=dual-bank-...` und
   `RECEIVE_WINDOW=128` müssen im Log auftauchen.

### Phase B: Idle-Test

1. Klipper läuft, kein Druck.
2. 10 Minuten beobachten — `bytes_retransmit` muss bei 0 bleiben.
3. `QUERY_USBCDC_DEBUG RESET=0` ausführen, plausibilisieren:
   `in_busy` sollte sehr niedrig sein (im Idle wird die zweite Bank
   selten gebraucht).

### Phase C: Reproduktions-Druck

1. Genau denselben Druck-Workload starten, der im Investigation-Log
   reproduzierbar gefailt hat.
2. Live-Monitor aus PLANS.md "Test Commands" laufen lassen.
3. Akzeptanzkriterium:
   - `mcu_rt` darf nicht in den ersten 30 Minuten auf >100 steigen.
   - Einzelne Retransmits (`mcu_rt` bleibt < ~20) sind OK, kein
     monotoner Anstieg.
4. Während des Drucks **mindestens zweimal** `QUERY_USBCDC_DEBUG`
   ausführen (z.B. nach 10 und nach 25 Minuten). Werte erfassen:
   - `in_busy` muss deutlich niedriger sein als der vorherige
     `1003`-Snapshot bei vergleichbarer `in_send`-Anzahl.
   - `in_max_busy_us` (falls Counter behalten) muss <5 ms bleiben.
   - `max_rpos` darf unter 128 bleiben.

### Phase D: Regressions-Tests

1. EBB36 Toolboard muss weiter sauber laufen — die Änderung ist
   atsamd-only, sollte EBB nicht beeinflussen, aber sicherheitshalber
   prüfen.
2. Reboot/Reconnect-Test: Klipper FIRMWARE_RESTART während Druck
   abbrechen, dann neu starten — Verbindung muss sauber kommen
   (testet die `bulk*_bank = 0` Reset-Logik in `usb_set_configure()`).
3. Optional: SAMD21-Board falls verfügbar (kein Mini 5+, sondern z.B.
   ein generisches SAMD21-Klipper-Board) mit derselben Firmware-Codebasis
   bauen und kurz testen, dass nichts kaputt ist. Wenn kein Board
   vorhanden, in der Open-Questions des PR vermerken.

### Phase E: Lange Stabilität

Wenn Phase C sauber durchläuft: ein vollständiger ~3-4h-Druck.
Akzeptanz: `mcu_rt` bleibt unter 50, kein Crash.

---

## Rollback-Strategie

Alle Änderungen sind in zwei Dateien:
- `src/atsamd/usbserial.c`
- `src/generic/usb_cdc.c`

Rollback = `git checkout master -- src/atsamd/usbserial.c src/generic/usb_cdc.c`,
neu bauen, flashen.

Wenn der Druck mit Dual-Bank schlechter ist als der Status quo
(z.B. corruption / out-of-order ACKs), sofort zurückrollen und
**nicht** versuchen, die FIFO-Logik live zu debuggen.

---

## Risiken und ihre Mitigation

| Risiko | Wahrscheinlichkeit | Auswirkung | Mitigation |
|--------|--------|--------|------------|
| Out-of-order IN-Pakete (Bank-Reihenfolge falsch) | mittel | hoch (Sequence-Bruch) | strikt FIFO, kein "ausweichen" auf andere Bank |
| BKxRDY nach Reset stehengeblieben | niedrig | hoch (Geister-Paket) | `EPSTATUSCLR` beider Bänke in `usb_set_configure()` |
| `usb_desc` zu groß / Padding-Problem | niedrig | hoch (HW liest Müll) | bestehende `__aligned(16)` beibehalten, sizeof(usb_desc) im Build verifizieren |
| Mehrfach-Loop in Task führt zu anderen Task-Latenzen | niedrig | mittel | max. 2 Iterationen pro Task-Lauf |
| SAMD21 Verhalten anders | mittel | mittel (andere Boards betroffen) | Bank-Modell ist identisch laut Datasheet, aber Phase D Regression-Test einplanen |
| Heisenbug bleibt — Dual-Bank war nicht die Ursache | niedrig wenn Pre-flight positiv, sonst hoch | hoch (Aufwand umsonst) | Pre-flight Schritt 0 macht das Risiko greifbar bevor der Umbau startet |

---

## Akzeptanzkriterien für den finalen Fix

Damit der Fix als "echter Fix" und nicht als "weitere Maskierung" gilt,
müssen ALLE Punkte erfüllt sein:

1. Der Reproduktions-Workload aus PLANS.md läuft mindestens 2× ohne
   `mcu_rt`-Eskalation durch.
2. `RECEIVE_WINDOW=128` (das volle Buffer-Fenster) bleibt aktiv und
   funktioniert. Kein Rückfall auf 64 nötig.
3. `QUERY_USBCDC_DEBUG`-Snapshots zeigen `in_busy` <100 bei
   `in_send` >50000 (heute: 1003 bei 57498 → ~1.7%, Ziel: <0.2%).
4. `bytes_invalid` bleibt 0.
5. EBB36 Verbindung bleibt unverändert stabil.
6. Ein `FIRMWARE_RESTART` mitten im Druck bringt die Verbindung sauber
   wieder.

Nur wenn alle 6 Punkte erfüllt sind: PR vorbereiten.

---

## PR-Vorbereitung (nach erfolgreichem Test)

1. Cleaner Branch von `master`, nur die zwei produktiven Dateien.
2. `USB_CDC_DEBUG_BUILD` Marker und alle Debug-Counter (`get_usbcdc_debug`,
   `usb_cdc_debug_s`, etc.) wieder entfernen — gehören nicht in den PR.
3. `klippy/extras/usbcdc_debug.py`, `klippy/chelper/serialqueue.c` Debug
   und `PLANS.md`/`dual-bank.md` **nicht** committen.
4. Commit-Message: kurze Beschreibung, dass dual-banked Bulk-Endpoints
   für SAMD21/SAMX5 USB-CDC aktiviert werden, mit Bezug auf das
   Mini 5+ Retransmit-Problem als Motivation. Vor dem Commit erst
   mit User abstimmen.

---

## Wenn die Pre-flight-Messung negativ ist

Falls Schritt 0 zeigt, dass `in_max_busy_us` <5 ms bleibt, ist die
Hypothese "IN-Bank ist der Engpass" falsch. Dann **diesen Plan nicht
ausführen**. Stattdessen folgende Pfade neu bewerten:

- Lokale `bulkout`/`bulkin` Buffer in einen explizit non-cacheable
  SRAM-Bereich legen (MPU-Region konfigurieren), um die CMCC-Hypothese
  abschließend auszuschließen.
- usbmon trotzdem auf dem Pi laufen lassen — auch wenn EBB sauber läuft,
  könnte ein Mini-5+-spezifischer Endpoint-Konfigurations-Quirk
  Pakete verlieren ohne dass `bytes_invalid` getriggert wird.
- Schematic-Review Mini 5+ USB-Pfad: Pull-ups, Common-Mode Choke,
  ESD-Diode-Kapazität — auf Mini-USB-Buchse evtl. anders dimensioniert
  als auf USB-C.
