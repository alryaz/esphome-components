from esphome.components import sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_COMMAND, CONF_LENGTH, CONF_POSITION, CONF_REVERSED, CONF_SOURCE
from .. import hekr_ns, CONF_HEKR_ID, Hekr

DEPENDENCIES = ["hekr"]
CODEOWNERS = ["@alryaz"]

HekrSensor = hekr_ns.class_("HekrSensor", sensor.Sensor, cg.Component)

CONFIG_SCHEMA = sensor.SENSOR_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(HekrSensor),
        cv.GenerateID(CONF_HEKR_ID): cv.use_id(Hekr),
        cv.Required(CONF_COMMAND): cv.uint8_t,
        cv.Optional(CONF_POSITION, default=0): cv.uint8_t,
        cv.Optional(CONF_LENGTH, default=1): cv.All(cv.uint8_t, cv.Range(min=1, min_included=True)),
        cv.Optional(CONF_REVERSED, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)

    paren = await cg.get_variable(config[CONF_HEKR_ID])
    cg.add(var.set_parent(paren))
    cg.add(var.set_command(config[CONF_COMMAND]))
    cg.add(var.set_position(config[CONF_POSITION]))
    cg.add(var.set_length(config[CONF_LENGTH]))
    cg.add(var.set_reversed(config[CONF_REVERSED]))
