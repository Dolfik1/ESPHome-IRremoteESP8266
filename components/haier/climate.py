import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, remote_transmitter, remote_receiver
from esphome.const import CONF_ID, CONF_MODEL

AUTO_LOAD = ["climate"]
DEPENDENCIES = ["remote_transmitter"]

haier_ns = cg.esphome_ns.namespace("haier")
HaierClimate = haier_ns.class_("HaierClimate", climate.Climate, cg.Component)

Model = haier_ns.enum("Model")
MODELS = {
    "V9014557_A": Model.V9014557_A,
    "V9014557_B": Model.V9014557_B,
}

CONF_TRANSMITTER_ID = "transmitter_id"
CONF_RECEIVER_ID = "receiver_id"

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(HaierClimate),
        cv.GenerateID(CONF_TRANSMITTER_ID): cv.use_id(remote_transmitter.RemoteTransmitterComponent),
        cv.Optional(CONF_RECEIVER_ID): cv.use_id(remote_receiver.RemoteReceiverComponent),
        cv.Optional(CONF_MODEL, default="V9014557_A"): cv.enum(MODELS),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    cg.add_library("IRremoteESP8266", None)
    
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    
    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])
    cg.add(var.set_transmitter(transmitter))
    
    if CONF_RECEIVER_ID in config:
        receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
        cg.add(var.set_receiver(receiver))
    
    cg.add(var.set_model(config[CONF_MODEL]))