from esphome.components import switch
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_COMMAND, CONF_POSITION, CONF_ASSUMED_STATE, CONF_OPTIMISTIC
from .. import hekr_ns, CONF_HEKR_ID, Hekr, COMMAND_VALIDATOR, command_to_code

DEPENDENCIES = ["hekr"]
CODEOWNERS = ["@alryaz"]

HekrSwitch = hekr_ns.class_("HekrSwitch", switch.Switch, cg.Component)

CONF_COMMAND_ON = "command_on"
CONF_COMMAND_OFF = "command_off"
CONF_VALUE_ON = "value_on"
CONF_VALUE_OFF = "value_off"

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(HekrSwitch),
        cv.GenerateID(CONF_HEKR_ID): cv.use_id(Hekr),
        cv.Optional(CONF_COMMAND): cv.uint8_t,
        cv.Optional(CONF_POSITION): cv.uint8_t,
        cv.Required(CONF_COMMAND_OFF): COMMAND_VALIDATOR,
        cv.Required(CONF_COMMAND_ON): COMMAND_VALIDATOR,
        cv.Optional(CONF_VALUE_OFF, default=0): cv.uint8_t,
        cv.Optional(CONF_VALUE_ON, default=1): cv.uint8_t,
        cv.Optional(CONF_OPTIMISTIC, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # @TODO: move to schema
    if CONF_COMMAND not in config:
        config[CONF_ASSUMED_STATE] = True

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    paren = await cg.get_variable(config[CONF_HEKR_ID])
    cg.add(var.set_parent(paren))

    if (CONF_COMMAND in config):
        cg.add(var.set_command(config[CONF_COMMAND]))
        cg.add(var.set_position(config[CONF_POSITION]))
        cg.add(var.set_value_off(config[CONF_VALUE_OFF]))
        cg.add(var.set_value_on(config[CONF_VALUE_ON]))

    await command_to_code(config[CONF_COMMAND_OFF], var.set_command_off_template, var.set_command_off_static)
    await command_to_code(config[CONF_COMMAND_ON], var.set_command_on_template, var.set_command_on_static)
    
    cg.add(var.set_optimistic(config[CONF_OPTIMISTIC]))
