#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/core/time.h"
#include <vector>
#include <string>
#include <cmath>

namespace esphome {
namespace autoterm_uart {

using namespace esphome::uart;
using namespace esphome::sensor;

class AutotermUART;  // Vorwärtsdeklaration

// ===================
// Custom Number Class
// ===================
class AutotermFanLevelNumber : public number::Number {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void control(float value) override;  // Implementierung folgt unten
};

// ===================
// Hauptklasse UART
// ===================
class AutotermUART : public Component {
 public:
  UARTComponent *uart_display_{nullptr};
  UARTComponent *uart_heater_{nullptr};

  // Sensoren
  Sensor *internal_temp_sensor_{nullptr};
  Sensor *external_temp_sensor_{nullptr};
  Sensor *heater_temp_sensor_{nullptr};
  Sensor *panel_temp_sensor_{nullptr};
  Sensor *voltage_sensor_{nullptr};
  Sensor *status_sensor_{nullptr};
  Sensor *fan_speed_set_sensor_{nullptr};
  Sensor *fan_speed_actual_sensor_{nullptr};
  Sensor *pump_frequency_sensor_{nullptr};
  text_sensor::TextSensor *status_text_sensor_{nullptr};


  AutotermFanLevelNumber *fan_level_number_{nullptr};

  struct Settings {
    uint8_t use_work_time = 1;
    uint8_t work_time = 0;
    uint8_t temperature_source = 4;
    uint8_t set_temperature = 16;
    uint8_t wait_mode = 0;
    uint8_t power_level = 8;
  } settings_;
  bool settings_valid_{false};
  bool display_connected_state_{false};
  uint32_t last_display_activity_{0};
  uint32_t last_status_request_millis_{0};
  uint32_t last_settings_request_millis_{0};
  float panel_temp_last_value_c_{NAN};
  std::vector<uint8_t> display_to_heater_buffer_;
  std::vector<uint8_t> heater_to_display_buffer_;

  void set_uart_display(UARTComponent *u) { uart_display_ = u; }
  void set_uart_heater(UARTComponent *u) { uart_heater_ = u; }

  // Sensor-Setter
  void set_internal_temp_sensor(Sensor *s) { internal_temp_sensor_ = s; }
  void set_external_temp_sensor(Sensor *s) { external_temp_sensor_ = s; }
  void set_heater_temp_sensor(Sensor *s) { heater_temp_sensor_ = s; }
  void set_voltage_sensor(Sensor *s) { voltage_sensor_ = s; }
  void set_status_sensor(Sensor *s) { status_sensor_ = s; }
  void set_fan_speed_set_sensor(Sensor *s) { fan_speed_set_sensor_ = s; }
  void set_fan_speed_actual_sensor(Sensor *s) { fan_speed_actual_sensor_ = s; }
  void set_pump_frequency_sensor(Sensor *s) { pump_frequency_sensor_ = s; }
  void set_panel_temp_sensor(Sensor *s) {
    panel_temp_sensor_ = s;
    if (s != nullptr && std::isfinite(panel_temp_last_value_c_)) {
      s->publish_state(panel_temp_last_value_c_);
    }
  }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor_ = s; }


  // Neue Setter mit Rückreferenz
  void set_fan_level_number(AutotermFanLevelNumber *n) {
    fan_level_number_ = n;
    if (n) n->setup_parent(this);
  }
  void loop() override {
    uint32_t now = millis();

    forward_and_sniff(uart_display_, uart_heater_, "display→heater", true);
    forward_and_sniff(uart_heater_, uart_display_, "heater→display");

    bool connected = uart_display_ != nullptr && (now - last_display_activity_) < 5000;
    if (connected != display_connected_state_) {
      display_connected_state_ = connected;
      ESP_LOGD("autoterm_uart", "Display connection %s", connected ? "detected" : "lost");
      if (connected) {
        last_status_request_millis_ = now;
        last_settings_request_millis_ = now;
      }
    }

    if (!connected) {
      if (now - last_status_request_millis_ >= 2000) {
        send_status_request();
        last_status_request_millis_ = now;
      }
      if (now - last_settings_request_millis_ >= 10000) {
        request_settings();
        last_settings_request_millis_ = now;
      }
    }
  }

  void setup() override {
    request_settings();
  }

 protected:
  void forward_and_sniff(UARTComponent *src, UARTComponent *dst, const char *tag,
                         bool from_display = false) {
    if (!src || !dst) return;

    auto &buffer = from_display ? display_to_heater_buffer_ : heater_to_display_buffer_;

    while (src->available()) {
      uint8_t b;
      if (!src->read_byte(&b)) break;

      dst->write_byte(b);
      buffer.push_back(b);

      if (from_display)
        last_display_activity_ = millis();

      if (buffer.size() >= 3 && buffer[0] == 0xAA) {
        uint8_t len = buffer[2];
        int total = 5 + len + 2;  // Header + Payload + CRC
        if (buffer.size() >= total) {
          if (validate_crc(buffer)) {
            if (is_panel_temperature_frame_(buffer))
              handle_panel_temperature_frame_(buffer);
            log_frame(tag, buffer);
            parse_status(buffer);
            parse_settings(buffer);
          } else {
            ESP_LOGW("autoterm_uart", "[%s] CRC falsch, verworfen", tag);
          }
          buffer.clear();
        }
      }
      if (buffer.size() > 64 && buffer[0] != 0xAA)
        buffer.clear();
    }
  }

