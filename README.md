# 🔥 Autoterm UART Bridge für ESPHome

Dieses Projekt implementiert eine vollständige **UART-Bridge zwischen Autoterm/Planar-Heizungen und dem Bedienteil**, mit Integration in **ESPHome** und damit Home Assistant.  
Es erlaubt das **Überwachen und Steuern der Heizung** direkt über WLAN, MQTT oder Home Assistant Entities.

---

> [!WARNING]
> **Experimentelles Projekt – Nutzung auf eigene Gefahr!**  
> Dieses Repository befindet sich **noch in Entwicklung**. Falsche Verdrahtung, fehlerhafte Konfiguration oder unvorhersehbares Verhalten der Firmware können die **Heizung beschädigen**.
>
> Wenn du dir unsicher bist: **nicht verwenden**.

---

## 📦 Funktionsübersicht

- 🧭 **Bidirektionale UART-Bridge** zwischen Display und Heizung  
- 📊 **Status- und Sensordaten**: Innen-, Außen-, Heiz- und Paneltemperatur, Spannung, Pumpenfrequenz, Lüfterdrehzahl  
- 🔘 **Steuerfunktionen**:
  - Ein-/Ausschalten der Heizung  
  - Nur Lüften (Fan Mode)  
  - Einstellen von Zieltemperatur, Leistungsstufe und Arbeitszeit  
  - Umschalten der Temperaturquelle  
  - Aktivieren eines „virtuellen Panel“-Modus (Override)  
- 🧩 **Kompatibel mit Home Assistant** (über ESPHome Integration)  
- 🧾 **Debug-Modus**: zeigt empfangene und gesendete UART-Frames in HEX-Darstellung  
- ⚙️ Unterstützt automatische Wiederverbindung und Statusabfrage, wenn kein Display erkannt wird  

---

## ⚙️ Beispielkonfiguration

Die vollständige Beispielkonfiguration findest du in der Datei **`air2d.yaml`**.  
Sie zeigt, wie die Autoterm-UART-Komponente in ESPHome eingebunden wird.  
Passe die Datei unbedingt an deine **eigene Verkabelung, GPIOs und Gerätekonfiguration** an.

---

## 🧠 UART-Kommunikation im Detail
 
Jede Nachricht (Frame) hat folgenden Aufbau:

| Byte-Index | Bedeutung | Beispielwert | Beschreibung |
|-------------|------------|---------------|---------------|
| 0 | Startbyte | `0xAA` | Kennzeichnet den Beginn eines Frames |
| 1 | Gerätekennung | `0x03` / `0x04` | `0x03` = Anfrage an Heizung, `0x04` = Antwort der Heizung |
| 2 | Länge des Payloads (in Bytes) | z. B. `0x13` | Anzahl Datenbytes zwischen Header und CRC |
| 3 | ? | `0x00` ||
| 4 | Funktionscode | `0x0F`, `0x02`, `0x03`, … | bestimmt den Typ der Nachricht |
| 5 … N−2 | Nutzdaten | – | variabel je nach Funktionscode |
| N−2, N−1 | CRC | z. B. `0x3A 0E` ||

CRC-Berechnung siehe Quellen.

---

### 🔹 Wichtige Funktionscodes

| Code (`data[4]`) | Richtung | Bedeutung / Zweck | Antwortgröße | Beschreibung |
|------------------|-----------|------------------|---------------|---------------|
| `0x0F` | Heizung → Display | **Statusmeldung** | 0x13 Bytes | enthält Temperaturen, Spannung, Lüfter- und Pumpenwerte sowie Statuscode |
| `0x02` | Heizung → Display | **Einstellungen (Settings)** | 6 Bytes | liefert aktuelle Parameter wie Temp-Quelle, Solltemp, Leistung usw. |
| `0x03` | Display → Heizung | **Power-Off-Kommando** | – | beendet Heizvorgang |
| `0x01` | Display → Heizung | **Power-ON mit Settings** | 6 Bytes | schaltet ein und überträgt aktuelle Settings |
| `0x11` | Display ↔ Heizung | **Panel-Temperatur (Messwert)** | 1 Byte | realer oder virtueller Panel-Sensorwert (0–255 °C) |
| `0x23` | Display → Heizung | **Fan-Mode-Start** | 4 Bytes | aktiviert „Nur Lüften“ mit bestimmter Drehzahl |
| `0x02` | Display → Heizung | **Settings-Schreiben** | 6 Bytes | neue Soll-Werte an Heizung übertragen |

