#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/number/number.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/select/select.h"
#include "esphome/core/time.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <string>
#include <vector>

namespace esphome {
namespace autoterm_uart {

using namespace esphome::uart;
using namespace esphome::sensor;

class AutotermUART;      // Vorwärtsdeklaration
class AutotermClimate;   // Vorwärtsdeklaration

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

class AutotermTempSourceSelect : public select::Select {
 public:
  void set_parent(AutotermUART *parent);
  void publish_for_source(uint8_t source);

 protected:
  void control(const std::string &value) override;

 private:
  AutotermUART *parent_{nullptr};
  const char *option_from_source_(uint8_t source) const;
  uint8_t source_from_option_(const std::string &option) const;
};

// ===================
// Hauptklasse UART
// ===================
class AutotermUART : public Component {
  friend class AutotermTempSourceSelect;

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
  Sensor *panel_temp_override_sensor_{nullptr};
  float panel_temp_override_value_c_{NAN};
  AutotermTempSourceSelect *temp_source_select_{nullptr};
  bool manual_temp_source_active_{false};
  uint8_t manual_temp_source_value_{0};
  float last_internal_temp_c_{NAN};
  float last_external_temp_c_{NAN};


  AutotermFanLevelNumber *fan_level_number_{nullptr};
  AutotermClimate *climate_{nullptr};

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

  void set_panel_temp_override_sensor(Sensor *s);
  void set_temp_source_select(AutotermTempSourceSelect *select);
  void set_temp_source_from_select(uint8_t source);
  void apply_temp_source_from_settings(uint8_t source);
  uint8_t get_manual_temp_source() const { return manual_temp_source_active_ ? manual_temp_source_value_ : 0; }
  uint8_t get_effective_temp_source() const;
  float get_temperature_for_source(uint8_t source) const;

  // Neue Setter mit Rückreferenz
  void set_fan_level_number(AutotermFanLevelNumber *n) {
    fan_level_number_ = n;
    if (n) n->setup_parent(this);
  }
  void set_climate(AutotermClimate *climate);

  // Kommandos für Betriebsarten
  void send_standby();
  void send_power_mode(bool start, uint8_t level);
  void send_temperature_hold_mode(bool start, uint8_t temp_sensor, uint8_t set_temp);
  void send_temperature_to_fan_mode(bool start, uint8_t temp_sensor, uint8_t set_temp);
  void send_fan_only(uint8_t level);
  void send_thermostat_placeholder();

  void loop() override {
    forward_and_sniff(uart_display_, uart_heater_, "display→heater", true);
    forward_and_sniff(uart_heater_, uart_display_, "heater→display");

    uint32_t now = millis();
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

      buffer.push_back(b);

      if (from_display)
        last_display_activity_ = millis();

      // Schraube lose Bytes vor dem Header direkt durch
      while (!buffer.empty() && buffer[0] != 0xAA) {
        dst->write_byte(buffer[0]);
        buffer.erase(buffer.begin());
      }

      while (true) {
        if (buffer.empty())
          break;
        if (buffer[0] != 0xAA)
          break;
        if (buffer.size() < 3)
          break;

        uint8_t len = buffer[2];
        size_t total = 5 + static_cast<size_t>(len) + 2;
        if (buffer.size() < total)
          break;

        std::vector<uint8_t> frame(buffer.begin(), buffer.begin() + total);
        process_frame_(std::move(frame), dst, tag, from_display);
        buffer.erase(buffer.begin(), buffer.begin() + total);
      }

      if (buffer.size() > 64) {
        for (uint8_t byte : buffer)
          dst->write_byte(byte);
        buffer.clear();
      }
    }
  }

