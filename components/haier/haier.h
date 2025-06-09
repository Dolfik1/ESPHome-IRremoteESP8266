#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace haier {

class HaierClimate : public climate_ir::ClimateIR {
 public:
  HaierClimate()
      : ClimateIR(16, 30, 1.0f, true, true,
                  {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, 
                   climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                  {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                   climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH}) {}

  void setup() override;
  
 protected:
  void transmit_state() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;

 private:
  void encode_state_();
  bool decode_state_(remote_base::RemoteReceiveData data);
  
  uint8_t remote_state_[13] = {0};
  uint8_t calculate_checksum_();
};

}  // namespace haier
}  // namespace esphome