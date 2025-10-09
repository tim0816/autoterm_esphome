| Modus | Name           | Steuercode der Heizung                             | Beschreibung |
|:------|:---------------|:---------------------------------------------------|:--------------|
| `0x00` | **Standby** | — | Heizung ist aus / wird ausgeschaltet |
| `0x01` | **Leistungsmodus** | `AA 03 06 00 02 FF FF 04 FF 02 00` | Heizung lässt sich über den Power-Level einstellen |
| `0x02` | **Heizen** | `AA 03 06 00 02 FF FF 02 14 02 FF` | Heizung läuft im Temperaturmodus; wenn die Zieltemperatur erreicht ist, läuft sie auf kleinster Stufe weiter |
| `0x03` | **Heizen + Lüften** | `AA 03 06 00 02 FF FF 02 14 01 FF` | Heizung läuft im Temperaturmodus; wenn die Zieltemperatur erreicht ist, schaltet sie automatisch in den Lüften-Modus |
| `0x04` | **Thermostat** | **Ein:** `AA 03 06 00 01 FF FF 04 FF 02 00 2A ED`<br>**Aus:** `AA 03 00 00 03` | Heizung wird im Leistungsmodus betrieben. Die Einstellung der gewählten Leistung erfolgt über den Power-Level. Bei Erreichen der Solltemperatur wird die Heizung per Ausschaltbefehl deaktiviert und bei Unterschreiten der Temperatur wieder eingeschaltet. Es soll noch eine Hysterese geben, sodass die Heizung erst ab 2 °C (einstellbar) unter Solltemperatur wieder startet. |
