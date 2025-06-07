#define _IR_ENABLE_DEFAULT_ false
#define SEND_HAIER_AC true
#define DECODE_HAIER_AC false

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

        static const char *const TAG = "haier.climate";

        void HaierClimate::setup()
        {
            climate_ir::ClimateIR::setup();
            this->apply_state();
        }

        climate::ClimateTraits HaierClimate::traits()
        {
            auto traits = climate_ir::ClimateIR::traits();
            // Haier supports vertical swing
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_VERTICAL);
            return traits;
        }

        void HaierClimate::transmit_state()
        {
            this->apply_state();
            this->send();
        }

        void HaierClimate::set_health(bool on)
        {
            this->ac_.setHealth(on);
            ESP_LOGI(TAG, "Setting health filter: %s", on ? "ON" : "OFF");
        }

        bool HaierClimate::get_health() const
        {
            return this->ac_.getHealth();
        }

        void HaierClimate::set_sleep(bool on)
        {
            this->ac_.setSleep(on);
            ESP_LOGI(TAG, "Setting sleep mode: %s", on ? "ON" : "OFF");
        }

        bool HaierClimate::get_sleep() const
        {
            return this->ac_.getSleep();
        }

        void HaierClimate::send()
        {
            uint8_t *message = this->ac_.getRaw();
            uint8_t length = kHaierACStateLength;

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

        void HaierClimate::apply_state()
        {
            if (this->mode == climate::CLIMATE_MODE_OFF)
            {
                this->ac_.setCommand(kHaierAcCmdOff);
            }
            else
            {
                this->ac_.setTemp(this->target_temperature);

                switch (this->mode)
                {
                case climate::CLIMATE_MODE_HEAT_COOL:
                case climate::CLIMATE_MODE_AUTO:
                    this->ac_.setMode(kHaierAcAuto);
                    break;
                case climate::CLIMATE_MODE_HEAT:
                    this->ac_.setMode(kHaierAcHeat);
                    break;
                case climate::CLIMATE_MODE_COOL:
                    this->ac_.setMode(kHaierAcCool);
                    break;
                case climate::CLIMATE_MODE_DRY:
                    this->ac_.setMode(kHaierAcDry);
                    break;
                case climate::CLIMATE_MODE_FAN_ONLY:
                    this->ac_.setMode(kHaierAcFan);
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
                        this->ac_.setFan(kHaierAcFanAuto);
                        break;
                    case climate::CLIMATE_FAN_LOW:
                        this->ac_.setFan(kHaierAcFanLow);
                        break;
                    case climate::CLIMATE_FAN_MEDIUM:
                        this->ac_.setFan(kHaierAcFanMed);
                        break;
                    case climate::CLIMATE_FAN_HIGH:
                        this->ac_.setFan(kHaierAcFanHigh);
                        break;
                    case climate::CLIMATE_FAN_QUIET:
                    case climate::CLIMATE_FAN_ON:
                    case climate::CLIMATE_FAN_OFF:
                    case climate::CLIMATE_FAN_MIDDLE:
                    case climate::CLIMATE_FAN_FOCUS:
                    case climate::CLIMATE_FAN_DIFFUSE:
                    default:
                        // Handle unsupported fan modes
                        this->ac_.setFan(kHaierAcFanAuto);
                        break;
                    }
                }

                switch (this->swing_mode)
                {
                case climate::CLIMATE_SWING_OFF:
                    this->ac_.setSwingV(kHaierAcSwingVOff);
                    break;
                case climate::CLIMATE_SWING_VERTICAL:
                    this->ac_.setSwingV(kHaierAcSwingVChg);
                    break;
                case climate::CLIMATE_SWING_HORIZONTAL:
                case climate::CLIMATE_SWING_BOTH:
                default:
                    // Handle unsupported swing modes
                    break;
                }

                this->ac_.setCommand(kHaierAcCmdOn);
            }

            String ac_state = this->ac_.toString();
            ESP_LOGI(TAG, "%s", ac_state.c_str());
        }

    } // namespace haier
} // namespace esphome