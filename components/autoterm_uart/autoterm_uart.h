#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/button/button.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/select/select.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/time.h"
#include <vector>
#include <functional>
#include <string>
#include <algorithm>
#include <cmath>

namespace esphome {
namespace autoterm_uart {

using namespace esphome::uart;
using namespace esphome::sensor;

class AutotermUART;  // Vorwärtsdeklaration

// ===================
// Custom Button Class
// ===================
class AutotermPowerOnButton : public button::Button {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void press_action() override;
};

class AutotermPowerOffButton : public button::Button {
  public:
   AutotermUART *parent_{nullptr};
   void setup_parent(AutotermUART *p) { parent_ = p; }

  protected:
   void press_action() override;  // implementieren wir unten
 };


// ===================
// Custom Button Class (Lüften)
// ===================
class AutotermFanModeButton : public button::Button {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void press_action() override;
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

class AutotermSetTemperatureNumber : public number::Number {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void control(float value) override;
};

class AutotermWorkTimeNumber : public number::Number {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void control(float value) override;
};

class AutotermPowerLevelNumber : public number::Number {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void control(float value) override;
};

class AutotermVirtualPanelTemperatureNumber : public number::Number {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void control(float value) override;
};

class AutotermVirtualPanelOverrideSwitch : public esphome::switch_::Switch {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void write_state(bool state) override;
};

class AutotermTemperatureSourceSelect : public select::Select {
 public:
  AutotermUART *parent_{nullptr};
  AutotermTemperatureSourceSelect() {
    this->traits.set_options({"internal sensor", "panel sensor", "external sensor",
                              "no automatic temperature control"});
  }
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void control(const std::string &value) override;
};

class AutotermUseWorkTimeSwitch : public esphome::switch_::Switch {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void write_state(bool state) override;
};

class AutotermWaitModeSwitch : public esphome::switch_::Switch {
 public:
  AutotermUART *parent_{nullptr};
  void setup_parent(AutotermUART *p) { parent_ = p; }

 protected:
  void write_state(bool state) override;
};

// ===================
// Hauptklasse UART
// ===================
class AutotermUART : public Component {
  friend class AutotermVirtualPanelTemperatureNumber;
  friend class AutotermVirtualPanelOverrideSwitch;
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
  text_sensor::TextSensor *temperature_source_text_sensor_{nullptr};
  Sensor *set_temperature_sensor_{nullptr};
  Sensor *work_time_sensor_{nullptr};
  Sensor *power_level_sensor_{nullptr};
  Sensor *wait_mode_sensor_{nullptr};
  Sensor *use_work_time_sensor_{nullptr};
  text_sensor::TextSensor *status_text_sensor_{nullptr};
  binary_sensor::BinarySensor *display_connected_sensor_{nullptr};
  Sensor *virtual_panel_temp_sensor_{nullptr};


  // Steuerobjekte
  AutotermPowerOnButton *power_on_button_{nullptr};
  AutotermPowerOffButton *power_off_button_{nullptr};
  AutotermFanModeButton *fan_mode_button_{nullptr};
  AutotermFanLevelNumber *fan_level_number_{nullptr};
  AutotermSetTemperatureNumber *set_temperature_number_{nullptr};
  AutotermWorkTimeNumber *work_time_number_{nullptr};
  AutotermPowerLevelNumber *power_level_number_{nullptr};
  AutotermVirtualPanelTemperatureNumber *virtual_panel_temp_number_{nullptr};
  AutotermVirtualPanelOverrideSwitch *virtual_panel_override_switch_{nullptr};
  AutotermTemperatureSourceSelect *temperature_source_select_{nullptr};
  AutotermUseWorkTimeSwitch *use_work_time_switch_{nullptr};
  AutotermWaitModeSwitch *wait_mode_switch_{nullptr};

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
  bool virtual_panel_override_enabled_{false};
  bool virtual_panel_value_valid_{false};
  uint8_t virtual_panel_last_raw_{0};
  float virtual_panel_last_value_c_{NAN};
  float panel_temp_last_value_c_{NAN};
  uint32_t last_virtual_panel_send_millis_{0};
  std::vector<uint8_t> display_to_heater_buffer_;
  std::vector<uint8_t> heater_to_display_buffer_;

