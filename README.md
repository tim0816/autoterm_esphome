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
und stellt alle Werte als Sensoren, Schalter, Buttons, Nummern und Selects in Home Assistant bereit.

---

## ⚙️ Funktionsumfang

| Kategorie | Beschreibung |
|:--|:--|
| 🔍 **Status-Monitoring** | Liest Temperatur-, Spannungs-, Drehzahl- und Pumpenwerte inklusive Statuscode und Text aus dem Heizungsbus und publiziert sie als Sensoren/Text-Sensoren. |
| 🧭 **Settings-Verwaltung** | Fragt zyklisch die aktuellen Einstellungen (Temperaturquelle, Solltemperatur, Arbeitszeit, Leistungsstufe, Wartebetrieb, Arbeitszeitmodus) ab, speichert sie lokal und veröffentlicht sie in Home Assistant. |
| 🌀 **Lüftersteuerung** | Aktiviert den Lüftermodus („only fan“) und setzt die Lüfterstufe (0–9). |
| 🔥 **Heizleistung & Temperatur** | Stellt Solltemperatur (0–40 °C), Arbeitszeit (0–255 min) und Leistungsstufe (0–9) per Number-Entitäten ein. |
| 🔌 **Start/Stop** | Startet (`power_on`) bzw. stoppt (`power_off`) die Heizung per Button-Entitäten. |
| 🪄 **Bridge-Funktion** | Forwardet alle UART-Frames zwischen Display ↔ Heizung und „snifft“ dabei passiv mit – inklusive CRC16-Validierung & Hex-Logging. |
| 🏠 **Home Assistant Integration** | Alle Sensoren, Buttons, Switches, Number- und Select-Entitäten werden automatisch angelegt; Min-/Max-/Step-Werte sind konfigurierbar und haben sinnvolle Defaultwerte. |

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
3. `air2d.yaml` in ESPHome importieren oder als Vorlage verwenden und deine WLAN-/Domain-Secrets eintragen.
4. Kompilieren & flashen. Nach dem Start erscheinen alle Entitäten automatisch in Home Assistant.
5. Optional: Passe die Standardwerte (z. B. Lüfterstufe, Temperaturgrenzen) im YAML an; Defaultbereiche liefert die Komponente automatisch.

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
| `sensor.autoterm_voltage` | Sensor | Bordspannung in Volt. |
| `sensor.autoterm_heater_temperature` | Sensor | Temperatur im Heizgerät (°C). |
| `sensor.autoterm_internal_temperature` | Sensor | Interner Sensor im Bedienteil (°C). |
| `sensor.autoterm_external_temperature` | Sensor | Externer Temperatursensor (°C). |
| `sensor.autoterm_fan_rpm_set` / `sensor.autoterm_fan_rpm_actual` | Sensor | Soll-/Ist-Drehzahl des Lüfters (rpm). |
| `sensor.autoterm_pump_frequency` | Sensor | Pumpenfrequenz in Hz. |
| `sensor.autoterm_status` | Sensor | Rohstatus als Zahl (z. B. 3.0). |
| `text_sensor.autoterm_status_text` | Text | Menschenlesbare Statusbeschreibung (z. B. „heating“). |
| `text_sensor.autoterm_temperature_source` | Text | Aktive Temperaturquelle als Text. |
| `sensor.autoterm_set_temperature` | Sensor | Ausgelesene Solltemperatur (°C). |
| `sensor.autoterm_work_time` | Sensor | Arbeitszeit (Minuten). |
| `sensor.autoterm_power_level` | Sensor | Leistungsstufe (0–9). |
| `sensor.autoterm_wait_mode` | Sensor | Wartebetriebsstatus (Rohwert). |
| `sensor.autoterm_use_work_time` | Sensor | Gibt an, ob der Timer aktiv ist (0 = aktiv). |

### Steuer-Entitäten

| Entity | Typ | Beschreibung |
|--------|-----|--------------|
| `button.autoterm_einschalten` | Button | Sendet Startkommando an die Heizung (`0x01`). |
| `button.autoterm_ausschalten` | Button | Sendet Stop (`0x03`). |
| `button.autoterm_lueften` | Button | Aktiviert den Lüftermodus (`0x23`, Level aus Number). |
| `number.autoterm_luefterstufe` | Number | Lüfterstufe 0–9, Standardbereich 0..9. |
| `number.autoterm_solltemperatur` | Number | Solltemperatur 0–40 °C. |
| `number.autoterm_arbeitszeit` | Number | Arbeitszeit 0–255 min. |
| `number.autoterm_leistungsstufe` | Number | Leistungslevel 0–9. |
| `select.autoterm_temperaturquelle` | Select | Wählt Temperaturquelle (intern, Panel, extern, ohne Regelung). |
| `switch.autoterm_arbeitszeit_verwenden` | Switch | Aktiviert/deaktiviert Arbeitszeit (Switch „AN“ ⇒ `use_work_time = 0`). |
| `switch.autoterm_wartebetrieb` | Switch | Setzt Wartebetrieb (Switch „AN“ ⇒ `wait_mode = 1`). |

---

## 📜 Unterstützte Befehle & Statuscodes

### Befehle (vom ESP gesendet)

| Code | Beschreibung | Richtung |
|------|--------------|----------|
| `0x01` | Heizung starten (Power ON). | ESP → Heizung |
| `0x03` | Heizung/Lüfter ausschalten (Power OFF). | ESP → Heizung |
| `0x23` | Lüftermodus aktivieren + Stufe setzen. Level wird als Drittbyte übertragen. | ESP → Heizung |
| `0x02` | Einstellungen anfordern (beim Start). | ESP → Heizung |
| `0x02` | Einstellungen schreiben (use_work_time, work_time, temp_src, set_temp, wait_mode, power_level). | ESP → Heizung |

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

*(Weitere Codes werden geloggt und als `unknown` angezeigt.)*

---


## 🛠️ Fehlerdiagnose & Logging

- Jede empfangene Nachricht wird inkl. CRC geprüft und bei Erfolg als Hexdump geloggt – ideal zum Reverse Engineering.
- Ungültige CRCs werden verworfen und als Warnung ausgegeben.


---

## 🧰 ToDo / Nächste Schritte

- [ ] ACK/Rückmeldungen für den Lüftermodus (`0x23`) auswerten.
- [ ] Diagnose-/Fehlercodes (`0x10`) parsen und als Entities veröffentlichen.
- [ ] Optional: Historisierung der Laufzeiten & Zündzyklen.

---

## 🧑‍💻 Autor

**Autoterm Air2D ESPHome Integration** – erstellt 2025 von Tim.