---

### 🔸 Beispiel: Status-Frame (`0x0F`)

**Richtung:** Heizung → Display

**Beispiel (aus Log):**

```
AA 04 13 00 0F 00 01 00 11 7F 00 84 01 24 00 00 00 00 00 00 00 00 00 65 3A 0E
```

| Offset | Feld | Beispiel | Bedeutung |
|---------|------|-----------|-----------|
| 5–6 | Statuscode | `00 01` | 0x0001 = „standby“ |
| 7 | (reserviert) | `00` | – |
| 8 | Interne Temperatur | `11` = 17 °C |
| 9 | Externe Temperatur | `7F` = 127 → −1 °C |
| 10 | Spannung (high) | `00` | – |
| 11 | Spannung (low) | `84` → 13.2 V (geteilt durch 10) |
| 12 | Lüfter Sollwert (raw) | `01` × 60 = 60 rpm |
| 13 | Lüfter Istwert (raw) | `24` × 60 = 2160 rpm |
| 14 | Pumpenfrequenz (raw) | `00` → 0.00 Hz (geteilt durch 100) |
| 15–22 | Reserviert | `00 …` | ungenutzt |
| 23–24 | CRC16 | `3A 0E` | gültig |

**Bekannte Statuscodes:**

| Code | Beschreibung |
|------|---------------|
| `0x0001` | standby |
| `0x0100` | cooling flame sensor |
| `0x0101` | ventilation |
| `0x0200` | prepare heating |
| `0x0201` | heating glow plug |
| `0x0202` | ignition 1 |
| `0x0203` | ignition 2 |
| `0x0204` | heating combustion chamber |
| `0x0300` | heating |
| `0x0323` | only fan |
| `0x0304` | cooling down |
| `0x0400` | shutting down |
| *andere* | unknown (HEX-Code wird mit angezeigt) |

---

### 🔸 Beispiel: Settings-Frame (`0x02`)

**Richtung:** Heizung → Display

```
AA 04 06 00 02 [use_work_time] [work_time] [temp_source] [set_temp] [wait_mode] [power_level] CRC_H CRC_L
```

**Beispiel:**

```
AA 04 06 00 02 00 78 02 0F 00 05 39 3D
```

| Byte | Bedeutung | Wert | Kommentar |
|------|------------|------|-----------|
| 5 | `use_work_time` | `00` | 0 = aktiv, 1 = deaktiv |
| 6 | `work_time` | `78` (120 min) | Arbeitszeit |
| 7 | `temperature_source` | `02` | 1 = intern, 2 = Panel, 3 = extern, 4 = keine Regelung |
| 8 | `set_temperature` | `0F` (15 °C) | Solltemperatur |
| 9 | `wait_mode` | `00` | 1 = Warten an, 2 = aus |
| 10 | `power_level` | `05` (Stufe 5) | Leistungsstufe 0–9 |
| 11–12 | CRC16 | `39 3D` | korrekt |

---

### 🔸 Beispiel: Panel-Temperatur-Frame (`0x11`)

**Richtung:** Display → Heizung

```
AA 03 01 00 11 [temp_raw] CRC_H CRC_L
```

- `temp_raw` = 0–255 → 0–255 °C  
- Wird alle 2 s übertragen (oder vom ESP simuliert, wenn „Virtual Panel Override“ aktiv ist)

**Beispiel:**

```
AA 04 01 00 11 10 B1 E5
```

→ Temperatur 16 °C

---

### 🔸 Power ON / OFF

**Power ON**

```
AA 03 06 00 01 [use_work_time] [work_time] [temp_src] [set_temp] [wait_mode] [power_lvl] CRC_H CRC_L
```

**Power OFF**

