# 🔥 Autoterm Air2D ESPHome Integration

> Vollständige ESPHome-Integration für die Autoterm / Planar Air2D Dieselheizung
> mit direkter UART-Kommunikation über einen ESP32 – inklusive Status-, Sensor-
> und Steuerfunktionen (Heizen, Lüften, Abschalten, Parameter).

---

## 🧩 Überblick

Dieses Projekt ermöglicht die **vollständige Ansteuerung und Überwachung einer Autoterm Air2D / Planar 2D** Heizung
über einen **ESP32 mit ESPHome**. Die Firmware lauscht parallel auf den Verbindungen zwischen Bedienteil und Heizung
und kann gleichzeitig eigene Kommandos senden.

Die Kommunikation erfolgt direkt über den UART-Bus zwischen:
- 📟 **Bedienteil (Display)**
- 🔥 **Heizung (Controller)**
- 🧠 **ESP32 (Bridge + Parser)**

Der ESP liest und schreibt Telegramme im Autoterm-Protokoll (0xAA … CRC16)
und stellt alle Werte als Sensoren, Schalter, Buttons, Nummern und Selects in Home Assistant bereit.【F:components/autoterm_uart/autoterm_uart.h†L241-L321】【F:components/autoterm_uart/__init__.py†L34-L83】

---

## ⚙️ Funktionsumfang

| Kategorie | Beschreibung |
|:--|:--|
| 🔍 **Status-Monitoring** | Liest Temperatur-, Spannungs-, Drehzahl- und Pumpenwerte inklusive Statuscode und Text aus dem Heizungsbus und publiziert sie als Sensoren/Text-Sensoren.【F:components/autoterm_uart/autoterm_uart.h†L390-L462】 |
| 🧭 **Settings-Verwaltung** | Fragt zyklisch die aktuellen Einstellungen (Temperaturquelle, Solltemperatur, Arbeitszeit, Leistungsstufe, Wartebetrieb, Arbeitszeitmodus) ab, speichert sie lokal und veröffentlicht sie in Home Assistant.【F:components/autoterm_uart/autoterm_uart.h†L464-L629】 |
| 🌀 **Lüftersteuerung** | Aktiviert den Lüftermodus („only fan“) und setzt die Lüfterstufe (0–9).【F:components/autoterm_uart/autoterm_uart.h†L337-L349】【F:components/autoterm_uart/autoterm_uart.h†L491-L515】 |
| 🔥 **Heizleistung & Temperatur** | Stellt Solltemperatur (0–40 °C), Arbeitszeit (0–255 min) und Leistungsstufe (0–9) per Number-Entitäten ein.【F:components/autoterm_uart/__init__.py†L136-L164】【F:components/autoterm_uart/autoterm_uart.h†L351-L364】【F:components/autoterm_uart/autoterm_uart.h†L517-L527】 |
| 🔌 **Start/Stop** | Startet (`power_on`) bzw. stoppt (`power_off`) die Heizung per Button-Entitäten.【F:components/autoterm_uart/__init__.py†L56-L58】【F:components/autoterm_uart/autoterm_uart.h†L332-L335】【F:components/autoterm_uart/autoterm_uart.h†L667-L716】 |
| 🪄 **Bridge-Funktion** | Forwardet alle UART-Frames zwischen Display ↔ Heizung und „snifft“ dabei passiv mit – inklusive CRC16-Validierung & Hex-Logging.【F:components/autoterm_uart/autoterm_uart.h†L241-L305】 |
| 🏠 **Home Assistant Integration** | Alle Sensoren, Buttons, Switches, Number- und Select-Entitäten werden automatisch angelegt; Min-/Max-/Step-Werte sind konfigurierbar und haben sinnvolle Defaultwerte.【F:components/autoterm_uart/__init__.py†L136-L176】 |

---

## 🧱 Projektstruktur

```
autoterm-air2d/
├── air2d.yaml                       # Beispiel-ESPHome-Konfiguration
└── components/
    └── autoterm_uart/
        ├── __init__.py              # Python-Binding & Konfig-Schema
        └── autoterm_uart.h          # C++-Implementierung (Bridge + Parser)
```

