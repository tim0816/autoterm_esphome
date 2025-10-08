import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import const
import esphome.components.uart as uart
import esphome.components.sensor as sensor
import esphome.components.text_sensor as text_sensor
import esphome.components.number as number
import esphome.components.switch as switch
import esphome.components.select as select
import esphome.components.button as button
import esphome.components.binary_sensor as binary_sensor

DEPENDENCIES = ["sensor", "text_sensor", "number", "button", "switch", "select", "binary_sensor"]

autoterm_ns = cg.esphome_ns.namespace("autoterm_uart")
AutotermPowerOnButton = autoterm_ns.class_("AutotermPowerOnButton", button.Button)
AutotermPowerOffButton = autoterm_ns.class_("AutotermPowerOffButton", button.Button)
AutotermFanModeButton = autoterm_ns.class_("AutotermFanModeButton", button.Button)
AutotermFanLevelNumber = autoterm_ns.class_("AutotermFanLevelNumber", number.Number)
AutotermSetTemperatureNumber = autoterm_ns.class_("AutotermSetTemperatureNumber", number.Number)
AutotermWorkTimeNumber = autoterm_ns.class_("AutotermWorkTimeNumber", number.Number)
AutotermPowerLevelNumber = autoterm_ns.class_("AutotermPowerLevelNumber", number.Number)
AutotermVirtualPanelTemperatureNumber = autoterm_ns.class_(
    "AutotermVirtualPanelTemperatureNumber", number.Number
)
AutotermVirtualPanelOverrideSwitch = autoterm_ns.class_(
    "AutotermVirtualPanelOverrideSwitch", switch.Switch
)
AutotermTemperatureSourceSelect = autoterm_ns.class_("AutotermTemperatureSourceSelect", select.Select)
AutotermUseWorkTimeSwitch = autoterm_ns.class_("AutotermUseWorkTimeSwitch", switch.Switch)
AutotermWaitModeSwitch = autoterm_ns.class_("AutotermWaitModeSwitch", switch.Switch)
AutotermUART = autoterm_ns.class_("AutotermUART", cg.Component)

TEMPERATURE_SOURCE_OPTIONS = [
    "internal sensor",
    "panel sensor",
    "external sensor",
    "no automatic temperature control",
]

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
    cv.Optional("display_connected"): binary_sensor.binary_sensor_schema(icon="mdi:monitor"),
    cv.Optional("virtual_panel_temperature"): cv.use_id(sensor.Sensor),

    cv.Optional("power_on"): button.button_schema(class_=AutotermPowerOnButton, icon="mdi:power"),
    cv.Optional("power_off"): button.button_schema(class_=AutotermPowerOffButton, icon="mdi:power-standby"),
    cv.Optional("fan_mode"): button.button_schema(class_=AutotermFanModeButton, icon="mdi:fan"),
    cv.Optional("fan_level"): number.number_schema(class_=AutotermFanLevelNumber, icon="mdi:fan-speed-1"),
    cv.Optional("set_temperature_control"): number.number_schema(
        class_=AutotermSetTemperatureNumber,
        icon="mdi:thermometer",
    ),
    cv.Optional("work_time_control"): number.number_schema(
        class_=AutotermWorkTimeNumber,
        icon="mdi:clock-outline",
    ),
    cv.Optional("power_level_control"): number.number_schema(
        class_=AutotermPowerLevelNumber,
        icon="mdi:fan",
    ),
    cv.Optional("virtual_panel_temperature_control"): number.number_schema(
        class_=AutotermVirtualPanelTemperatureNumber,
        icon="mdi:thermometer",
    ),
    cv.Optional("virtual_panel_override_switch"): switch.switch_schema(
        class_=AutotermVirtualPanelOverrideSwitch,
        icon="mdi:swap-horizontal",
    ),
    cv.Optional("temperature_source_control"): select.select_schema(
        class_=AutotermTemperatureSourceSelect,
        icon="mdi:thermometer",
    ),
    cv.Optional("use_work_time_switch"): switch.switch_schema(
        class_=AutotermUseWorkTimeSwitch,
        icon="mdi:timer-cog-outline",
    ),
    cv.Optional("wait_mode_switch"): switch.switch_schema(
        class_=AutotermWaitModeSwitch,
        icon="mdi:pause",
    ),


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

    if "display_connected" in config:
        bs = await binary_sensor.new_binary_sensor(config["display_connected"])
        cg.add(var.set_display_connected_sensor(bs))

    if "virtual_panel_temperature" in config:
        temp = await cg.get_variable(config["virtual_panel_temperature"])
        cg.add(var.set_virtual_panel_temp_sensor(temp))

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
    if "set_temperature_control" in config:
        conf = config["set_temperature_control"]
        min_v = conf.get("min_value", 0)
        max_v = conf.get("max_value", 40)
        step_v = conf.get("step", 1)
        num = await number.new_number(conf, min_value=min_v, max_value=max_v, step=step_v)
        cg.add(var.set_set_temperature_number(num))
    if "work_time_control" in config:
        conf = config["work_time_control"]
        min_v = conf.get("min_value", 0)
        max_v = conf.get("max_value", 255)
        step_v = conf.get("step", 1)
        num = await number.new_number(conf, min_value=min_v, max_value=max_v, step=step_v)
        cg.add(var.set_work_time_number(num))
    if "power_level_control" in config:
        conf = config["power_level_control"]
        min_v = conf.get("min_value", 0)
        max_v = conf.get("max_value", 9)
        step_v = conf.get("step", 1)
        num = await number.new_number(conf, min_value=min_v, max_value=max_v, step=step_v)
        cg.add(var.set_power_level_number(num))
    if "virtual_panel_temperature_control" in config:
        conf = config["virtual_panel_temperature_control"]
        min_v = conf.get("min_value", 0)
        max_v = conf.get("max_value", 40)
        step_v = conf.get("step", 1)
        num = await number.new_number(conf, min_value=min_v, max_value=max_v, step=step_v)
        cg.add(var.set_virtual_panel_temp_number(num))
    if "virtual_panel_override_switch" in config:
        sw = await switch.new_switch(config["virtual_panel_override_switch"])
        cg.add(var.set_virtual_panel_override_switch(sw))
    if "temperature_source_control" in config:
        sel = await select.new_select(
            config["temperature_source_control"],
            options=TEMPERATURE_SOURCE_OPTIONS,
        )
        cg.add(var.set_temperature_source_select(sel))
    if "use_work_time_switch" in config:
        sw = await switch.new_switch(config["use_work_time_switch"])
        cg.add(var.set_use_work_time_switch(sw))
    if "wait_mode_switch" in config:
        sw = await switch.new_switch(config["wait_mode_switch"])
        cg.add(var.set_wait_mode_switch(sw))
