#include "esphome.h"

class AutotermHeaterComponent : public PollingComponent {
 public:
  UARTDevice *uart;

  // Sensoren
  Sensor *temperature_sensor = new Sensor();
  Sensor *voltage_sensor = new Sensor();
  Sensor *status_sensor = new Sensor();

  std::vector<uint8_t> buffer;

  AutotermHeaterComponent(UARTComponent *uart_) : PollingComponent(1000), uart(uart_) {}

  void setup() override {}
  void update() override;
  void process_byte(uint8_t b);
  void parse_message(const std::vector<uint8_t> &msg);
  void request_status();
};