---

## 🚀 Installation

1. ESP32 mit **zwei UARTs** an Heizung und Bedienteil anschließen (siehe Verdrahtung).
2. Repository in dein ESPHome-Verzeichnis kopieren (z. B. `/config/esphome/autoterm-air2d/`).
3. `air2d.yaml` in ESPHome importieren oder als Vorlage verwenden und deine WLAN-/Domain-Secrets eintragen.【F:air2d.yaml†L1-L38】
4. Kompilieren & flashen. Nach dem Start erscheinen alle Entitäten automatisch in Home Assistant.
5. Optional: Passe die Standardwerte (z. B. Lüfterstufe, Temperaturgrenzen) im YAML an; Defaultbereiche liefert die Komponente automatisch.【F:components/autoterm_uart/__init__.py†L136-L164】

---

## 🔌 Hardware-Verdrahtung

| Signal | ESP32 Pin | Beschreibung |
|--------|-----------|--------------|
| UART1 RX | 16 | vom Display TX |
| UART1 TX | 17 | zum Display RX |
| UART2 RX | 22 | von der Heizung TX |
| UART2 TX | 23 | zur Heizung RX |
| GND | GND | gemeinsame Masse |

> ⚠️ UART-Pegel beachten – 5 V-Leitungen ggf. mit Pegelwandler anpassen.

---

## 🧠 Entitäten in Home Assistant

### Sensoren & Text-Sensoren

| Entity | Typ | Beschreibung |
|--------|-----|--------------|
| `sensor.autoterm_voltage` | Sensor | Bordspannung in Volt.【F:components/autoterm_uart/__init__.py†L42-L44】【F:components/autoterm_uart/autoterm_uart.h†L404-L457】 |
| `sensor.autoterm_heater_temperature` | Sensor | Temperatur im Heizgerät (°C).【F:components/autoterm_uart/__init__.py†L41】【F:components/autoterm_uart/autoterm_uart.h†L405-L456】 |
| `sensor.autoterm_internal_temperature` | Sensor | Interner Sensor im Bedienteil (°C).【F:components/autoterm_uart/__init__.py†L39】【F:components/autoterm_uart/autoterm_uart.h†L402-L455】 |
| `sensor.autoterm_external_temperature` | Sensor | Externer Temperatursensor (°C).【F:components/autoterm_uart/__init__.py†L40】【F:components/autoterm_uart/autoterm_uart.h†L403-L455】 |
| `sensor.autoterm_fan_rpm_set` / `sensor.autoterm_fan_rpm_actual` | Sensor | Soll-/Ist-Drehzahl des Lüfters (rpm).【F:components/autoterm_uart/__init__.py†L44-L45】【F:components/autoterm_uart/autoterm_uart.h†L406-L461】 |
| `sensor.autoterm_pump_frequency` | Sensor | Pumpenfrequenz in Hz.【F:components/autoterm_uart/__init__.py†L46】【F:components/autoterm_uart/autoterm_uart.h†L408-L461】 |
| `sensor.autoterm_status` | Sensor | Rohstatus als Zahl (z. B. 3.0).【F:components/autoterm_uart/__init__.py†L43】【F:components/autoterm_uart/autoterm_uart.h†L401-L458】 |
| `text_sensor.autoterm_status_text` | Text | Menschenlesbare Statusbeschreibung (z. B. „heating“).【F:components/autoterm_uart/__init__.py†L48】【F:components/autoterm_uart/autoterm_uart.h†L410-L459】 |
| `text_sensor.autoterm_temperature_source` | Text | Aktive Temperaturquelle als Text.【F:components/autoterm_uart/__init__.py†L49】【F:components/autoterm_uart/autoterm_uart.h†L614-L619】 |
| `sensor.autoterm_set_temperature` | Sensor | Ausgelesene Solltemperatur (°C).【F:components/autoterm_uart/__init__.py†L50】【F:components/autoterm_uart/autoterm_uart.h†L469-L487】 |
| `sensor.autoterm_work_time` | Sensor | Arbeitszeit (Minuten).【F:components/autoterm_uart/__init__.py†L51】【F:components/autoterm_uart/autoterm_uart.h†L468-L487】 |
| `sensor.autoterm_power_level` | Sensor | Leistungsstufe (0–9).【F:components/autoterm_uart/__init__.py†L52】【F:components/autoterm_uart/autoterm_uart.h†L470-L487】 |
| `sensor.autoterm_wait_mode` | Sensor | Wartebetriebsstatus (Rohwert).【F:components/autoterm_uart/__init__.py†L53】【F:components/autoterm_uart/autoterm_uart.h†L472-L487】 |
| `sensor.autoterm_use_work_time` | Sensor | Gibt an, ob der Timer aktiv ist (0 = aktiv).【F:components/autoterm_uart/__init__.py†L54】【F:components/autoterm_uart/autoterm_uart.h†L468-L487】 |

