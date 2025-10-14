import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import const
import esphome.components.uart as uart
import esphome.components.sensor as sensor
import esphome.components.text_sensor as text_sensor
import esphome.components.number as number
import esphome.components.climate as climate
import esphome.components.select as select

DEPENDENCIES = ["sensor", "text_sensor", "number", "climate"]
AUTO_LOAD = ["sensor", "text_sensor", "number", "climate", "select"]

autoterm_ns = cg.esphome_ns.namespace("autoterm_uart")
AutotermFanLevelNumber = autoterm_ns.class_("AutotermFanLevelNumber", number.Number)
AutotermUART = autoterm_ns.class_("AutotermUART", cg.Component)
AutotermClimate = autoterm_ns.class_("AutotermClimate", climate.Climate)
AutotermTempSourceSelect = autoterm_ns.class_("AutotermTempSourceSelect", select.Select)

CONF_CLIMATE = "climate"
CONF_DEFAULT_LEVEL = "default_level"
CONF_DEFAULT_TEMPERATURE = "default_temperature"
CONF_DEFAULT_TEMP_SENSOR = "default_temp_sensor"
CONF_THERMOSTAT_HYS_ON = "thermostat_hysteresis_on"
CONF_THERMOSTAT_HYS_OFF = "thermostat_hysteresis_off"
CONF_PANEL_TEMP_OVERRIDE = "panel_temp_override"
CONF_PANEL_TEMP_OVERRIDE_SENSOR = "sensor"
CONF_TEMP_SOURCE_SELECT = "temperature_source_select"

TEMP_SOURCE_OPTIONS = ["Intern", "Panel", "Extern", "Home Assistant"]

CLIMATE_SCHEMA = climate.climate_schema(AutotermClimate).extend({
    cv.Optional(CONF_DEFAULT_LEVEL, default=4): cv.int_range(min=0, max=9),
    cv.Optional(CONF_DEFAULT_TEMPERATURE, default=20.0): cv.temperature,
    cv.Optional(CONF_DEFAULT_TEMP_SENSOR, default=2): cv.int_range(min=1, max=4),
    cv.Optional(CONF_THERMOSTAT_HYS_ON, default=2.0): cv.float_range(min=1.0, max=5.0),
    cv.Optional(CONF_THERMOSTAT_HYS_OFF, default=1.0): cv.float_range(min=0.0, max=2.0),
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(AutotermUART),
    cv.Required("uart_display_id"): cv.use_id(uart.UARTComponent),
    cv.Required("uart_heater_id"): cv.use_id(uart.UARTComponent),

    cv.Optional("internal_temp"): sensor.sensor_schema(unit_of_measurement="°C", icon="mdi:thermometer"),
    cv.Optional("external_temp"): sensor.sensor_schema(unit_of_measurement="°C", icon="mdi:thermometer"),
    cv.Optional("heater_temp"): sensor.sensor_schema(unit_of_measurement="°C", icon="mdi:thermometer"),
    cv.Optional("panel_temp"): sensor.sensor_schema(unit_of_measurement="°C", icon="mdi:thermometer"),
    cv.Optional("voltage"): sensor.sensor_schema(unit_of_measurement="V", icon="mdi:flash"),
    cv.Optional("status"): sensor.sensor_schema(icon="mdi:information"),
    cv.Optional("fan_speed_set"): sensor.sensor_schema(unit_of_measurement="rpm", icon="mdi:fan"),
    cv.Optional("fan_speed_actual"): sensor.sensor_schema(unit_of_measurement="rpm", icon="mdi:fan"),
    cv.Optional("pump_frequency"): sensor.sensor_schema(unit_of_measurement="Hz", icon="mdi:water-pump"),
    cv.Optional("runtime_hours"): sensor.sensor_schema(
        unit_of_measurement="h",
        icon="mdi:clock-outline",
        accuracy_decimals=2,
        device_class=const.DEVICE_CLASS_DURATION,
        state_class=const.STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional("session_runtime"): sensor.sensor_schema(
        unit_of_measurement="h",
        icon="mdi:timer-outline",
        accuracy_decimals=2,
        device_class=const.DEVICE_CLASS_DURATION,
        state_class=const.STATE_CLASS_MEASUREMENT,
    ),

    cv.Optional("status_text"): text_sensor.text_sensor_schema(icon="mdi:information"),

    cv.Optional("fan_level"): number.number_schema(class_=AutotermFanLevelNumber, icon="mdi:fan-speed-1"),

    cv.Optional(CONF_CLIMATE): CLIMATE_SCHEMA,
    cv.Optional(CONF_PANEL_TEMP_OVERRIDE): cv.Schema({
        cv.Required(CONF_PANEL_TEMP_OVERRIDE_SENSOR): cv.use_id(sensor.Sensor),
    }),
    cv.Optional(CONF_TEMP_SOURCE_SELECT): select.select_schema(class_=AutotermTempSourceSelect, icon="mdi:thermometer-probe"),

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
        ("panel_temp", "set_panel_temp_sensor"),
        ("voltage", "set_voltage_sensor"),
        ("status", "set_status_sensor"),
        ("fan_speed_set", "set_fan_speed_set_sensor"),
        ("fan_speed_actual", "set_fan_speed_actual_sensor"),
        ("pump_frequency", "set_pump_frequency_sensor"),
        ("runtime_hours", "set_runtime_hours_sensor"),
        ("session_runtime", "set_session_runtime_sensor"),
    ]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(sens))

    for key, setter in [
        ("status_text", "set_status_text_sensor"),
    ]:
        if key in config:
            txt = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(var, setter)(txt))

    if "fan_level" in config:
        conf = config["fan_level"]
        # Standardwerte definieren, falls nicht im YAML angegeben
        min_v = conf.get("min_value", 0)
        max_v = conf.get("max_value", 9)
        step_v = conf.get("step", 1)
        num = await number.new_number(conf, min_value=min_v, max_value=max_v, step=step_v)
        cg.add(var.set_fan_level_number(num))

    if CONF_CLIMATE in config:
        climate_conf = config[CONF_CLIMATE]
        clim = cg.new_Pvariable(climate_conf[const.CONF_ID])
       # await cg.register_component(clim, climate_conf)
        await climate.register_climate(clim, climate_conf)
        cg.add(clim.set_default_level(climate_conf[CONF_DEFAULT_LEVEL]))
        cg.add(clim.set_default_temperature(climate_conf[CONF_DEFAULT_TEMPERATURE]))
        cg.add(clim.set_default_temp_sensor(climate_conf[CONF_DEFAULT_TEMP_SENSOR]))
        cg.add(clim.set_thermostat_hysteresis(
            climate_conf[CONF_THERMOSTAT_HYS_ON],
            climate_conf[CONF_THERMOSTAT_HYS_OFF],
        ))
        cg.add(var.set_climate(clim))

    if CONF_PANEL_TEMP_OVERRIDE in config:
        override_conf = config[CONF_PANEL_TEMP_OVERRIDE]
        src = await cg.get_variable(override_conf[CONF_PANEL_TEMP_OVERRIDE_SENSOR])
        cg.add(var.set_panel_temp_override_sensor(src))

    if CONF_TEMP_SOURCE_SELECT in config:
        select_conf = config[CONF_TEMP_SOURCE_SELECT]
        sel = await select.new_select(select_conf, options=TEMP_SOURCE_OPTIONS)
        cg.add(var.set_temp_source_select(sel))
