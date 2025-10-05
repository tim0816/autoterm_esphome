#include "autoterm_serial.h"

/// CRC16 Modbus (Little Endian)
uint16_t crc16_modbus(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t pos = 0; pos < length; pos++) {
    crc ^= (uint16_t)data[pos];
    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void AutotermHeaterComponent::update() {
  while (uart->available()) {
    uint8_t b = uart->read();
    process_byte(b);
  }
}

void AutotermHeaterComponent::process_byte(uint8_t b) {
  buffer.push_back(b);

  if (buffer.size() >= 5) {  // Mindestlänge: Preamble + device + len + 2
    if (buffer[0] != 0xAA) {
      buffer.clear();
      return;
    }

    uint8_t len = buffer[2]; // Payload-Länge
    size_t total_len = 1 + 1 + 1 + len + 2 + 2; // Preamble + device + len + payload + msg_id + CRC

    if (buffer.size() == total_len) {
      uint16_t crc_calc = crc16_modbus(buffer.data(), total_len - 2);
      uint16_t crc_recv = buffer[total_len - 2] | (buffer[total_len - 1] << 8);
      if (crc_calc == crc_recv) {
        parse_message(buffer);
      } else {
        ESP_LOGW("autoterm", "CRC Fehler: recv=0x%04X calc=0x%04X", crc_recv, crc_calc);
      }
      buffer.clear();
    }
  }
}

void AutotermHeaterComponent::parse_message(const std::vector<uint8_t> &msg) {
  uint8_t device = msg[1];
  uint8_t len = msg[2];
  uint8_t msg_id1 = msg[3];
  uint8_t msg_id2 = msg[4];

  ESP_LOGD("autoterm", "Nachricht dev=0x%02X id=%02X%02X len=%d", device, msg_id1, msg_id2, len);

  // Beispiel: msg_id2 = 0x10 = Statuspaket
  if (msg_id2 == 0x10 && len >= 6) {
    int16_t temp_raw = msg[5] | (msg[6] << 8);
    float temp = temp_raw / 10.0f;
    temperature_sensor->publish_state(temp);

    uint16_t volt_raw = msg[7] | (msg[8] << 8);
    float volt = volt_raw / 1000.0f;
    voltage_sensor->publish_state(volt);

    status_sensor->publish_state(msg[9]);  // Statuscode
  }
}

void AutotermHeaterComponent::request_status() {
  std::vector<uint8_t> pkt = {0xAA, 0x03, 0x02, 0x00, 0x10, 0x00, 0x00};
  uint16_t crc = crc16_modbus(pkt.data(), pkt.size());
  pkt.push_back(crc & 0xFF);
  pkt.push_back((crc >> 8) & 0xFF);

  uart->write_array(pkt.data(), pkt.size());
  ESP_LOGD("autoterm", "Status-Anfrage gesendet");
}
