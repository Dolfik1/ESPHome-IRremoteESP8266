#define _IR_ENABLE_DEFAULT_ false
#define SEND_HAIER_AC176 true
#define DECODE_HAIER_AC176 false

#include "esphome.h"
#include "ir_Haier.h"
#include "haier.h"

namespace esphome
{
    namespace haier
    {
        // copied from ir_Haier.cpp
        const uint16_t kHaierAcHdr = 3000;
        const uint16_t kHaierAcHdrGap = 4300;
        const uint16_t kHaierAcBitMark = 520;
        const uint16_t kHaierAcOneSpace = 1650;
        const uint16_t kHaierAcZeroSpace = 650;
        const uint32_t kHaierAcMinGap = 150000;

        static const char *const TAG = "haier_yrw02.climate";

        void HaierYRW02Climate::setup()
        {
            climate_ir::ClimateIR::setup();
            // Устанавливаем модель A для YRW02
            this->ac_.setModel(haier_ac176_remote_model_t::V9014557_A);
            this->apply_state();
        }

        climate::ClimateTraits HaierYRW02Climate::traits()
        {
            auto traits = climate_ir::ClimateIR::traits();
            // Haier YRW02 supports both vertical and horizontal swing
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_VERTICAL);
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_HORIZONTAL);
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_BOTH);
            return traits;
        }

        void HaierYRW02Climate::transmit_state()
        {
            this->apply_state();
            this->send_ir();
        }

        void HaierYRW02Climate::set_health(bool on)
        {
            this->ac_.setHealth(on);
            ESP_LOGI(TAG, "Setting health filter: %s", on ? "ON" : "OFF");
        }

        bool HaierYRW02Climate::get_health() const
        {
            return this->ac_.getHealth();
        }

        void HaierYRW02Climate::set_sleep(bool on)
        {
            this->ac_.setSleep(on);
            ESP_LOGI(TAG, "Setting sleep mode: %s", on ? "ON" : "OFF");
        }

        bool HaierYRW02Climate::get_sleep() const
        {
            return this->ac_.getSleep();
        }

        void HaierYRW02Climate::set_turbo(bool on)
        {
            this->ac_.setTurbo(on);
            ESP_LOGI(TAG, "Setting turbo mode: %s", on ? "ON" : "OFF");
        }

        bool HaierYRW02Climate::get_turbo() const
        {
            return this->ac_.getTurbo();
        }

        void HaierYRW02Climate::set_quiet(bool on)
        {
            this->ac_.setQuiet(on);
            ESP_LOGI(TAG, "Setting quiet mode: %s", on ? "ON" : "OFF");
        }

        bool HaierYRW02Climate::get_quiet() const
        {
            return this->ac_.getQuiet();
        }

        void HaierYRW02Climate::send_ir()
        {
            uint8_t *message = this->ac_.getRaw();
            uint8_t length = kHaierACYRW02StateLength; // 13 байт для YRW02

            auto transmit = this->transmitter_->transmit();
            auto *data = transmit.get_data();

            data->set_carrier_frequency(38000);

            // Pre-Header
            data->mark(kHaierAcHdr);
            data->space(kHaierAcHdr);

            // Header
            data->mark(kHaierAcHdr);
            data->space(kHaierAcHdrGap);

            // Data
            for (uint8_t i = 0; i < length; i++)
            {
                uint8_t d = *(message + i);
                for (uint8_t bit = 0; bit < 8; bit++, d >>= 1)
                {
                    if (d & 1)
                    {
                        data->mark(kHaierAcBitMark);
                        data->space(kHaierAcOneSpace);
                    }
                    else
                    {
                        data->mark(kHaierAcBitMark);
                        data->space(kHaierAcZeroSpace);
                    }
                }
            }

            // Footer
            data->mark(kHaierAcBitMark);
            data->space(kHaierAcMinGap);

            transmit.perform();
        }

        void HaierYRW02Climate::apply_state()
        {
            if (this->mode == climate::CLIMATE_MODE_OFF)
            {
                this->ac_.setPower(false);
            }
            else
            {
                this->ac_.setPower(true);
                this->ac_.setTemp(this->target_temperature);

                switch (this->mode)
                {
                case climate::CLIMATE_MODE_HEAT_COOL:
                case climate::CLIMATE_MODE_AUTO:
                    this->ac_.setMode(kHaierAcYrw02Auto);
                    break;
                case climate::CLIMATE_MODE_HEAT:
                    this->ac_.setMode(kHaierAcYrw02Heat);
                    break;
                case climate::CLIMATE_MODE_COOL:
                    this->ac_.setMode(kHaierAcYrw02Cool);
                    break;
                case climate::CLIMATE_MODE_DRY:
                    this->ac_.setMode(kHaierAcYrw02Dry);
                    break;
                case climate::CLIMATE_MODE_FAN_ONLY:
                    this->ac_.setMode(kHaierAcYrw02Fan);
                    break;
                case climate::CLIMATE_MODE_OFF:
                default:
                    // Handle OFF mode and any other modes
                    break;
                }

                if (this->fan_mode.has_value())
                {
                    switch (this->fan_mode.value())
                    {
                    case climate::CLIMATE_FAN_AUTO:
                        this->ac_.setFan(kHaierAcYrw02FanAuto);
                        break;
                    case climate::CLIMATE_FAN_LOW:
                        this->ac_.setFan(kHaierAcYrw02FanLow);
                        break;
                    case climate::CLIMATE_FAN_MEDIUM:
                        this->ac_.setFan(kHaierAcYrw02FanMed);
                        break;
                    case climate::CLIMATE_FAN_HIGH:
                        this->ac_.setFan(kHaierAcYrw02FanHigh);
                        break;
                    default:
                        this->ac_.setFan(kHaierAcYrw02FanAuto);
                        break;
                    }
                }

                switch (this->swing_mode)
                {
                case climate::CLIMATE_SWING_OFF:
                    this->ac_.setSwingV(kHaierAcYrw02SwingVOff);
                    this->ac_.setSwingH(kHaierAcYrw02SwingHMiddle);
                    break;
                case climate::CLIMATE_SWING_VERTICAL:
                    this->ac_.setSwingV(kHaierAcYrw02SwingVAuto);
                    this->ac_.setSwingH(kHaierAcYrw02SwingHMiddle);
                    break;
                case climate::CLIMATE_SWING_HORIZONTAL:
                    this->ac_.setSwingV(kHaierAcYrw02SwingVOff);
                    this->ac_.setSwingH(kHaierAcYrw02SwingHAuto);
                    break;
                case climate::CLIMATE_SWING_BOTH:
                    this->ac_.setSwingV(kHaierAcYrw02SwingVAuto);
                    this->ac_.setSwingH(kHaierAcYrw02SwingHAuto);
                    break;
                default:
                    break;
                }
            }

            String ac_state = this->ac_.toString();
            ESP_LOGI(TAG, "%s", ac_state.c_str());
        }

    } // namespace haier
} // namespace esphome