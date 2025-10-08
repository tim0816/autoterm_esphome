# Autoterm Air 2D – Protokollanalyse und Betriebsmodusvergleich

Dieses Dokument fasst die beobachteten Kommunikationsprotokolle zwischen dem **Display** und der **Autoterm Air 2D Standheizung** zusammen.  
Es basiert auf echten Log-Dateien und dokumentiert die Unterschiede der drei Hauptbetriebsarten.

---

## 🔧 Allgemeine Struktur der Display → Heizung Telegramme

**Frame-Aufbau:**
```
AA 03 06 00 XX [use_work_time] [work_time] [temp_src] [set_temp] [wait_mode] [level] [CRC_L] [CRC_H]
```

| Feld | Bedeutung | Werte / Beschreibung |
|------|------------|----------------------|
| `AA 03` | Header | Startbyte, Display → Heizung |
| `06 00` | Länge / Datentyp | 6 Byte Nutzlast |
| `XX` | Command-ID | 0x01 = Start / Control, 0x02 = Settings-Update |
| `use_work_time` | Zeitsteuerung | 0 = Timer aktiv, 255 = Dauerbetrieb |
| `work_time` | Laufzeit in Minuten | 1–240 oder 255 (∞) |
| `temp_src` | Temperaturquelle | 2 = Panel-Sensor, 4 = keine Regelung |
| `set_temp` | Solltemperatur | °C = hexadezimaler Wert |
| `wait_mode` | Betriebsmodus | 1 = Heizen+Lüften, 2 = Temperaturmodus, 3 = Timer |
| `level` | Leistungsstufe | 0–8 oder 255 (Automatik) |
| `CRC_L/H` | Prüfsumme | CRC16 über Frameinhalt |

---

## 🔩 Bekannte Display → Heizung Frames

| Funktion | Vollständiger Frame | Dekodierte Werte | Beschreibung |
|-----------|--------------------|------------------|--------------|
| **Start – Thermostatmodus** | `AA 03 06 00 01 FF FF 04 FF 02 08 EC DF` | temp_src=4, wait_mode=2, level=8 | Start ohne automatische Temperaturregelung (feste Leistung) |
| **Start – Heizen + Lüften** | `AA 03 06 00 01 FF FF 02 16 01 FF E6 4F` | temp_src=2, wait_mode=1, set_temp=22°C | Start mit Temperaturregelung (Heizen+Lüften) |
| **Start – Temperaturmodus (nur Heizen)** | `AA 03 06 00 01 FF FF 02 14 02 FF D6 EE` | temp_src=2, wait_mode=2, set_temp=20°C | Start Temperaturmodus (Heizen bis Solltemp) |
| **Set Temp 30°C** | `AA 03 06 00 02 FF FF 02 1E 01 FF 24 FD` | temp_src=2, set_temp=30°C | Laufendes Temperatur-Update |
| **Set Temp 24°C** | `AA 03 06 00 02 FF FF 02 18 01 FF 25 1D` | temp_src=2, set_temp=24°C | Sollwertänderung |
| **Set Temp 20°C** | `AA 03 06 00 02 FF FF 02 14 01 FF 26 DD` | temp_src=2, set_temp=20°C | Sollwertänderung |

---

## 🧩 Vergleich der Betriebsmodi

| Parameter / Byte-Feld | **Thermostatmodus** | **Heizen + Lüften** | **Temperaturmodus (nur Heizen)** | Beschreibung |
|------------------------|---------------------|----------------------|-----------------------------------|--------------|
| Command-ID | 0x01 | 0x01 / 0x02 | 0x01 | Start / Update |
| use_work_time | 255 | 255 | 255 | Dauerbetrieb |
| work_time | 255 | 255 / 0 | 255 | Laufzeit |
| temp_src | 4 (no auto ctrl) | 2 (Panel sensor) | 2 (Panel sensor) | Temperaturquelle |
| set_temp | 20°C | 22–30°C | 20°C | Solltemperatur |
| wait_mode | 2 | 1 | 2 | Regelstrategie |
| level | 8 (fix) | 255 (Auto) | 255 (Auto) | Lüfterstufe |
| Status | 0x0100, 0x0200 | 0x0101, 0x0200, 0x030x | 0x030x | Rückmeldungen der Heizung |
| Verhalten | Keine Regelung, feste Leistung | Automatische Regelung mit Lüfternachlauf | Heizen bis Solltemperatur, dann Aus |

---

## 🧠 Zusammenfassung

| Modus | Regelstrategie | Sensorquelle | Typische Verwendung |
|--------|----------------|---------------|---------------------|
| **Thermostatmodus** | Keine Temperaturregelung (feste Leistung) | Keine Regelung (0x04) | Dauerlüftung / manuell |
| **Heizen + Lüften** | Regelung nach Panelsensor | Panel (0x02) | Komfortmodus, automatisch |
| **Temperaturmodus** | Heizen bis Sollwert, danach Aus | Panel (0x02) | Klassischer Heizmodus |

---

© 2025 – Analyse basierend auf realen Logs der Autoterm Air 2D.
