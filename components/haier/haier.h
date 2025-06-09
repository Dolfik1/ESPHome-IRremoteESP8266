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
        class HaierYRW02Climate : public climate_ir::ClimateIR
        {
        public:
            HaierYRW02Climate()
                : ClimateIR(16, 30, 1.0f, true, true,
                            {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                            {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL, climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH}) {}

            void setup() override;
            climate::ClimateTraits traits() override;

            // Additional Haier YRW02-specific methods
            void set_health(bool on);
            bool get_health() const;
            void set_sleep(bool on);
            bool get_sleep() const;
            void set_turbo(bool on);
            bool get_turbo() const;
            void set_quiet(bool on);
            bool get_quiet() const;

        protected:
            void transmit_state() override;

        private:
            void send();
            void apply_state();

            IRHaierACYRW02 ac_ = IRHaierACYRW02(255); // pin is not used
        };

    } // namespace haier
} // namespace esphome