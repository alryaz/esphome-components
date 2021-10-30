"""Sonoff D1 Custom Component for esphome"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_GAMMA_CORRECT, CONF_OUTPUT_ID
from esphome.components import uart, light

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@alryaz"]

# Namespace and base class
sonoff_d1_ns = cg.esphome_ns.namespace("sonoff_d1")
SonoffD1Light = sonoff_d1_ns.class_(
    "SonoffD1Light",
    light.LightOutput,
    uart.UARTDevice,
    cg.Component
)

CONFIG_SCHEMA = cv.All(
    light.BRIGHTNESS_ONLY_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(SonoffD1Light),
            # cv.Optional(CONF_GAMMA_CORRECT, default=0): cv.positive_float,
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "sonoff_d1",
    baud_rate=9600,
    require_tx=True,
    require_rx=True
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await light.register_light(var, config)