```
AA 03 00 00 03 CRC_H CRC_L
```

---

### 🔸 Fan Mode (Nur Lüften)

```
AA 03 04 00 23 FF FF [level] FF CRC_H CRC_L
```

- Aktiviert „Fan Mode“ (nur Lüfterbetrieb)  
- `level` = 0–9 (Leistungsstufe)

**Beispiel:**

```
AA 03 04 00 23 FF FF 08 FF 1A 2B
```

---

### 🔸 Anfrage-Frames vom ESP (bei fehlendem Display)

Wenn der ESP kein Bedienteil erkennt, sendet er regelmäßig eigene Requests:

| Funktion | Intervall | Frame | Zweck |
|-----------|------------|--------|--------|
| **Status-Request** | alle 2 s | `AA 03 00 00 0F CRC` | fordert aktuellen Heizstatus an |
| **Settings-Request** | alle 10 s | `AA 03 00 00 02 CRC` | fordert aktuelle Einstellungen an |

---

### 🔹 Übersicht aller bekannten Telegrammtypen

| Code | Richtung | Länge (Payload) | Zweck |
|------|-----------|----------------|-------|
| `0x0F` | Heater → Display | 19 B | Statusdaten |
| `0x02` | Heater → Display | 6 B | Settings lesen |
| `0x02` | Display → Heater | 6 B | Settings schreiben |
| `0x01` | Display → Heater | 6 B | Power ON |
| `0x03` | Display → Heater | 0 B | Power OFF |
| `0x11` | Display → Heater | 1 B | Panel-Temperatur |
| `0x23` | Display → Heater | 4 B | Fan Mode starten |

---

## 🧩 Entitäten in Home Assistant

| Typ | Name | Beschreibung |
|------|------|--------------|
| Sensor | Interne Temperatur | Temperatursensor im Gerät |
| Sensor | Externe Temperatur | Außentemperaturfühler |
| Sensor | Heizkörpertemperatur | Temperatur im Wärmetauscher |
| Sensor | Panel Temperatur | Rohwert vom Panel (oder virtuell) |
| Sensor | Spannung | Bordnetzspannung |
| Sensor | Lüfter Soll (rpm) | RPM × 60 |
| Sensor | Lüfter Ist (rpm) | RPM × 60 |
| Sensor | Pumpenfrequenz | Hz |
| Sensor | Statuswert (numerisch) | zusammengesetzt aus High/Low |
| Text Sensor | Heizstatus (Text) | „heating“, „standby“ etc. inkl. HEX bei unknown |
| Text Sensor | Temperaturquelle | „internal“, „panel“, „external“, „no automatic…“ |
| Button | Heizung Ein/Aus | Startet oder stoppt den Heizprozess |
| Number | Zieltemperatur | Solltemperatur |
| Number | Lüfterstufe | 0–9 (manuell) |
| Number | Arbeitszeit | 0–255 min |
| Number | Leistungsstufe | 0–9 |
| Switch | Warte-Modus | 1 = on, 2 = off |
| Switch | Virtuelles Panel Override | ESP32 simuliert Panel |
| Select | Temperaturquelle wählen | setzt 1/2/3/4 |

---

## 🧑‍💻 Entwicklung & Tests

Getestet mit:

- **ESP32 DevKit v1**  
- **Autoterm Air 2D**  
- UART-Sniffer-Log zur Protokollanalyse  
- CRC-Validierung nach Modbus-Standard  
- ESPHome 2025.x / Home Assistant 2025.x  

---

## 🛠️ Bekannte Einschränkungen

- Autoterm-Protokoll teilweise reverse-engineered  
- Unbekannte Statuscodes werden als HEX angezeigt  
- UART-Verbindungen müssen elektrisch sauber sein   

---

## 📚 Quellen & Referenzen

- 🔗 [schroeder-robert / autoterm-air-2d-serial-control](https://github.com/schroeder-robert/autoterm-air-2d-serial-control)  
  Reverse Engineering und Steuerung der Autoterm Air 2D über serielle Schnittstelle.

---

## 📄 Lizenz

MIT License © 2025  
Entwickelt von **Tim**
