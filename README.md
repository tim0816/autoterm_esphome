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
| 🌀 **Lüftersteuerung** | Aktiviert den Lüftermodus („only fan“) und ändert die Lüfterstufe (0–9) |
| ⛔ **Ausschalten** | Schaltet Heizung oder Lüfter komplett aus (`0x03`) |
| 🧾 **Protokoll-CRC** | CRC16 (Modbus) Validierung sämtlicher Frames |
| 🪄 **Bridge-Funktion** | UART-Forwarding zwischen Display ↔ Heizung, ESP „snifft“ passiv mit |
| 🏠 **Home Assistant Integration** | Alle Sensoren, Schalter und Buttons erscheinen automatisch als Entities |

---

## 🧱 Projektstruktur