  // CRC16 (Modbus)
  bool validate_crc(const std::vector<uint8_t> &data) {
    if (data.size() < 3) return false;
    uint16_t expected = crc16_modbus_(data.data(), data.size() - 2);
    uint16_t recv_crc = (data[data.size() - 2] << 8) | data[data.size() - 1];
    return expected == recv_crc;
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
  void parse_settings(const std::vector<uint8_t> &data, bool from_display);
public:
  void send_fan_mode(bool on, int level);

 protected:
  void request_settings();
  void send_status_request();
  bool is_panel_temperature_frame_(const std::vector<uint8_t> &frame) const;
  void handle_panel_temperature_frame_(const std::vector<uint8_t> &frame);
  void process_frame_(std::vector<uint8_t> frame, UARTComponent *dst, const char *tag, bool from_display);
  bool should_override_panel_temperature_() const;
  void apply_temp_source_override_(std::vector<uint8_t> &frame);
  uint8_t compute_override_temperature_byte_() const;
  void update_crc_(std::vector<uint8_t> &frame);
  bool send_command_(uint8_t command, const std::vector<uint8_t> &payload, const char *log_label);
  uint16_t append_crc_(std::vector<uint8_t> &frame);
  static uint16_t crc16_modbus_(const uint8_t *data, size_t length);

  void publish_temp_source_select_(uint8_t source);
  uint8_t clamp_temp_source_(uint8_t source) const;
  bool should_force_temp_source_() const;
  uint8_t map_source_to_heater_(uint8_t source) const;
};

// ===================
// Climate-Class
// ===================
class AutotermClimate : public climate::Climate {
 public:
  void set_parent(AutotermUART *parent);
  void set_default_level(uint8_t level);
  void set_default_temperature(float temperature_c);
  void set_default_temp_sensor(uint8_t sensor);

  void handle_status_update(uint16_t status_code, float internal_temp);
  void handle_settings_update(const AutotermUART::Settings &settings, bool from_display);

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

 private:
  AutotermUART *parent_{nullptr};
  float target_temperature_c_{20.0f};
  float current_temperature_c_{NAN};
  uint8_t fan_level_{4};
  uint8_t default_temp_sensor_{0x01};
  std::string preset_mode_{"power"};