### Steuer-Entitäten

| Entity | Typ | Beschreibung |
|--------|-----|--------------|
| `button.autoterm_einschalten` | Button | Sendet Startkommando an die Heizung (`0x01`).【F:components/autoterm_uart/__init__.py†L56】【F:components/autoterm_uart/autoterm_uart.h†L332-L335】【F:components/autoterm_uart/autoterm_uart.h†L694-L715】 |
| `button.autoterm_ausschalten` | Button | Sendet Stop (`0x03`).【F:components/autoterm_uart/__init__.py†L57】【F:components/autoterm_uart/autoterm_uart.h†L667-L692】 |
| `button.autoterm_lueften` | Button | Aktiviert den Lüftermodus (`0x23`, Level aus Number).【F:components/autoterm_uart/__init__.py†L58】【F:components/autoterm_uart/autoterm_uart.h†L337-L343】【F:components/autoterm_uart/autoterm_uart.h†L491-L515】 |
| `number.autoterm_luefterstufe` | Number | Lüfterstufe 0–9, Standardbereich 0..9.【F:components/autoterm_uart/__init__.py†L59-L143】 |
| `number.autoterm_solltemperatur` | Number | Solltemperatur 0–40 °C.【F:components/autoterm_uart/__init__.py†L60-L151】 |
| `number.autoterm_arbeitszeit` | Number | Arbeitszeit 0–255 min.【F:components/autoterm_uart/__init__.py†L64-L157】 |
| `number.autoterm_leistungsstufe` | Number | Leistungslevel 0–9.【F:components/autoterm_uart/__init__.py†L68-L164】 |
| `select.autoterm_temperaturquelle` | Select | Wählt Temperaturquelle (intern, Panel, extern, ohne Regelung).【F:components/autoterm_uart/__init__.py†L72-L170】【F:components/autoterm_uart/autoterm_uart.h†L365-L374】【F:components/autoterm_uart/autoterm_uart.h†L640-L665】 |
| `switch.autoterm_arbeitszeit_verwenden` | Switch | Aktiviert/deaktiviert Arbeitszeit (Switch „AN“ ⇒ `use_work_time = 0`).【F:components/autoterm_uart/__init__.py†L76-L174】【F:components/autoterm_uart/autoterm_uart.h†L377-L379】【F:components/autoterm_uart/autoterm_uart.h†L544-L546】【F:components/autoterm_uart/autoterm_uart.h†L625-L626】 |
| `switch.autoterm_wartebetrieb` | Switch | Setzt Wartebetrieb (Switch „AN“ ⇒ `wait_mode = 1`).【F:components/autoterm_uart/__init__.py†L80-L176】【F:components/autoterm_uart/autoterm_uart.h†L382-L384】【F:components/autoterm_uart/autoterm_uart.h†L548-L549】【F:components/autoterm_uart/autoterm_uart.h†L626-L628】 |

---

## 📜 Unterstützte Befehle & Statuscodes

### Befehle (vom ESP gesendet)

