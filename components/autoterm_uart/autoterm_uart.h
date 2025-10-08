#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/number/number.h"
#include "esphome/components/button/button.h"
#include <vector>

namespace esphome {
namespace autoterm_uart {

using namespace esphome::uart;
using namespace esphome::sensor;

class AutotermUART;  // Vorwärtsdeklaration

// ===================
// Custom Button Class
// ===================
class AutotermPowerOffButton : public button::Button {
  public:
   AutotermUART *parent_{nullptr};
   void setup_parent(AutotermUART *p) { parent_ = p; }
 
  protected:
   void press_action() override;  // implementieren wir unten
 };
 

// ===================
// Custom Switch Class
// ===================
class AutotermFanModeSwitch : public switch_::Switch {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void write_state(bool state) override;  // Implementierung folgt unten
};

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
  Sensor *voltage_sensor_{nullptr};
  Sensor *status_sensor_{nullptr};
  Sensor *fan_speed_set_sensor_{nullptr};
  Sensor *fan_speed_actual_sensor_{nullptr};
  Sensor *pump_frequency_sensor_{nullptr};
  Sensor *temperature_source_sensor_{nullptr};
  Sensor *set_temperature_sensor_{nullptr};
  Sensor *work_time_sensor_{nullptr};
  Sensor *power_level_sensor_{nullptr};
  Sensor *wait_mode_sensor_{nullptr};
  Sensor *use_work_time_sensor_{nullptr};
  text_sensor::TextSensor *status_text_sensor_{nullptr};


  // Steuerobjekte
  AutotermPowerOffButton *power_off_button_{nullptr};
  AutotermFanModeSwitch *fan_mode_switch_{nullptr};
  AutotermFanLevelNumber *fan_level_number_{nullptr};

  void send_power_off();
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
  void set_temperature_source_sensor(Sensor *s) { temperature_source_sensor_ = s; }
  void set_set_temperature_sensor(Sensor *s) { set_temperature_sensor_ = s; }
  void set_work_time_sensor(Sensor *s) { work_time_sensor_ = s; }
  void set_power_level_sensor(Sensor *s) { power_level_sensor_ = s; }
  void set_wait_mode_sensor(Sensor *s) { wait_mode_sensor_ = s; }
  void set_use_work_time_sensor(Sensor *s) { use_work_time_sensor_ = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor_ = s; }


  // Neue Setter mit Rückreferenz
  void set_power_off_button(AutotermPowerOffButton *b) {
    power_off_button_ = b;
    if (b) b->setup_parent(this);
  }
  void set_fan_mode_switch(AutotermFanModeSwitch *s) {
    fan_mode_switch_ = s;
    if (s) s->setup_parent(this);
  }
  void set_fan_level_number(AutotermFanLevelNumber *n) {
    fan_level_number_ = n;
    if (n) n->setup_parent(this);
  }

  void loop() override {
    forward_and_sniff(uart_display_, uart_heater_, "display→heater");
    forward_and_sniff(uart_heater_, uart_display_, "heater→display");
  }

 protected:
  void forward_and_sniff(UARTComponent *src, UARTComponent *dst, const char *tag) {
    static std::vector<uint8_t> buffer;
    if (!src || !dst) return;

    while (src->available()) {
      uint8_t b;
      if (!src->read_byte(&b)) break;
      dst->write_byte(b);
      buffer.push_back(b);

      // Startbyte prüfen
      if (buffer.size() >= 3 && buffer[0] == 0xAA) {
        uint8_t len = buffer[2];
        int total = 5 + len + 2;  // Header + Payload + CRC
        if (buffer.size() >= total) {
          if (validate_crc(buffer)) {
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
};

// ===================
// Methodenimplementierungen
// ===================

// Switch gedrückt → Fan Mode senden
void AutotermFanModeSwitch::write_state(bool state) {
  publish_state(state);
  if (parent_) {
    int level = parent_->fan_level_number_ ? (int)parent_->fan_level_number_->state : 8;
    parent_->send_fan_mode(state, level);
  }
}

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
  if (status_val == 0.1f) status_txt = "standby";
  else if (status_val == 1.0f) status_txt = "cooling flame sensor";
  else if (status_val == 1.1f) status_txt = "ventilation";
  else if (status_val == 2.1f) status_txt = "heating glow plug";
  else if (status_val == 2.2f) status_txt = "ignition 1";
  else if (status_val == 2.3f) status_txt = "ignition 2";
  else if (status_val == 2.4f) status_txt = "heating combustion chamber";
  else if (status_val == 3.0f) status_txt = "heating";
  else if (status_val == 3.35f) status_txt = "only fan";
  else if (status_val == 3.4f) status_txt = "cooling down";
  else if (status_val == 4.0f) status_txt = "shutting down";

  ESP_LOGI("autoterm_uart",
           "Status: %s | U=%.1fV | Heater %.0f°C | Fan %.0f/%.0f rpm | Pump %.2f Hz",
           status_txt, voltage, heater_temp, fan_actual_rpm, fan_set_rpm, pump_freq);

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

    if (use_work_time_sensor_) use_work_time_sensor_->publish_state(use_work_time);
    if (work_time_sensor_) work_time_sensor_->publish_state(work_time);
    if (temperature_source_sensor_) temperature_source_sensor_->publish_state(temp_source);
    if (set_temperature_sensor_) set_temperature_sensor_->publish_state(set_temp);
    if (wait_mode_sensor_) wait_mode_sensor_->publish_state(wait_mode);
    if (power_level_sensor_) power_level_sensor_->publish_state(power_level);
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

  ESP_LOGI("autoterm_uart", "Sent Fan Mode %s, Level %d (CRC %04X)",
           on ? "ON" : "OFF", level, crc);
}

void AutotermPowerOffButton::press_action() {
  ESP_LOGI("autoterm_uart", "Power OFF button pressed");
  if (parent_) parent_->send_power_off();
}



void AutotermUART::send_power_off() {
  if (!this->uart_heater_) return;

  const uint8_t header[] = {0xAA, 0x03, 0x00, 0x00, 0x03};
  std::vector<uint8_t> frame(header, header + 5);

  // CRC16 (Modbus)
  uint16_t crc = 0xFFFF;
  for (auto b : frame) {
    crc ^= b;
    for (int i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
  }
  frame.push_back((crc >> 8) & 0xFF);
  frame.push_back(crc & 0xFF);

  this->uart_heater_->write_array(frame);
  this->uart_heater_->flush();

  ESP_LOGI("autoterm_uart", "Sent Power OFF command (CRC %04X)", crc);
}


}  // namespace autoterm_uart
}  // namespace esphome