  static uint8_t clamp_level_(int level);
  static float clamp_temperature_(float temperature);
  std::string fan_mode_label_from_level_(uint8_t level) const;
  uint8_t fan_mode_label_to_level_(const std::string &label) const;
  std::string sanitize_preset_(const std::string &preset) const;
  uint8_t resolve_temp_sensor_() const;
  climate::ClimateMode deduce_mode_from_settings_(const AutotermUART::Settings &settings) const;
  std::string deduce_preset_from_settings_(const AutotermUART::Settings &settings) const;
  void apply_state_(climate::ClimateMode mode, const std::string &preset, uint8_t level, float target_temp);
  void update_action_from_status_(uint16_t status_code);
  static std::string preset_from_enum_(climate::ClimatePreset preset);
  static uint8_t fan_level_from_enum_(climate::ClimateFanMode mode, uint8_t fallback_level);
};

// ===================
// Methodenimplementierungen
// ===================

// Number geändert → Level senden
void AutotermFanLevelNumber::control(float value) {
  publish_state(value);
  if (parent_) parent_->send_fan_mode(true, (int)value);
}

void AutotermTempSourceSelect::set_parent(AutotermUART *parent) {
  parent_ = parent;
  this->traits.set_options({"Intern", "Panel", "Extern", "Home Assistant"});
}

const char *AutotermTempSourceSelect::option_from_source_(uint8_t source) const {
  switch (source) {
    case 1:
      return "Intern";
    case 2:
      return "Panel";
    case 3:
      return "Extern";
    case 4:
      return "Home Assistant";
    default:
      return "Intern";
  }
}

uint8_t AutotermTempSourceSelect::source_from_option_(const std::string &option) const {
  if (option == "Intern" || option == "1")
    return 1;
  if (option == "Panel" || option == "2")
    return 2;
  if (option == "Extern" || option == "3")
    return 3;
  if (option == "Home Assistant" || option == "4")
    return 4;
  return 0;
}

void AutotermTempSourceSelect::publish_for_source(uint8_t source) {
  this->publish_state(option_from_source_(source));
}

void AutotermTempSourceSelect::control(const std::string &value) {
  if (parent_ == nullptr) {
    this->publish_state(value);
    return;
  }
  uint8_t src = source_from_option_(value);
  if (src == 0) {
    ESP_LOGW("autoterm_uart", "Temperature source select received unknown option '%s'", value.c_str());
    parent_->publish_temp_source_select_(parent_->get_manual_temp_source());
    return;
  }
  parent_->set_temp_source_from_select(src);
}

void AutotermUART::set_panel_temp_override_sensor(Sensor *s) {
  panel_temp_override_sensor_ = s;
  if (panel_temp_override_sensor_ != nullptr) {
    panel_temp_override_sensor_->add_on_state_callback([this](float value) {
      this->panel_temp_override_value_c_ = value;
    });
    if (panel_temp_override_sensor_->has_state())
      panel_temp_override_value_c_ = panel_temp_override_sensor_->state;
  }
}

void AutotermUART::set_temp_source_select(AutotermTempSourceSelect *select) {
  temp_source_select_ = select;
  if (temp_source_select_ != nullptr) {
    temp_source_select_->set_parent(this);
    uint8_t initial = manual_temp_source_active_ ? manual_temp_source_value_
                                                : (settings_valid_ ? clamp_temp_source_(settings_.temperature_source)
                                                                   : static_cast<uint8_t>(1));
    publish_temp_source_select_(initial);
  }
}

void AutotermUART::set_temp_source_from_select(uint8_t source) {
  uint8_t clamped = clamp_temp_source_(source);
  bool changed = !manual_temp_source_active_ || manual_temp_source_value_ != clamped;
  manual_temp_source_active_ = true;
  manual_temp_source_value_ = clamped;
  publish_temp_source_select_(clamped);
  if (changed) {
    ESP_LOGI("autoterm_uart", "Temperature source set via select to %u", static_cast<unsigned>(clamped));
    if (climate_ != nullptr)
      climate_->publish_state();
  }
}

void AutotermUART::apply_temp_source_from_settings(uint8_t source) {
  uint8_t clamped = clamp_temp_source_(source);
  settings_.temperature_source = clamped;
  if (!manual_temp_source_active_)
    publish_temp_source_select_(clamped);
}

uint8_t AutotermUART::get_effective_temp_source() const {
  if (manual_temp_source_active_ && manual_temp_source_value_ >= 1 && manual_temp_source_value_ <= 4)
    return manual_temp_source_value_;
  if (settings_valid_)
    return clamp_temp_source_(settings_.temperature_source);
  return 1;
}

float AutotermUART::get_temperature_for_source(uint8_t source) const {
  uint8_t clamped = clamp_temp_source_(source);
  float value = NAN;
  switch (clamped) {
    case 1:
      value = last_internal_temp_c_;
      break;
    case 2:
      value = panel_temp_last_value_c_;
      break;
    case 3:
      value = last_external_temp_c_;
      break;
    case 4:
      value = panel_temp_override_value_c_;
      break;
    default:
      value = last_internal_temp_c_;
      break;
  }
  if (std::isfinite(value))
    return value;
  if (std::isfinite(last_internal_temp_c_))
    return last_internal_temp_c_;
  if (std::isfinite(panel_temp_last_value_c_))
    return panel_temp_last_value_c_;
  if (std::isfinite(last_external_temp_c_))
    return last_external_temp_c_;
  return NAN;
}

void AutotermUART::process_frame_(std::vector<uint8_t> frame, UARTComponent *dst, const char *tag, bool from_display) {
  if (frame.empty())
    return;

  bool valid = validate_crc(frame);
  std::vector<uint8_t> outgoing = frame;

  if (valid && from_display) {
    if (is_panel_temperature_frame_(outgoing) && should_override_panel_temperature_()) {
      if (outgoing.size() > 5) {
        uint8_t original_byte = outgoing[5];
        uint8_t override_byte = compute_override_temperature_byte_();
        if (override_byte != original_byte) {
          outgoing[5] = override_byte;
          update_crc_(outgoing);
          ESP_LOGD("autoterm_uart", "Panel temp override active: %u -> %u (source %.1f°C)",
                   static_cast<unsigned>(original_byte),
                   static_cast<unsigned>(override_byte),
                   panel_temp_override_value_c_);
        }
      }
    }
    apply_temp_source_override_(outgoing);
  }

  if (dst != nullptr && !outgoing.empty()) {
    dst->write_array(outgoing.data(), outgoing.size());
    dst->flush();
  }

  if (!valid) {
    ESP_LOGW("autoterm_uart", "[%s] CRC falsch, weitergeleitet", tag);
    return;
  }

  if (is_panel_temperature_frame_(outgoing))
    handle_panel_temperature_frame_(outgoing);
  log_frame(tag, outgoing);
  parse_status(outgoing);
  parse_settings(outgoing, from_display);
}

void AutotermUART::publish_temp_source_select_(uint8_t source) {
  if (temp_source_select_ != nullptr) {
    uint8_t clamped = clamp_temp_source_(source);
    temp_source_select_->publish_for_source(clamped);
  }
}

uint8_t AutotermUART::clamp_temp_source_(uint8_t source) const {
  if (source < 1)
    return 1;
  if (source > 4)
    return 4;
  return source;
}

bool AutotermUART::should_force_temp_source_() const {
  return manual_temp_source_active_ && manual_temp_source_value_ >= 1 && manual_temp_source_value_ <= 4;
}

uint8_t AutotermUART::map_source_to_heater_(uint8_t source) const {
  switch (clamp_temp_source_(source)) {
    case 1:
      return 0x01;
    case 2:
      return 0x02;
    case 3:
      return 0x03;
    case 4:
      return 0x02;  // Home Assistant meldet sich gegenüber der Heizung als Panelsensor
    default:
      return 0x01;
  }
}

void AutotermUART::apply_temp_source_override_(std::vector<uint8_t> &frame) {
  if (!should_force_temp_source_())
    return;
  if (frame.size() < 7)
    return;
  if (frame[0] != 0xAA || frame[1] != 0x03)
    return;
  uint8_t command = frame[4];
  if (command != 0x01 && command != 0x02)
    return;
  size_t payload_index = 5;
  if (frame.size() <= payload_index + 2)
    return;
  uint8_t desired = map_source_to_heater_(manual_temp_source_value_);
  uint8_t current = frame[payload_index + 2];
  if (current == desired)
    return;
  frame[payload_index + 2] = desired;
  update_crc_(frame);
  ESP_LOGD("autoterm_uart", "Temperature source override active: %u -> %u",
           static_cast<unsigned>(current), static_cast<unsigned>(desired));
}

bool AutotermUART::should_override_panel_temperature_() const {
  if (!std::isfinite(panel_temp_override_value_c_))
    return false;
  uint8_t source = 0;
  if (manual_temp_source_active_ && manual_temp_source_value_ >= 1 && manual_temp_source_value_ <= 4)
    source = manual_temp_source_value_;
  else if (settings_valid_)
    source = settings_.temperature_source;
  if (source != 4)
    return false;
  return true;
}

uint8_t AutotermUART::compute_override_temperature_byte_() const {
  float value = panel_temp_override_value_c_;
  if (!std::isfinite(value))
    value = 0.0f;
  if (value < 0.0f)
    value = 0.0f;
  if (value > 99.0f)
    value = 99.0f;
  return static_cast<uint8_t>(std::round(value));
}

void AutotermUART::update_crc_(std::vector<uint8_t> &frame) {
  if (frame.size() < 3)
    return;
  uint16_t crc = crc16_modbus_(frame.data(), frame.size() - 2);
  frame[frame.size() - 2] = (crc >> 8) & 0xFF;
  frame[frame.size() - 1] = crc & 0xFF;
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
    case 0x0305:
      status_txt = "idle ventilation";
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

  last_internal_temp_c_ = internal_temp;
  last_external_temp_c_ = external_temp;

  if (voltage_sensor_) voltage_sensor_->publish_state(voltage);
  if (status_sensor_) status_sensor_->publish_state(status_val);
  if (status_text_sensor_) status_text_sensor_->publish_state(status_txt);
  if (fan_speed_set_sensor_) fan_speed_set_sensor_->publish_state(fan_set_rpm);
  if (fan_speed_actual_sensor_) fan_speed_actual_sensor_->publish_state(fan_actual_rpm);
  if (pump_frequency_sensor_) pump_frequency_sensor_->publish_state(pump_freq);
  if (climate_) climate_->handle_status_update(status_code, internal_temp);
}

void AutotermUART::parse_settings(const std::vector<uint8_t> &data, bool from_display) {
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
    apply_temp_source_from_settings(s.temperature_source);
    if (climate_) climate_->handle_settings_update(settings_, from_display);
  }
}

void AutotermUART::send_fan_mode(bool on, int level) {
  if (!on) {
    send_standby();
    return;
  }
  int clamped = std::max(0, std::min(level, 9));
  send_fan_only(static_cast<uint8_t>(clamped));
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

uint16_t AutotermUART::crc16_modbus_(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t pos = 0; pos < length; pos++) {
    crc ^= data[pos];
    for (int i = 0; i < 8; i++) {
      if (crc & 0x0001)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }
  return crc;
}

uint16_t AutotermUART::append_crc_(std::vector<uint8_t> &frame) {
  uint16_t crc = crc16_modbus_(frame.data(), frame.size());
  frame.push_back((crc >> 8) & 0xFF);
  frame.push_back(crc & 0xFF);
  return crc;
}

bool AutotermUART::send_command_(uint8_t command, const std::vector<uint8_t> &payload, const char *log_label) {
  if (!uart_heater_) {
    ESP_LOGW("autoterm_uart", "UART heater not configured, skipping command 0x%02X", command);
    return false;
  }

  std::vector<uint8_t> frame;
  frame.reserve(5 + payload.size() + 2);
  frame.push_back(0xAA);
  frame.push_back(0x03);
  frame.push_back(static_cast<uint8_t>(payload.size()));
  frame.push_back(0x00);
  frame.push_back(command);
  frame.insert(frame.end(), payload.begin(), payload.end());

  uint16_t crc = append_crc_(frame);

  uart_heater_->write_array(frame);
  uart_heater_->flush();

  std::string payload_hex;
  char temp[4];
  for (auto byte : payload) {
    snprintf(temp, sizeof(temp), "%02X", byte);
    payload_hex += temp;
    payload_hex += ' ';
  }
  if (!payload_hex.empty())
    payload_hex.pop_back();

  ESP_LOGW("autoterm_uart", "Sent %s (cmd=0x%02X len=%u payload=[%s] crc=%04X)",
           log_label != nullptr ? log_label : "frame",
           command, static_cast<unsigned>(payload.size()), payload_hex.c_str(), crc);
  return true;
}

void AutotermUART::send_standby() {
  send_command_(0x03, {}, "mode.standby");
}

void AutotermUART::send_power_mode(bool start, uint8_t level) {
  uint8_t clamped_level = std::min<uint8_t>(level, 9);
  std::vector<uint8_t> payload{0xFF, 0xFF, 0x04, 0xFF, 0x02, clamped_level};
  send_command_(start ? 0x01 : 0x02, payload, start ? "mode.power.start" : "mode.power.set");
}

void AutotermUART::send_temperature_hold_mode(bool start, uint8_t temp_sensor, uint8_t set_temp) {
  uint8_t sensor = map_source_to_heater_(temp_sensor);
  uint8_t temp_byte = std::min<uint8_t>(set_temp, 30);
  std::vector<uint8_t> payload{0xFF, 0xFF, sensor, temp_byte, 0x02, 0xFF};
  send_command_(start ? 0x01 : 0x02, payload, start ? "mode.temp_hold.start" : "mode.temp_hold.set");
}

void AutotermUART::send_temperature_to_fan_mode(bool start, uint8_t temp_sensor, uint8_t set_temp) {
  uint8_t sensor = map_source_to_heater_(temp_sensor);
  uint8_t temp_byte = std::min<uint8_t>(set_temp, 30);
  std::vector<uint8_t> payload{0xFF, 0xFF, sensor, temp_byte, 0x01, 0xFF};
  send_command_(start ? 0x01 : 0x02, payload, start ? "mode.temp_to_fan.start" : "mode.temp_to_fan.set");
}

void AutotermUART::send_fan_only(uint8_t level) {
  uint8_t clamped_level = std::min<uint8_t>(level, 9);
  std::vector<uint8_t> payload{0xFF, 0xFF, clamped_level, 0xFF};
  send_command_(0x23, payload, "mode.fan_only");
}

void AutotermUART::send_thermostat_placeholder() {
  ESP_LOGW("autoterm_uart", "Thermostat mode requested but not implemented yet");
}

void AutotermUART::request_settings() {
  if (send_command_(0x02, {}, "request.settings"))
    last_settings_request_millis_ = millis();
}

void AutotermUART::send_status_request() {
  if (send_command_(0x0F, {}, "request.status"))
    last_status_request_millis_ = millis();
}
 
// ===================
// AutotermClimate Implementierungen
// ===================

void AutotermClimate::set_parent(AutotermUART *parent) {
  parent_ = parent;
  preset_mode_ = sanitize_preset_(preset_mode_);
  this->mode = climate::CLIMATE_MODE_OFF;
  this->action = climate::CLIMATE_ACTION_OFF;
  this->fan_mode.reset();
  fan_level_ = clamp_level_(fan_level_);
  {
    std::string fan_label = fan_mode_label_from_level_(fan_level_);
    if (!fan_label.empty())
      this->custom_fan_mode = fan_label;
    else
      this->custom_fan_mode.reset();
  }
  this->preset.reset();
  if (!preset_mode_.empty())
    this->custom_preset = preset_mode_;
  else
    this->custom_preset.reset();
  target_temperature_c_ = clamp_temperature_(target_temperature_c_);
  this->target_temperature = target_temperature_c_;
  if (!std::isnan(current_temperature_c_))
    this->current_temperature = current_temperature_c_;
  else
    this->current_temperature = NAN;
}

void AutotermClimate::set_default_level(uint8_t level) {
  fan_level_ = clamp_level_(level);
  this->fan_mode.reset();
  std::string fan_label = fan_mode_label_from_level_(fan_level_);
  if (!fan_label.empty())
    this->custom_fan_mode = fan_label;
  else
    this->custom_fan_mode.reset();
}

void AutotermClimate::set_default_temperature(float temperature_c) {
  target_temperature_c_ = clamp_temperature_(temperature_c);
  this->target_temperature = target_temperature_c_;
}

void AutotermClimate::set_default_temp_sensor(uint8_t sensor) {
  if (sensor < 1)
    sensor = 1;
  if (sensor > 4)
    sensor = 4;
  default_temp_sensor_ = sensor;
}

climate::ClimateTraits AutotermClimate::traits() {
  climate::ClimateTraits traits;
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_FAN_ONLY,
      climate::CLIMATE_MODE_AUTO,
  });
  std::set<std::string> presets;
  presets.insert("power");
  presets.insert("temp_hold");
  presets.insert("temp_to_fan");
  presets.insert("thermostat");
  traits.set_supported_custom_presets(std::move(presets));
  std::set<std::string> fan_modes;
  for (int i = 0; i <= 9; i++)
    fan_modes.insert("Stufe " + std::to_string(i));
  traits.set_supported_custom_fan_modes(std::move(fan_modes));
  traits.set_visual_min_temperature(0.0f);
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supports_current_temperature(true);
  return traits;
}