| Code | Beschreibung | Richtung |
|------|--------------|----------|
| `0x01` | Heizung starten (Power ON).【F:components/autoterm_uart/autoterm_uart.h†L694-L715】 | ESP → Heizung |
| `0x03` | Heizung/Lüfter ausschalten (Power OFF).【F:components/autoterm_uart/autoterm_uart.h†L667-L692】 | ESP → Heizung |
| `0x23` | Lüftermodus aktivieren + Stufe setzen. Level wird als Drittbyte übertragen.【F:components/autoterm_uart/autoterm_uart.h†L491-L515】 | ESP → Heizung |
| `0x02` | Einstellungen anfordern (beim Start).【F:components/autoterm_uart/autoterm_uart.h†L552-L570】 | ESP → Heizung |
| `0x02` | Einstellungen schreiben (use_work_time, work_time, temp_src, set_temp, wait_mode, power_level).【F:components/autoterm_uart/autoterm_uart.h†L572-L604】 | ESP → Heizung |

### Ausgewertete Statuscodes

| Code | Text | Beschreibung |
|------|------|--------------|
| `0x0001` | `standby` | Heizung im Bereitschaftsmodus. |
| `0x0100` | `cooling flame sensor` | Nachlauf/Kühlung des Flammensensors. |
| `0x0101` | `ventilation` | Lüfterbetrieb. |
| `0x0201` – `0x0204` | `heating …` / `ignition …` | Zünd- und Aufheizphasen. |
| `0x0300` | `heating` | Normaler Heizbetrieb. |
| `0x0323` | `only fan` | Reiner Lüftermodus. |
| `0x0304` | `cooling down` | Nachlauf/Abkühlung. |
| `0x0400` | `shutting down` | Abschaltvorgang. |

*(Weitere Codes werden geloggt und als `unknown` angezeigt.)*【F:components/autoterm_uart/autoterm_uart.h†L410-L446】

---

## ⚙️ Konfiguration anpassen

- **Logger deaktivieren/umschalten:** Der UART-Logger wird deaktiviert (`baud_rate: 0`), um Kollisionen mit dem Heizungsbus zu vermeiden.【F:air2d.yaml†L15-L18】 
- **Weboberfläche:** ESPHome-Webserver ist optional aktiv (`web_server`). Entfernen, falls nicht benötigt.【F:air2d.yaml†L35-L37】
- **Entitätsnamen:** Passe die `name:`-Felder im `autoterm_uart`-Block an deine Installation an.【F:air2d.yaml†L62-L127】
- **Grenzwerte:** Für Numbers werden sinnvolle Standardbereiche gesetzt, die bei Bedarf überschrieben werden können (z. B. `min_value`, `max_value`).【F:components/autoterm_uart/__init__.py†L136-L164】
- **Temperaturquelle:** Die Select-Entität akzeptiert nur die vier vordefinierten Optionen der Autoterm-Elektronik.【F:components/autoterm_uart/__init__.py†L27-L33】【F:components/autoterm_uart/autoterm_uart.h†L640-L665】

---

## 🛠️ Fehlerdiagnose & Logging

- Jede empfangene Nachricht wird inkl. CRC geprüft und bei Erfolg als Hexdump geloggt – ideal zum Reverse Engineering.【F:components/autoterm_uart/autoterm_uart.h†L249-L305】
- Ungültige CRCs werden verworfen und als Warnung ausgegeben.【F:components/autoterm_uart/autoterm_uart.h†L264-L270】
- Beim Start fordert die Komponente automatisch die aktuellen Einstellungen an und veröffentlicht sie, sobald gültig.【F:components/autoterm_uart/autoterm_uart.h†L246-L247】【F:components/autoterm_uart/autoterm_uart.h†L552-L604】
- Über Home Assistant lässt sich jederzeit kontrollieren, welche Werte zuletzt zur Heizung gesendet wurden (siehe Sensoren & Switches).

---

## 🧰 ToDo / Nächste Schritte

- [ ] ACK/Rückmeldungen für den Lüftermodus (`0x23`) auswerten.
- [ ] Diagnose-/Fehlercodes (`0x10`) parsen und als Entities veröffentlichen.
- [ ] Optional: Historisierung der Laufzeiten & Zündzyklen.

---

## 🧑‍💻 Autor

**Autoterm Air2D ESPHome Integration** – erstellt 2025 von Tim.