  // CRC16 (Modbus)
  bool validate_crc(const std::vector<uint8_t> &data) {
    if (data.size() < 3) return false;
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < data.size() - 2; pos++) {
      crc ^= data[pos];
      for (int i = 0; i < 8; i++) {
        if (crc & 0x0001)
          crc = (crc >> 1) ^ 0xA001;
        else
          crc >>= 1;
      }
    }
    uint16_t recv_crc = (data[data.size() - 2] << 8) | data[data.size() - 1];
    return crc == recv_crc;
  }

  void log_frame(const char *tag, const std::vector<uint8_t> &data) {
    std::string hex;
    char temp[6];
    for (auto v : data) {
      sprintf(temp, "%02X ", v);
      hex += temp;
    }
    ESP_LOGI("autoterm_uart", "[%s] Frame (%u bytes): %s", tag, (unsigned)data.size(), hex.c_str());
  }

  void parse_status(const std::vector<uint8_t> &data);
  void parse_settings(const std::vector<uint8_t> &data);
public:
  void send_fan_mode(bool on, int level);

 protected:
  void request_settings();
  void send_status_request();
  bool is_panel_temperature_frame_(const std::vector<uint8_t> &frame) const;
  void handle_panel_temperature_frame_(const std::vector<uint8_t> &frame);
};

// ===================
// Methodenimplementierungen
// ===================

// Number geändert → Level senden
void AutotermFanLevelNumber::control(float value) {
  publish_state(value);
  if (parent_) parent_->send_fan_mode(true, (int)value);
}

// ===================
// Bestehende Methoden
// ===================
void AutotermUART::parse_status(const std::vector<uint8_t> &data) {
  if (data.size() < 24) return;
  if (data[1] != 0x04 || data[4] != 0x0F) return;

  const uint8_t *p = &data[5];
  uint8_t s_hi = p[0];
  uint8_t s_lo = p[1];
  uint16_t status_code = (static_cast<uint16_t>(s_hi) << 8) | s_lo;
  uint8_t fan_set_raw = p[11];
  uint8_t fan_actual_raw = p[12];
  uint8_t pump_raw = p[14];
  float status_val = s_hi + (s_lo / 10.0f);
  float internal_temp = (p[3] > 127 ? p[3] - 255 : p[3]);
  float external_temp = (p[4] > 127 ? p[4] - 255 : p[4]);
  float voltage = p[6] / 10.0f;
  float heater_temp = p[8] - 15;
  float fan_set_rpm = fan_set_raw * 60.0f;
  float fan_actual_rpm = fan_actual_raw * 60.0f;
  float pump_freq = pump_raw / 100.0f;

  const char *status_txt = "unknown";
  switch (status_code) {
    case 0x0001:
      status_txt = "standby";
      break;
    case 0x0100:
      status_txt = "cooling flame sensor";
      break;
    case 0x0101:
      status_txt = "ventilation";
      break;
   case 0x0200:
      status_txt = "prepare heating";
      break;
    case 0x0201:
      status_txt = "heating glow plug";
      break;
    case 0x0202:
      status_txt = "ignition 1";
      break;
    case 0x0203:
      status_txt = "ignition 2";
      break;
    case 0x0204:
      status_txt = "heating combustion chamber";
      break;
    case 0x0300:
      status_txt = "heating";
      break;
    case 0x0323:
      status_txt = "only fan";
      break;
    case 0x0304:
      status_txt = "cooling down";
      break;
    case 0x0400:
      status_txt = "shutting down";
      break;
    default:
      // Wenn unbekannter Status, erweitere Textausgabe um HEX-Werte
      static char unknown_buf[32];
      snprintf(unknown_buf, sizeof(unknown_buf), "unknown (0x%02X%02X)", s_hi, s_lo);
      status_txt = unknown_buf;
      break;
  }

  ESP_LOGD("autoterm_uart",
           "Status: %s (0x%02X%02X) | U=%.1fV | Heater %.0f°C | Fan %.0f/%.0f rpm | Pump %.2f Hz",
           status_txt, s_hi, s_lo, voltage, heater_temp, fan_actual_rpm, fan_set_rpm, pump_freq);

  if (internal_temp_sensor_) internal_temp_sensor_->publish_state(internal_temp);
  if (external_temp_sensor_) external_temp_sensor_->publish_state(external_temp);
  if (heater_temp_sensor_) heater_temp_sensor_->publish_state(heater_temp);
  if (voltage_sensor_) voltage_sensor_->publish_state(voltage);
  if (status_sensor_) status_sensor_->publish_state(status_val);
  if (status_text_sensor_) status_text_sensor_->publish_state(status_txt);
  if (fan_speed_set_sensor_) fan_speed_set_sensor_->publish_state(fan_set_rpm);
  if (fan_speed_actual_sensor_) fan_speed_actual_sensor_->publish_state(fan_actual_rpm);
  if (pump_frequency_sensor_) pump_frequency_sensor_->publish_state(pump_freq);
}