void AutotermClimate::control(const climate::ClimateCall &call) {
  climate::ClimateMode new_mode = this->mode;
  if (call.get_mode().has_value())
    new_mode = *call.get_mode();

  std::string new_preset = preset_mode_;
  bool preset_overridden = false;
  if (call.get_custom_preset().has_value()) {
    new_preset = sanitize_preset_(*call.get_custom_preset());
    preset_overridden = true;
  } else if (call.get_preset().has_value()) {
    new_preset = sanitize_preset_(preset_from_enum_(*call.get_preset()));
    preset_overridden = true;
  }

  if (!preset_overridden) {
    switch (new_mode) {
      case climate::CLIMATE_MODE_HEAT:
        new_preset = "temp_hold";
        break;
      case climate::CLIMATE_MODE_AUTO:
        new_preset = "temp_to_fan";
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
      case climate::CLIMATE_MODE_OFF:
        new_preset.clear();
        break;
      default:
        new_preset = "power";
        break;
    }
    if (!new_preset.empty())
      new_preset = sanitize_preset_(new_preset);
  }

  uint8_t new_level = fan_level_;
  if (call.get_custom_fan_mode().has_value())
    new_level = fan_mode_label_to_level_(*call.get_custom_fan_mode());
  else if (call.get_fan_mode().has_value())
    new_level = fan_level_from_enum_(*call.get_fan_mode(), fan_level_);
  new_level = clamp_level_(new_level);

  float new_target_temp = target_temperature_c_;
  if (call.get_target_temperature().has_value())
    new_target_temp = clamp_temperature_(*call.get_target_temperature());

  ESP_LOGD("autoterm_uart", "Climate control -> mode=%d preset=%s level=%u target=%.1f°C",
           static_cast<int>(new_mode), new_preset.c_str(), new_level, new_target_temp);

  if (!parent_) {
    ESP_LOGW("autoterm_uart", "Climate control requested without parent link");
    apply_state_(new_mode, new_preset, new_level, new_target_temp);
    return;
  }

  climate::ClimateMode previous_mode = this->mode;
  bool should_start = previous_mode == climate::CLIMATE_MODE_OFF ||
                      previous_mode == climate::CLIMATE_MODE_FAN_ONLY ||
                      !parent_->settings_valid_;

  if (new_mode == climate::CLIMATE_MODE_OFF) {
    parent_->send_standby();
  } else if (new_mode == climate::CLIMATE_MODE_FAN_ONLY) {
    if (previous_mode != climate::CLIMATE_MODE_OFF)
      parent_->send_standby();
    parent_->send_fan_only(new_level);
  } else {
    if (previous_mode == climate::CLIMATE_MODE_FAN_ONLY)
      parent_->send_standby();

    if (new_preset == "power") {
      parent_->send_power_mode(should_start, new_level);
    } else if (new_preset == "temp_hold") {
      uint8_t sensor = resolve_temp_sensor_();
      uint8_t temp_byte = static_cast<uint8_t>(std::round(new_target_temp));
      parent_->send_temperature_hold_mode(should_start, sensor, temp_byte);
    } else if (new_preset == "temp_to_fan") {
      uint8_t sensor = resolve_temp_sensor_();
      uint8_t temp_byte = static_cast<uint8_t>(std::round(new_target_temp));
      parent_->send_temperature_to_fan_mode(should_start, sensor, temp_byte);
    } else if (new_preset == "thermostat") {
      parent_->send_thermostat_placeholder();
    }
  }

  apply_state_(new_mode, new_preset, new_level, new_target_temp);
}