  void send_power_on();
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
  void set_panel_temp_sensor(Sensor *s) {
    panel_temp_sensor_ = s;
    if (s != nullptr && std::isfinite(panel_temp_last_value_c_)) {
      s->publish_state(panel_temp_last_value_c_);
    }
  }
  void set_temperature_source_text_sensor(text_sensor::TextSensor *s) {
    temperature_source_text_sensor_ = s;
  }
  void set_set_temperature_sensor(Sensor *s) { set_temperature_sensor_ = s; }
  void set_work_time_sensor(Sensor *s) { work_time_sensor_ = s; }
  void set_power_level_sensor(Sensor *s) { power_level_sensor_ = s; }
  void set_wait_mode_sensor(Sensor *s) { wait_mode_sensor_ = s; }
  void set_use_work_time_sensor(Sensor *s) { use_work_time_sensor_ = s; }
  void set_status_text_sensor(text_sensor::TextSensor *s) { status_text_sensor_ = s; }
  void set_display_connected_sensor(binary_sensor::BinarySensor *s) {
    display_connected_sensor_ = s;
  }
  void set_virtual_panel_temp_sensor(Sensor *s) {
    virtual_panel_temp_sensor_ = s;
    if (s != nullptr) {
      s->add_on_state_callback([this](float value) { this->send_virtual_panel_temperature(value); });
      if (s->has_state()) {
        this->send_virtual_panel_temperature(s->state);
      }
    }
  }


  // Neue Setter mit Rückreferenz
  void set_power_on_button(AutotermPowerOnButton *b) {
    power_on_button_ = b;
    if (b) b->setup_parent(this);
  }
  void set_power_off_button(AutotermPowerOffButton *b) {
    power_off_button_ = b;
    if (b) b->setup_parent(this);
  }
  void set_fan_mode_button(AutotermFanModeButton *b) {
    fan_mode_button_ = b;
    if (b) b->setup_parent(this);
  }
  void set_fan_level_number(AutotermFanLevelNumber *n) {
    fan_level_number_ = n;
    if (n) n->setup_parent(this);
  }
  void set_set_temperature_number(AutotermSetTemperatureNumber *n) {
    set_temperature_number_ = n;
    if (n) n->setup_parent(this);
  }
  void set_work_time_number(AutotermWorkTimeNumber *n) {
    work_time_number_ = n;
    if (n) n->setup_parent(this);
  }
  void set_power_level_number(AutotermPowerLevelNumber *n) {
    power_level_number_ = n;
    if (n) n->setup_parent(this);
  }
  void set_virtual_panel_temp_number(AutotermVirtualPanelTemperatureNumber *n) {
    virtual_panel_temp_number_ = n;
    if (n) {
      n->setup_parent(this);
      if (virtual_panel_value_valid_)
        n->publish_state(virtual_panel_last_value_c_);
    }
  }
  void set_virtual_panel_override_switch(AutotermVirtualPanelOverrideSwitch *s) {
    virtual_panel_override_switch_ = s;
    if (s) {
      s->setup_parent(this);
      s->publish_state(virtual_panel_override_enabled_);
    }
  }
  void set_temperature_source_select(AutotermTemperatureSourceSelect *s) {
    temperature_source_select_ = s;
    if (s) s->setup_parent(this);
  }
  void set_use_work_time_switch(AutotermUseWorkTimeSwitch *s) {
    use_work_time_switch_ = s;
    if (s) s->setup_parent(this);
  }
  void set_wait_mode_switch(AutotermWaitModeSwitch *s) {
    wait_mode_switch_ = s;
    if (s) s->setup_parent(this);
  }

