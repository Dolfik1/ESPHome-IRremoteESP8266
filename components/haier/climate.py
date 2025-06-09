import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_MODEL

AUTO_LOAD = ["climate_ir"]

haier_ns = cg.esphome_ns.namespace("haier")
HaierClimate = haier_ns.class_("HaierClimate", climate_ir.ClimateIR)

Model = haier_ns.enum("Model")
MODELS = {
    "V9014557_A": Model.V9014557_A,
    "V9014557_B": Model.V9014557_B,
}

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(HaierClimate),
        cv.Optional(CONF_MODEL, default="V9014557_A"): cv.enum(MODELS),
    }
)


async def to_code(config):
    cg.add_library("IRremoteESP8266", None)

    var = await climate_ir.new_climate_ir(config)
    cg.add(var.set_model(config[CONF_MODEL]))