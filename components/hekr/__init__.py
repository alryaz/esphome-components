import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart, text_sensor
from esphome.const import CONF_ID, ENTITY_CATEGORY_DIAGNOSTIC, CONF_COMMAND
from esphome.cpp_generator import Pvariable

AUTO_LOAD = ["text_sensor"]
CODEOWNERS = ["@alryaz"]
DEPENDENCIES = ["uart"]

hekr_ns = cg.esphome_ns.namespace("hekr")
Hekr = hekr_ns.class_("Hekr", cg.Component, uart.UARTDevice)
HekrCommandAction = hekr_ns.class_("HekrCommandAction", automation.Action)

CONF_HEKR_ID = "hekr_id"
CONF_LAST_FRAME = "last_frame"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Hekr),
            cv.Optional(CONF_LAST_FRAME): text_sensor.text_sensor_schema(
                icon="mdi:code-json",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

def validate_int_array(value):
    if isinstance(value, (bytes, bytearray)):
        return list(value)
    if isinstance(value, int):
        return [cv.hex_uint8_t(value)]
    if isinstance(value, str):
        return value.encode("utf-8")
    if isinstance(value, list):
        return cv.Schema([cv.hex_uint8_t])(value)
    raise cv.Invalid(
        "data must either be a string wrapped in quotes or a list of bytes"
    )

COMMAND_VALIDATOR = cv.templatable(cv.All(validate_int_array, cv.Length(min=1, msg="At least one value is required")))

async def command_to_code(data, set_template, set_static, args=()):
    if cg.is_template(data):
        templ = await cg.templatable(data, args, cg.std_vector.template(cg.uint8))
        cg.add(set_template(templ))
    else:
        cg.add(set_static(data))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_LAST_FRAME in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_FRAME])
        cg.add(var.set_last_frame_sensor(sens))


@automation.register_action(
    "hekr.command",
    HekrCommandAction,
    cv.maybe_simple_value(
        {
            cv.GenerateID(): cv.use_id(Hekr),
            cv.Required(CONF_COMMAND): COMMAND_VALIDATOR,
        },
        key=CONF_COMMAND,
    ),
)
async def hekr_command_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    await command_to_code(config[CONF_COMMAND], var.set_command_template, var.set_command_static, args)
    return var