void AutotermClimate::handle_status_update(uint16_t status_code, float internal_temp) {
  bool changed = false;
  float display_temp = internal_temp;
  if (parent_ != nullptr) {
    uint8_t source = parent_->get_effective_temp_source();
    float resolved = parent_->get_temperature_for_source(source);
    if (std::isfinite(resolved))
      display_temp = resolved;
  }

  if (!std::isnan(display_temp)) {
    if (std::isnan(current_temperature_c_) || std::fabs(display_temp - current_temperature_c_) > 0.1f) {
      current_temperature_c_ = display_temp;
      this->current_temperature = display_temp;
      changed = true;
    }
  }

  climate::ClimateAction previous_action = this->action;
  update_action_from_status_(status_code);
  if (this->action != previous_action)
    changed = true;

  if (changed)
    this->publish_state();
}

void AutotermClimate::handle_settings_update(const AutotermUART::Settings &settings, bool from_display) {
  if (!from_display)
    return;
  uint8_t level = clamp_level_(settings.power_level);
  float target = clamp_temperature_(static_cast<float>(settings.set_temperature));
  std::string preset = deduce_preset_from_settings_(settings);
  climate::ClimateMode mode = deduce_mode_from_settings_(settings);
  apply_state_(mode, preset, level, target);
}

