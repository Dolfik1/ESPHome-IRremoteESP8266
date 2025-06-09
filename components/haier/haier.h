#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/climate_ir/climate_ir.h"
#include "ir_Haier.h"

namespace esphome
{
    namespace haier
    {
        enum Model {
            V9014557_A = haier_ac176_remote_model_t::V9014557_A,
            V9014557_B = haier_ac176_remote_model_t::V9014557_B
        };

        class HaierClimate : public climate_ir::ClimateIR
        {
        public:
            HaierClimate()
                : ClimateIR(16, 30, 1.0f, true, true,
                            {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                            {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL, climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH}) {}

            void set_model(const Model model);
            
            void setup() override;
            climate::ClimateTraits traits() override;

        protected:
            void transmit_state() override;
            bool on_receive(remote_base::RemoteReceiveData data) override;

        private:
            void send();
            void apply_state();

            IRHaierAC176 ac_ = IRHaierAC176(255); // pin is not used
        };

    } // namespace haier
} // namespace esphome