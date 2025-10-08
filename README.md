# 🔥 Autoterm Air2D ESPHome Integration

> Vollständige ESPHome-Integration für die Autoterm / Planar Air2D Dieselheizung  
> mit direkter UART-Kommunikation über einen ESP32 – inklusive Status-, Sensor-  
> und Steuerfunktionen (Heizen, Lüften, Abschalten, Parameter).

---

## 🧩 Überblick

Dieses Projekt ermöglicht die **vollständige Ansteuerung und Überwachung einer Autoterm Air2D / Planar 2D** Heizung  
über einen **ESP32 mit ESPHome**.  

Die Kommunikation erfolgt direkt über den UART-Bus zwischen:
- 📟 **Bedienteil (Display)**  
- 🔥 **Heizung (Controller)**  
- 🧠 **ESP32 (Bridge + Parser)**

Der ESP liest und schreibt Telegramme im Autoterm-Protokoll (0xAA … CRC16)  
und stellt alle Werte als Sensoren, Schalter, Buttons und Nummern in Home Assistant bereit.

---

## ⚙️ Funktionen

| Kategorie | Beschreibung |
|:--|:--|
| 🔍 **Statusauswertung** | Liest regelmäßig Heizungstemperatur, Spannung, interne/externe Sensorwerte, Lüfterdrehzahl, Pumpenfrequenz, Betriebsstatus |
| 🧭 **Settings** | Liest aktuelle Einstellungen (Temperaturquelle, Solltemperatur, Arbeitszeit, Leistungsstufe, etc.) |
| 🌀 **Lüftersteuerung** | Aktiviert den Lüftermodus („only fan“) und nutzt die aktuell konfigurierte Leistungsstufe |
| ⛔ **Ausschalten** | Schaltet Heizung oder Lüfter komplett aus (`0x03`) |
| 🧾 **Protokoll-CRC** | CRC16 (Modbus) Validierung sämtlicher Frames |
| 🪄 **Bridge-Funktion** | UART-Forwarding zwischen Display ↔ Heizung, ESP „snifft“ passiv mit |
| 🏠 **Home Assistant Integration** | Alle Sensoren, Schalter und Buttons erscheinen automatisch als Entities |

---

## 🧱 Projektstruktur
autoterm-air2d/
├── air2d.yaml # ESPHome-Konfiguration
└── components/
└── autoterm_uart/
├── init.py # Python-Definition (Schema, Bindings)
└── autoterm_uart.h # C++-Implementierung (Bridge + Parser)


---

## 🚀 Installation

1. ESP32 mit **RX/TX** an Heizung und Bedienteil anschließen  
   (zwei UARTs: einer zur Heizung, einer zum Display)
2. Dateien in dein ESPHome-Verzeichnis kopieren: /config/esphome/autoterm-air2d/

3. In ESPHome-UI:
- Projekt importieren (`air2d.yaml`)
- Kompilieren & Flashen
4. Nach dem Start erscheinen alle Sensoren automatisch in Home Assistant.

---

## 🔌 Hardware-Verdrahtung

| Signal | ESP32 Pin | Beschreibung |
|--------|-----------|--------------|
| UART1 RX | 16 | vom Display TX |
| UART1 TX | 17 | zum Display RX |
| UART2 RX | 22 | von der Heizung TX |
| UART2 TX | 23 | zur Heizung RX |
| GND | GND | gemeinsame Masse |

> ⚠️ UART-Pegel beachten – 5V-Leitungen ggf. mit Pegelwandler anpassen.

---

## 📜 Unterstützte Befehle

| Code | Beschreibung | Richtung | Beispiel |
|------|---------------|-----------|-----------|
| `0x0F` | Get Status | Display → Heizung / Heizung → Display | `AA 04 13 00 0F … CRC` |
| `0x02` | Get Settings | Display ↔ Heizung | `AA 04 06 00 02 … CRC` |
| `0x23` | Turn only fan on | ESP → Heizung | `AA 03 04 00 23 FF FF 08 FF CRC` |
| `0x03` | Turn heater/fan off | ESP → Heizung | `AA 03 00 00 03 CRC` |
| `0x01` | Start heater *(optional)* | ESP → Heizung | `AA 03 00 00 01 CRC` |

---

## 🧠 Sensoren in Home Assistant

| Entity | Beschreibung |
|--------|---------------|
| `sensor.autoterm_voltage` | Bordspannung (V) |
| `sensor.autoterm_heater_temperature` | Heizungstemperatur |
| `sensor.autoterm_internal_temperature` | Interner Temperatursensor |
| `sensor.autoterm_external_temperature` | Externer Temperatursensor |
| `sensor.autoterm_fan_rpm_set` | Soll-Drehzahl *(Diagnostic Entity)* |
| `sensor.autoterm_fan_rpm_actual` | Ist-Drehzahl *(Diagnostic Entity)* |
| `sensor.autoterm_pump_frequency` | Pumpenfrequenz *(Diagnostic Entity)* |
| `sensor.autoterm_status` | Statuscode (z. B. 3.0) *(Diagnostic Entity)* |
| `text_sensor.autoterm_status_text` | Statusbeschreibung (z. B. „heating“) |
| `text_sensor.autoterm_temperature_source` | Temperaturquelle *(Diagnostic Entity)* |

---

## 🔘 Steuerung in Home Assistant

| Entity | Typ | Beschreibung |
|--------|-----|--------------|
| `switch.autoterm_lueften` | Switch | Lüftermodus ein/aus (`0x23`) |
| `number.autoterm_luefterstufe` | Number | Lüfterstufe 0–9 |
| `button.autoterm_ausschalten` | Button | Heizung/Lüfter ausschalten (`0x03`) |
| *(optional)* `button.autoterm_starten` | Button | Heizung starten (`0x01`) |
| `button.autoterm_lueften` | Button | Lüften mit aktueller Leistungsstufe starten |

---

## 🧩 Technische Details

- UART-Kommunikation mit 9600 baud, 8N1  
- CRC16 (Modbus, Polynomial `0xA001`)  
- Byte-Frames beginnen immer mit `0xAA`  
- Bridge-Logik: ESP liest beide UARTs und forwardet 1:1 zwischen Display ↔ Heizung  
- Parser erkennt Nachrichtentyp (`data[4]`) und ruft passende Routine auf

---

## 🧰 ToDo / Nächste Schritte

- [ ] Implementierung des **Start-Heater-Buttons (`0x01`)**
- [ ] Unterstützung für Temperatur-Setpoints senden
- [ ] Rückmeldeparser für `0x23` (Fan-ACK)
- [ ] Integration der Diagnosemeldungen (`0x10`)

---

## 🧑‍💻 Autor

**Autoterm Air2D ESPHome Integration**  
Erstellt 2025 von Tim


---


