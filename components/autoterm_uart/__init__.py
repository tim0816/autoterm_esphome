import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import const
import esphome.components.uart as uart
import esphome.components.sensor as sensor
import esphome.components.text_sensor as text_sensor
import esphome.components.number as number
import esphome.components.button as button

DEPENDENCIES = ["sensor", "text_sensor", "number", "button"]

autoterm_ns = cg.esphome_ns.namespace("autoterm_uart")
AutotermPowerOnButton = autoterm_ns.class_("AutotermPowerOnButton", button.Button)
AutotermPowerOffButton = autoterm_ns.class_("AutotermPowerOffButton", button.Button)
AutotermFanModeButton = autoterm_ns.class_("AutotermFanModeButton", button.Button)
AutotermFanLevelNumber = autoterm_ns.class_("AutotermFanLevelNumber", number.Number)
AutotermUART = autoterm_ns.class_("AutotermUART", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(AutotermUART),
    cv.Required("uart_display_id"): cv.use_id(uart.UARTComponent),
    cv.Required("uart_heater_id"): cv.use_id(uart.UARTComponent),

    cv.Optional("internal_temp"): sensor.sensor_schema(unit_of_measurement="°C", icon="mdi:thermometer"),
    cv.Optional("external_temp"): sensor.sensor_schema(unit_of_measurement="°C", icon="mdi:thermometer"),
    cv.Optional("heater_temp"): sensor.sensor_schema(unit_of_measurement="°C", icon="mdi:thermometer"),
    cv.Optional("voltage"): sensor.sensor_schema(unit_of_measurement="V", icon="mdi:flash"),
    cv.Optional("status"): sensor.sensor_schema(icon="mdi:information"),
    cv.Optional("fan_speed_set"): sensor.sensor_schema(unit_of_measurement="rpm", icon="mdi:fan"),
    cv.Optional("fan_speed_actual"): sensor.sensor_schema(unit_of_measurement="rpm", icon="mdi:fan"),
    cv.Optional("pump_frequency"): sensor.sensor_schema(unit_of_measurement="Hz", icon="mdi:water-pump"),

    cv.Optional("status_text"): text_sensor.text_sensor_schema(icon="mdi:information"),
    cv.Optional("temperature_source"): text_sensor.text_sensor_schema(icon="mdi:thermometer"),
    cv.Optional("set_temperature"): sensor.sensor_schema(unit_of_measurement="°C", icon="mdi:thermometer"),
    cv.Optional("work_time"): sensor.sensor_schema(unit_of_measurement="min", icon="mdi:clock-outline"),
    cv.Optional("power_level"): sensor.sensor_schema(icon="mdi:fan"),
    cv.Optional("wait_mode"): sensor.sensor_schema(icon="mdi:pause"),
    cv.Optional("use_work_time"): sensor.sensor_schema(icon="mdi:timer-outline"),

    cv.Optional("power_on"): button.button_schema(class_=AutotermPowerOnButton, icon="mdi:power"),
    cv.Optional("power_off"): button.button_schema(class_=AutotermPowerOffButton, icon="mdi:power-standby"),
    cv.Optional("fan_mode"): button.button_schema(class_=AutotermFanModeButton, icon="mdi:fan"),
    cv.Optional("fan_level"): number.number_schema(class_=AutotermFanLevelNumber, icon="mdi:fan-speed-1"),


})


async def to_code(config):
    var = cg.new_Pvariable(config[const.CONF_ID])
    await cg.register_component(var, config)
    disp = await cg.get_variable(config["uart_display_id"])
    heat = await cg.get_variable(config["uart_heater_id"])
    cg.add(var.set_uart_display(disp))
    cg.add(var.set_uart_heater(heat))

    for key, setter in [
        ("internal_temp", "set_internal_temp_sensor"),
        ("external_temp", "set_external_temp_sensor"),
        ("heater_temp", "set_heater_temp_sensor"),
        ("voltage", "set_voltage_sensor"),
        ("status", "set_status_sensor"),
        ("fan_speed_set", "set_fan_speed_set_sensor"),
        ("fan_speed_actual", "set_fan_speed_actual_sensor"),
        ("pump_frequency", "set_pump_frequency_sensor"),
        ("set_temperature", "set_set_temperature_sensor"),
        ("work_time", "set_work_time_sensor"),
        ("power_level", "set_power_level_sensor"),
        ("wait_mode", "set_wait_mode_sensor"),
        ("use_work_time", "set_use_work_time_sensor"),
    ]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(sens))

    for key, setter in [
        ("status_text", "set_status_text_sensor"),
        ("temperature_source", "set_temperature_source_text_sensor"),
    ]:
        if key in config:
            txt = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(var, setter)(txt))

    if "power_on" in config:
        btn = await button.new_button(config["power_on"])
        cg.add(var.set_power_on_button(btn))

    if "power_off" in config:
        btn = await button.new_button(config["power_off"])
        cg.add(var.set_power_off_button(btn))

    if "fan_mode" in config:
        btn = await button.new_button(config["fan_mode"])
        cg.add(var.set_fan_mode_button(btn))

    if "fan_level" in config:
        conf = config["fan_level"]
        # Standardwerte definieren, falls nicht im YAML angegeben
        min_v = conf.get("min_value", 0)
        max_v = conf.get("max_value", 9)
        step_v = conf.get("step", 1)
        num = await number.new_number(conf, min_value=min_v, max_value=max_v, step=step_v)
        cg.add(var.set_fan_level_number(num))
