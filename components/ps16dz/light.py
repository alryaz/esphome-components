"""PS-16-DZ Custom Component for esphome"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_GAMMA_CORRECT, CONF_OUTPUT_ID, CONF_TRIGGER_ID, CONF_MIN_VALUE, CONF_MAX_VALUE
from esphome.components import uart, light
from esphome import automation

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@alryaz"]

# Namespace and base class
ps16dz_ns = cg.esphome_ns.namespace("ps16dz")
PS16DZLight = ps16dz_ns.class_(
    "PS16DZLight",
    light.LightOutput,
    uart.UARTDevice,
    cg.Component
)
PS16DZSettingsEnterTrigger = ps16dz_ns.class_(
    "PS16DZSettingsEnterTrigger", automation.Trigger.template(),
)
PS16DZSettingsExitTrigger = ps16dz_ns.class_(
    "PS16DZSettingsExitTrigger", automation.Trigger.template(),
)

CONF_ON_SETTINGS_ENTER = "on_settings_enter"
CONF_ON_SETTINGS_EXIT = "on_settings_exit"

CONFIG_SCHEMA = cv.All(
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(PS16DZLight),
            cv.Optional(CONF_GAMMA_CORRECT, default=0): cv.positive_float,
            cv.Optional(CONF_MIN_VALUE, default=0): cv.int_range(min=0, max=100),
            cv.Optional(CONF_MAX_VALUE, default=100): cv.int_range(min=0, max=100),
            cv.Optional(CONF_ON_SETTINGS_ENTER): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        PS16DZSettingsEnterTrigger
                    ),
                }
            ),
            cv.Optional(CONF_ON_SETTINGS_EXIT): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        PS16DZSettingsExitTrigger
                    ),
                }
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "ps16dz",
    baud_rate=19200,
    require_tx=True,
    require_rx=True
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await light.register_light(var, config)

    for key in (CONF_ON_SETTINGS_ENTER, CONF_ON_SETTINGS_EXIT):
        for conf in config.get(key, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, (), conf)
