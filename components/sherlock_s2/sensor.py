import esphome.automation as automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    CONF_CURRENT,
    CONF_ID,
    CONF_POWER,
    CONF_TRIGGER_ID,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_WATT,
    DEVICE_CLASS_BATTERY,
    UNIT_PERCENT,
    CONF_BATTERY_LEVEL,
)
from esphome.cpp_types import Component

CODEOWNERS = ["@alryaz"]
DEPENDENCIES = ["uart"]

sherlock_s2_ns = cg.esphome_ns.namespace("sherlock_s2")
SherlockS2Component = sherlock_s2_ns.class_(
    "SherlockS2Component",
    uart.UARTDevice,
    Component,
)
SherlockS2ActionStartTrigger = sherlock_s2_ns.class_(
    "SherlockS2ActionStartTrigger",
    automation.Trigger.template(),
)
SherlockS2LockStateTrigger = sherlock_s2_ns.class_(
    "SherlockS2LockStateTrigger",
    automation.Trigger.template(cg.float_),
)

CONF_BATTERY_VOLTAGE = "battery_voltage"
CONF_ON_ACTION_START = "on_action_start"
CONF_ON_LOCK = "on_lock"
CONF_ON_UNLOCK = "on_unlock"

_lock_state_trigger_validator = automation.validate_automation(
    {
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SherlockS2LockStateTrigger),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SherlockS2Component),
        cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=3,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_ON_ACTION_START): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                    SherlockS2ActionStartTrigger
                ),
            }
        ),
        cv.Optional(CONF_ON_LOCK): _lock_state_trigger_validator,
        cv.Optional(CONF_ON_UNLOCK): _lock_state_trigger_validator,
    }
).extend(uart.UART_DEVICE_SCHEMA)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "sherlock_s2",
    baud_rate=115200,
    require_tx=False,
    require_rx=True,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_VOLTAGE in config:
        conf = config[CONF_VOLTAGE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_voltage_sensor(sens))
    if CONF_BATTERY_LEVEL in config:
        conf = config[CONF_BATTERY_LEVEL]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_battery_level_sensor(sens))

    for conf in config.get(CONF_ON_ACTION_START, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, (), conf)

    for key, cg_call in (
        (CONF_ON_LOCK, var.add_on_lock_trigger),
        (CONF_ON_UNLOCK, var.add_on_unlock_trigger),
    ):
        for conf in config.get(key, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
            cg.add(cg_call(trigger))
            await automation.build_automation(trigger, [(cg.float_, "duration")], conf)