uint8_t AutotermClimate::clamp_level_(int level) {
  if (level < 0)
    return 0;
  if (level > 9)
    return 9;
  return static_cast<uint8_t>(level);
}

float AutotermClimate::clamp_temperature_(float temperature) {
  if (temperature < 0.0f)
    return 0.0f;
  if (temperature > 30.0f)
    return 30.0f;
  return temperature;
}

std::string AutotermClimate::fan_mode_label_from_level_(uint8_t level) const {
  level = clamp_level_(level);
  return "Stufe " + std::to_string(static_cast<int>(level));
}

uint8_t AutotermClimate::fan_mode_label_to_level_(const std::string &label) const {
  const std::string prefix = "Stufe ";
  if (label.size() <= prefix.size() || label.compare(0, prefix.size(), prefix) != 0)
    return fan_level_;
  std::string digits = label.substr(prefix.size());
  if (digits.empty())
    return fan_level_;
  int value = 0;
  for (char c : digits) {
    if (!std::isdigit(static_cast<unsigned char>(c)))
      return fan_level_;
    value = value * 10 + (c - '0');
  }
  return clamp_level_(value);
}

std::string AutotermClimate::sanitize_preset_(const std::string &preset) const {
  if (preset == "power" || preset == "temp_hold" || preset == "temp_to_fan" || preset == "thermostat")
    return preset;
  return preset_mode_;
}

