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
        class HaierClimate : public climate_ir::ClimateIR
        {
        public:
            HaierClimate()
                : ClimateIR(16, 30, 1.0f, true, true,
                            {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                            {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

            void setup() override;
            climate::ClimateTraits traits() override;

            // Additional Haier-specific methods
            void set_health(bool on);
            bool get_health() const;
            void set_sleep(bool on);
            bool get_sleep() const;

        protected:
            void transmit_state() override;

        private:
            void send();
            void apply_state();

            IRHaierAC ac_ = IRHaierAC(255); // pin is not used
        };

    } // namespace haier
} // namespace esphome