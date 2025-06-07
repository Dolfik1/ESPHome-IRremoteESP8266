import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID

AUTO_LOAD = ["climate_ir"]

haier_ns = cg.esphome_ns.namespace("haier")
HaierClimate = haier_ns.class_("HaierClimate", climate_ir.ClimateIR)

CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(HaierClimate)


async def to_code(config):
    cg.add_library("IRremoteESP8266", None)
    var = await climate_ir.new_climate_ir(config)