uint8_t AutotermClimate::resolve_temp_sensor_() const {
  if (parent_ != nullptr) {
    uint8_t manual = parent_->get_manual_temp_source();
    if (manual >= 1 && manual <= 4)
      return manual;
    if (parent_->settings_valid_) {
      uint8_t src = parent_->settings_.temperature_source;
      if (src >= 1 && src <= 4)
        return src;
    }
  }
  uint8_t sensor = default_temp_sensor_;
  if (sensor < 1)
    sensor = 1;
  if (sensor > 4)
    sensor = 4;
  return sensor;
}

climate::ClimateMode AutotermClimate::deduce_mode_from_settings_(const AutotermUART::Settings &settings) const {
  if (settings.wait_mode == 0x00 && settings.power_level == 0)
    return climate::CLIMATE_MODE_OFF;
  if (settings.temperature_source == 0x04)
    return climate::CLIMATE_MODE_HEAT;
  if (settings.wait_mode == 0x01)
    return climate::CLIMATE_MODE_AUTO;
  if (settings.wait_mode == 0x02)
    return climate::CLIMATE_MODE_HEAT;
  return this->mode;
}

std::string AutotermClimate::deduce_preset_from_settings_(const AutotermUART::Settings &settings) const {
  if (settings.temperature_source == 0x04)
    return "power";
  if (settings.wait_mode == 0x01)
    return "temp_to_fan";
  if (settings.wait_mode == 0x02)
    return "temp_hold";
  return preset_mode_;
}