  void loop() override {
    uint32_t now = millis();

    forward_and_sniff(uart_display_, uart_heater_, "display→heater", true);
    forward_and_sniff(uart_heater_, uart_display_, "heater→display");

    if (virtual_panel_override_enabled_ && virtual_panel_value_valid_ &&
        now - last_virtual_panel_send_millis_ >= 2000) {
      transmit_virtual_panel_temperature_();
    }

    bool connected = uart_display_ != nullptr && (now - last_display_activity_) < 5000;
    if (connected != display_connected_state_) {
      display_connected_state_ = connected;
      ESP_LOGI("autoterm_uart", "Display connection %s", connected ? "detected" : "lost");
      if (display_connected_sensor_)
        display_connected_sensor_->publish_state(connected);
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
    if (display_connected_sensor_)
      display_connected_sensor_->publish_state(false);
    request_settings();
  }

 protected:
  void forward_and_sniff(UARTComponent *src, UARTComponent *dst, const char *tag,
                         bool from_display = false) {
    if (!src || !dst) return;

    auto &buffer = from_display ? display_to_heater_buffer_ : heater_to_display_buffer_;

    if (!from_display || !virtual_panel_override_enabled_) {
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
      return;
    }

    while (src->available()) {
      uint8_t b;
      if (!src->read_byte(&b)) break;

      last_display_activity_ = millis();
      buffer.push_back(b);

      if (buffer.size() == 1 && buffer[0] != 0xAA) {
        dst->write_byte(buffer[0]);
        buffer.clear();
        continue;
      }

      if (buffer.size() < 3 || buffer[0] != 0xAA)
        continue;

      uint8_t len = buffer[2];
      size_t total = 5 + len + 2;
      if (buffer.size() < total)
        continue;

      bool crc_ok = validate_crc(buffer);
      bool drop = false;
      if (crc_ok && is_panel_temperature_frame_(buffer)) {
        drop = true;
        ESP_LOGD("autoterm_uart", "Suppressing panel temperature frame while override active");
        handle_panel_temperature_frame_(buffer);
      }

      if (!drop) {
        dst->write_array(buffer);
      }

      if (crc_ok) {
        log_frame(tag, buffer);
        parse_status(buffer);
        parse_settings(buffer);
      } else {
        ESP_LOGW("autoterm_uart", "[%s] CRC falsch, verworfen", tag);
      }

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
    ESP_LOGD("autoterm_uart", "[%s] Frame (%u bytes): %s", tag, (unsigned)data.size(), hex.c_str());
  }

  void parse_status(const std::vector<uint8_t> &data);
  void parse_settings(const std::vector<uint8_t> &data);
public:
  void send_fan_mode(bool on, int level);
  void set_set_temperature(uint8_t value);
  void set_work_time(uint8_t value);
  void set_power_level(uint8_t value);
  void set_temperature_source(uint8_t value);
  bool set_temperature_source_from_string(const std::string &value);
  void set_use_work_time(bool use);
  void set_wait_mode(bool on);

 protected:
  void request_settings();
  void send_status_request();
  void send_settings(const Settings &settings);
  void publish_settings_(const Settings &settings);
  void update_settings_(const std::function<void(Settings &)> &updater);
  std::string temperature_source_to_string(uint8_t value) const;
  uint8_t temperature_source_from_string(const std::string &value) const;
  void send_virtual_panel_temperature(float value);
  void transmit_virtual_panel_temperature_();
  void set_virtual_panel_override_enabled_(bool enabled);
  bool is_panel_temperature_frame_(const std::vector<uint8_t> &frame) const;
  void handle_panel_temperature_frame_(const std::vector<uint8_t> &frame);
};

// ===================
// Methodenimplementierungen
// ===================

// Button gedrückt → Lüften aktivieren
void AutotermPowerOnButton::press_action() {
  ESP_LOGI("autoterm_uart", "Power ON button pressed");
  if (parent_) parent_->send_power_on();
}

void AutotermFanModeButton::press_action() {
  ESP_LOGI("autoterm_uart", "Fan Mode button pressed");
  if (parent_) {
    int level = parent_->fan_level_number_ ? (int)parent_->fan_level_number_->state : 8;
    parent_->send_fan_mode(true, level);
  }
}

// Number geändert → Level senden
void AutotermFanLevelNumber::control(float value) {
  publish_state(value);
  if (parent_) parent_->send_fan_mode(true, (int)value);
}

void AutotermSetTemperatureNumber::control(float value) {
  publish_state(value);
  if (parent_) parent_->set_set_temperature(static_cast<uint8_t>(value));
}

void AutotermWorkTimeNumber::control(float value) {
  publish_state(value);
  if (parent_) parent_->set_work_time(static_cast<uint8_t>(value));
}

void AutotermPowerLevelNumber::control(float value) {
  publish_state(value);
  if (parent_) parent_->set_power_level(static_cast<uint8_t>(value));
}

void AutotermVirtualPanelTemperatureNumber::control(float value) {
  publish_state(value);
  if (parent_ == nullptr) {
    return;
  }
  parent_->send_virtual_panel_temperature(value);
}

void AutotermVirtualPanelOverrideSwitch::write_state(bool state) {
  publish_state(state);
  if (parent_)
    parent_->set_virtual_panel_override_enabled_(state);
}

void AutotermTemperatureSourceSelect::control(const std::string &value) {
  if (parent_ == nullptr) {
    ESP_LOGW("autoterm_uart", "Temperature source select has no parent");
    return;
  }

  if (parent_->set_temperature_source_from_string(value)) {
    publish_state(value);
  }
}

void AutotermUseWorkTimeSwitch::write_state(bool state) {
  publish_state(state);
  if (parent_) parent_->set_use_work_time(state);
}

void AutotermWaitModeSwitch::write_state(bool state) {
  publish_state(state);
  if (parent_) parent_->set_wait_mode(state);
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

  ESP_LOGI("autoterm_uart",
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
    publish_settings_(settings_);
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

void AutotermUART::set_set_temperature(uint8_t value) {
  update_settings_([value](Settings &s) { s.set_temperature = value; });
}

void AutotermUART::set_work_time(uint8_t value) {
  update_settings_([value](Settings &s) { s.work_time = value; });
}

void AutotermUART::set_power_level(uint8_t value) {
  update_settings_([value](Settings &s) { s.power_level = value; });
}

void AutotermUART::set_temperature_source(uint8_t value) {
  update_settings_([value](Settings &s) { s.temperature_source = value; });
}

bool AutotermUART::set_temperature_source_from_string(const std::string &value) {
  uint8_t numeric_value = temperature_source_from_string(value);
  if (numeric_value == 0) {
    ESP_LOGW("autoterm_uart", "Unknown temperature source option: %s", value.c_str());
    return false;
  }

  set_temperature_source(numeric_value);
  return true;
}

void AutotermUART::set_use_work_time(bool use) {
  update_settings_([use](Settings &s) { s.use_work_time = use ? 0 : 1; });
}

void AutotermUART::set_wait_mode(bool on) {
  update_settings_([on](Settings &s) { s.wait_mode = on ? 1 : 2; });
}

void AutotermUART::send_virtual_panel_temperature(float value) {
  if (!std::isfinite(value)) {
    ESP_LOGW("autoterm_uart", "Ignoring non-finite virtual panel temperature %.2f", value);
    return;
  }

  float clamped = std::clamp(value, -40.0f, 215.0f);
  int raw = static_cast<int>(std::lround(clamped));
  raw = std::clamp(raw, 0, 255);

  if (virtual_panel_temp_number_ != nullptr)
    virtual_panel_temp_number_->publish_state(clamped);

  virtual_panel_last_raw_ = static_cast<uint8_t>(raw);
  virtual_panel_last_value_c_ = clamped;
  virtual_panel_value_valid_ = true;
  if (virtual_panel_override_enabled_) {
    last_virtual_panel_send_millis_ = 0;  // force immediate send on next loop
  }
}

void AutotermUART::transmit_virtual_panel_temperature_() {
  if (!virtual_panel_value_valid_) {
    ESP_LOGW("autoterm_uart", "No virtual panel value to transmit");
    last_virtual_panel_send_millis_ = millis();
    return;
  }

  if (!uart_heater_) {
    ESP_LOGW("autoterm_uart", "Cannot send virtual panel temperature without heater UART");
    last_virtual_panel_send_millis_ = millis();
    return;
  }

  std::vector<uint8_t> frame = {0xAA, 0x03, 0x01, 0x00, 0x11, virtual_panel_last_raw_};

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

  ESP_LOGI("autoterm_uart",
           "Sent virtual panel temperature override %.2f°C (raw=%u, CRC %04X)",
           virtual_panel_last_value_c_, virtual_panel_last_raw_, crc);

  last_virtual_panel_send_millis_ = millis();
}

void AutotermUART::set_virtual_panel_override_enabled_(bool enabled) {
  if (virtual_panel_override_enabled_ == enabled)
    return;

  virtual_panel_override_enabled_ = enabled;
  ESP_LOGI("autoterm_uart", "Virtual panel override %s", enabled ? "enabled" : "disabled");

  if (virtual_panel_override_switch_)
    virtual_panel_override_switch_->publish_state(enabled);

  if (enabled) {
    last_virtual_panel_send_millis_ = 0;
    display_to_heater_buffer_.clear();
  } else {
    display_to_heater_buffer_.clear();
  }
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

void AutotermUART::send_settings(const Settings &settings) {
  if (!uart_heater_) return;
  uint8_t header[5] = {0xAA, 0x03, 0x06, 0x00, 0x02};
  uint8_t payload[6] = {settings.use_work_time,
                        settings.work_time,
                        settings.temperature_source,
                        settings.set_temperature,
                        settings.wait_mode,
                        settings.power_level};

  std::vector<uint8_t> frame(header, header + 5);
  frame.insert(frame.end(), payload, payload + 6);

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

  ESP_LOGD("autoterm_uart",
           "Sent settings: use_work_time=%u work_time=%u temp_src=%u set_temp=%u wait_mode=%u level=%u (CRC %04X)",
           settings.use_work_time, settings.work_time, settings.temperature_source,
           settings.set_temperature, settings.wait_mode, settings.power_level, crc);

  settings_ = settings;
  settings_valid_ = true;
  publish_settings_(settings_);
}

void AutotermUART::publish_settings_(const Settings &settings) {
  if (use_work_time_sensor_) use_work_time_sensor_->publish_state(settings.use_work_time);
  if (work_time_sensor_) work_time_sensor_->publish_state(settings.work_time);
  if (set_temperature_sensor_) set_temperature_sensor_->publish_state(settings.set_temperature);
  if (wait_mode_sensor_) wait_mode_sensor_->publish_state(settings.wait_mode);
  if (power_level_sensor_) power_level_sensor_->publish_state(settings.power_level);

  std::string temp_source_txt = temperature_source_to_string(settings.temperature_source);
  if (temperature_source_text_sensor_) temperature_source_text_sensor_->publish_state(temp_source_txt);

  if (temperature_source_select_)
    temperature_source_select_->publish_state(temp_source_txt);
  if (set_temperature_number_)
    set_temperature_number_->publish_state(settings.set_temperature);
  if (work_time_number_)
    work_time_number_->publish_state(settings.work_time);
  if (power_level_number_)
    power_level_number_->publish_state(settings.power_level);
  if (use_work_time_switch_)
    use_work_time_switch_->publish_state(settings.use_work_time == 0);
  if (wait_mode_switch_)
    wait_mode_switch_->publish_state(settings.wait_mode == 1);
}

void AutotermUART::update_settings_(const std::function<void(Settings &)> &updater) {
  Settings new_settings = settings_;
  if (!settings_valid_) {
    new_settings = Settings{};
  }
  updater(new_settings);
  send_settings(new_settings);
}

std::string AutotermUART::temperature_source_to_string(uint8_t value) const {
  switch (value) {
    case 1:
      return "internal sensor";
    case 2:
      return "panel sensor";
    case 3:
      return "external sensor";
    case 4:
      return "no automatic temperature control";
    default:
      return "unknown";
  }
}

uint8_t AutotermUART::temperature_source_from_string(const std::string &value) const {
  if (value == "internal sensor")
    return 1;
  if (value == "panel sensor")
    return 2;
  if (value == "external sensor")
    return 3;
  if (value == "no automatic temperature control")
    return 4;
  return 0;
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

  ESP_LOGD("autoterm_uart", "Sent Power OFF command (CRC %04X)", crc);
}

void AutotermUART::send_power_on() {
  if (!this->uart_heater_) return;

  const uint8_t header[] = {0xAA, 0x03, 0x06, 0x00, 0x01};
  Settings payload_settings = settings_;
  if (!settings_valid_) {
    payload_settings = Settings{};
  }

  const uint8_t payload[] = {payload_settings.use_work_time,
                             payload_settings.work_time,
                             payload_settings.temperature_source,
                             payload_settings.set_temperature,
                             payload_settings.wait_mode,
                             payload_settings.power_level};


  std::vector<uint8_t> frame(header, header + 5);
  frame.insert(frame.end(), payload, payload + sizeof(payload));

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

  ESP_LOGD("autoterm_uart", "Sent Power ON command (CRC %04X)", crc);
}


}  // namespace autoterm_uart
}  // namespace esphome