void AutotermUART::parse_settings(const std::vector<uint8_t> &data) {
  if (data.size() < 13) return;
  if (data.size() >= 5 && data[1] == 0x04 && data[4] == 0x02) {
    const uint8_t *p = &data[5];
    uint8_t use_work_time = p[0];
    uint8_t work_time = p[1];
    uint8_t temp_source = p[2];
    uint8_t set_temp = p[3];
    uint8_t wait_mode = p[4];
    uint8_t power_level = p[5];

    ESP_LOGD("autoterm_uart",
             "Settings: use_work_time=%d work_time=%d temp_src=%d set_temp=%d wait_mode=%d level=%d",
             use_work_time, work_time, temp_source, set_temp, wait_mode, power_level);
    Settings s{};
    s.use_work_time = use_work_time;
    s.work_time = work_time;
    s.temperature_source = temp_source;
    s.set_temperature = set_temp;
    s.wait_mode = wait_mode;
    s.power_level = power_level;
    settings_ = s;
    settings_valid_ = true;
  }
}

void AutotermUART::send_fan_mode(bool on, int level) {
  if (!uart_heater_) return;
  uint8_t payload[4] = {0xFF, 0xFF, (uint8_t)level, 0xFF};
  uint8_t header[5] = {0xAA, 0x03, 0x04, 0x00, 0x23};

  std::vector<uint8_t> frame;
  frame.insert(frame.end(), header, header + 5);
  frame.insert(frame.end(), payload, payload + 4);

  // CRC16 (Modbus)
  uint16_t crc = 0xFFFF;
  for (auto b : frame) {
    crc ^= b;
    for (int i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
  }
  frame.push_back((crc >> 8) & 0xFF);
  frame.push_back(crc & 0xFF);

  uart_heater_->write_array(frame);
  uart_heater_->flush();

  ESP_LOGD("autoterm_uart", "Sent Fan Mode %s, Level %d (CRC %04X)",
           on ? "ON" : "OFF", level, crc);
}

bool AutotermUART::is_panel_temperature_frame_(const std::vector<uint8_t> &frame) const {
  if (frame.size() < 8)
    return false;
  if (frame[0] != 0xAA)
    return false;
  if (frame[1] != 0x03 && frame[1] != 0x04)
    return false;
  if (frame[2] != 0x01)
    return false;
  if (frame[3] != 0x00)
    return false;
  if (frame[4] != 0x11)
    return false;
  return true;
}

void AutotermUART::handle_panel_temperature_frame_(const std::vector<uint8_t> &frame) {
  if (frame.size() < 6)
    return;

  uint8_t raw = frame[5];
  float temperature_c = static_cast<float>(raw);
  panel_temp_last_value_c_ = temperature_c;

  if (panel_temp_sensor_ != nullptr)
    panel_temp_sensor_->publish_state(temperature_c);
}

void AutotermUART::request_settings() {
  if (!uart_heater_) return;
  const uint8_t header[] = {0xAA, 0x03, 0x00, 0x00, 0x02};
  std::vector<uint8_t> frame(header, header + sizeof(header));

  uint16_t crc = 0xFFFF;
  for (auto b : frame) {
    crc ^= b;
    for (int i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
  }
  frame.push_back((crc >> 8) & 0xFF);
  frame.push_back(crc & 0xFF);

  uart_heater_->write_array(frame);
  uart_heater_->flush();

  ESP_LOGD("autoterm_uart", "Requested settings (CRC %04X)", crc);
  last_settings_request_millis_ = millis();
}

void AutotermUART::send_status_request() {
  if (!uart_heater_) return;

  const uint8_t header[] = {0xAA, 0x03, 0x00, 0x00, 0x0F};
  std::vector<uint8_t> frame(header, header + sizeof(header));

  uint16_t crc = 0xFFFF;
  for (auto b : frame) {
    crc ^= b;
    for (int i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
  }
  frame.push_back((crc >> 8) & 0xFF);
  frame.push_back(crc & 0xFF);

  uart_heater_->write_array(frame);
  uart_heater_->flush();

  ESP_LOGD("autoterm_uart", "Requested status (CRC %04X)", crc);
  last_status_request_millis_ = millis();
}


}  // namespace autoterm_uart
}  // namespace esphome