std::string AutotermClimate::preset_from_enum_(climate::ClimatePreset preset) {
  switch (preset) {
    case climate::CLIMATE_PRESET_NONE:
      return "power";
    case climate::CLIMATE_PRESET_HOME:
    case climate::CLIMATE_PRESET_COMFORT:
    case climate::CLIMATE_PRESET_SLEEP:
      return "temp_hold";
    case climate::CLIMATE_PRESET_AWAY:
    case climate::CLIMATE_PRESET_ACTIVITY:
      return "temp_to_fan";
    case climate::CLIMATE_PRESET_BOOST:
      return "power";
    case climate::CLIMATE_PRESET_ECO:
      return "thermostat";
    default:
      return "";
  }
}

uint8_t AutotermClimate::fan_level_from_enum_(climate::ClimateFanMode mode, uint8_t fallback_level) {
  switch (mode) {
    case climate::CLIMATE_FAN_OFF:
      return 0;
    case climate::CLIMATE_FAN_LOW:
      return 1;
    case climate::CLIMATE_FAN_MEDIUM:
      return 4;
    case climate::CLIMATE_FAN_MIDDLE:
      return 5;
    case climate::CLIMATE_FAN_HIGH:
      return 7;
    case climate::CLIMATE_FAN_ON:
      return 9;
    case climate::CLIMATE_FAN_FOCUS:
      return 8;
    case climate::CLIMATE_FAN_DIFFUSE:
      return 3;
    case climate::CLIMATE_FAN_QUIET:
      return 2;
    case climate::CLIMATE_FAN_AUTO:
      return fallback_level;
    default:
      return fallback_level;
  }
}

void AutotermClimate::apply_state_(climate::ClimateMode mode, const std::string &preset, uint8_t level, float target_temp) {
  preset_mode_ = sanitize_preset_(preset);
  fan_level_ = clamp_level_(level);
  target_temperature_c_ = clamp_temperature_(target_temp);

  this->mode = mode;
  this->preset.reset();
  if (mode != climate::CLIMATE_MODE_FAN_ONLY && mode != climate::CLIMATE_MODE_OFF && !preset_mode_.empty())
    this->custom_preset = preset_mode_;
  else
    this->custom_preset.reset();
  this->fan_mode.reset();
  {
    std::string fan_label = fan_mode_label_from_level_(fan_level_);
    if (!fan_label.empty())
      this->custom_fan_mode = fan_label;
    else
      this->custom_fan_mode.reset();
  }
  this->target_temperature = target_temperature_c_;
  if (!std::isnan(current_temperature_c_))
    this->current_temperature = current_temperature_c_;
  else
    this->current_temperature = NAN;

  if (mode == climate::CLIMATE_MODE_OFF)
    this->action = climate::CLIMATE_ACTION_OFF;
  else if (mode == climate::CLIMATE_MODE_FAN_ONLY)
    this->action = climate::CLIMATE_ACTION_FAN;
  else
    this->action = climate::CLIMATE_ACTION_HEATING;

  this->publish_state();
}

void AutotermClimate::update_action_from_status_(uint16_t status_code) {
  climate::ClimateAction action = climate::CLIMATE_ACTION_IDLE;
  switch (status_code) {
    case 0x0000:
    case 0x0001:
      action = this->mode == climate::CLIMATE_MODE_OFF ? climate::CLIMATE_ACTION_OFF
                                                       : climate::CLIMATE_ACTION_IDLE;
      break;
    case 0x0101:
    case 0x0323:
      action = climate::CLIMATE_ACTION_FAN;
      break;
    case 0x0200:
    case 0x0201:
    case 0x0202:
    case 0x0203:
    case 0x0204:
    case 0x0300:
      action = climate::CLIMATE_ACTION_HEATING;
      break;
    case 0x0304:
    case 0x0400:
      action = climate::CLIMATE_ACTION_IDLE;
      break;
    default:
      if (this->mode == climate::CLIMATE_MODE_OFF)
        action = climate::CLIMATE_ACTION_OFF;
      else if (this->mode == climate::CLIMATE_MODE_FAN_ONLY)
        action = climate::CLIMATE_ACTION_FAN;
      else
        action = climate::CLIMATE_ACTION_IDLE;
      break;
  }
  this->action = action;
}

void AutotermUART::set_climate(AutotermClimate *climate) {
  climate_ = climate;
  if (climate_ != nullptr) {
    climate_->set_parent(this);
    if (settings_valid_)
      climate_->handle_settings_update(settings_, false);
  }
}

}  // namespace autoterm_uart
}  // namespace esphome
