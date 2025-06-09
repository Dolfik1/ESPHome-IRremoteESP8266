#define _IR_ENABLE_DEFAULT_ false
#define SEND_HAIER_AC176 true
#define DECODE_HAIER_AC176 true

#include "esphome.h"
#include "ir_Haier.h"
#include "haier.h"

namespace esphome
{
    namespace haier
    {
        // Constants copied from ir_Haier.cpp
        const uint16_t kHaierAcHdr = 3000;
        const uint16_t kHaierAcHdrGap = 4300;
        const uint16_t kHaierAcBitMark = 520;
        const uint16_t kHaierAcOneSpace = 1650;
        const uint16_t kHaierAcZeroSpace = 650;
        const uint32_t kHaierAcMinGap = 150000;

        static const char *const TAG = "haier.climate";

        void HaierClimate::set_model(const Model model)
        {
            this->ac_.setModel((haier_ac176_remote_model_t) model);
        }

        void HaierClimate::setup()
        {
            climate_ir::ClimateIR::setup();
            this->apply_state();
        }

        climate::ClimateTraits HaierClimate::traits()
        {
            auto traits = climate_ir::ClimateIR::traits();
            
            // Add supported features
            traits.set_supports_current_temperature(false);
            traits.set_visual_min_temperature(16);
            traits.set_visual_max_temperature(30);
            traits.set_visual_temperature_step(1);
            
            // Add supported modes
            traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
            traits.add_supported_mode(climate::CLIMATE_MODE_AUTO);
            traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
            traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
            traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
            traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);
            
            // Add supported fan modes
            traits.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);
            traits.add_supported_fan_mode(climate::CLIMATE_FAN_LOW);
            traits.add_supported_fan_mode(climate::CLIMATE_FAN_MEDIUM);
            traits.add_supported_fan_mode(climate::CLIMATE_FAN_HIGH);
            
            // Add supported swing modes
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_OFF);
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_VERTICAL);
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_HORIZONTAL);
            traits.add_supported_swing_mode(climate::CLIMATE_SWING_BOTH);
            
            return traits;
        }

        void HaierClimate::transmit_state()
        {
            this->apply_state();
            this->send();
        }

        bool HaierClimate::on_receive(remote_base::RemoteReceiveData data)
        {
            // Try to decode the received IR data
            if (!data.is_valid()) return false;
            
            auto decode_data = data.get_data();
            
            // Basic validation - check if this looks like a Haier AC message
            if (decode_data.size() < kHaierAC176StateLength) return false;
            
            // Try to validate as Haier AC176 protocol
            uint8_t state[kHaierAC176StateLength];
            for (int i = 0; i < kHaierAC176StateLength && i < decode_data.size(); i++) {
                state[i] = decode_data[i];
            }
            
            if (!IRHaierAC176::validChecksum(state, kHaierAC176StateLength)) {
                ESP_LOGV(TAG, "Invalid checksum");
                return false;
            }
            
            // Set the raw state and parse it
            this->ac_.setRaw(state);
            
            // Update climate component state from the received data
            if (this->ac_.getPower()) {
                auto mode = this->ac_.getMode();
                switch (mode) {
                    case kHaierAcYrw02Auto:
                        this->mode = climate::CLIMATE_MODE_AUTO;
                        break;
                    case kHaierAcYrw02Cool:
                        this->mode = climate::CLIMATE_MODE_COOL;
                        break;
                    case kHaierAcYrw02Heat:
                        this->mode = climate::CLIMATE_MODE_HEAT;
                        break;
                    case kHaierAcYrw02Dry:
                        this->mode = climate::CLIMATE_MODE_DRY;
                        break;
                    case kHaierAcYrw02Fan:
                        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
                        break;
                    default:
                        this->mode = climate::CLIMATE_MODE_AUTO;
                }
            } else {
                this->mode = climate::CLIMATE_MODE_OFF;
            }
            
            this->target_temperature = this->ac_.getTemp();
            
            // Update fan mode
            auto fan = this->ac_.getFan();
            switch (fan) {
                case kHaierAcYrw02FanAuto:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
                    break;
                case kHaierAcYrw02FanLow:
                    this->fan_mode = climate::CLIMATE_FAN_LOW;
                    break;
                case kHaierAcYrw02FanMed:
                    this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
                    break;
                case kHaierAcYrw02FanHigh:
                    this->fan_mode = climate::CLIMATE_FAN_HIGH;
                    break;
                default:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
            }
            
            // Update swing mode
            auto swingV = this->ac_.getSwingV();
            auto swingH = this->ac_.getSwingH();
            
            if (swingV == kHaierAcYrw02SwingVOff && swingH == kHaierAcYrw02SwingHMiddle) {
                this->swing_mode = climate::CLIMATE_SWING_OFF;
            } else if (swingV != kHaierAcYrw02SwingVOff && swingH == kHaierAcYrw02SwingHMiddle) {
                this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
            } else if (swingV == kHaierAcYrw02SwingVOff && swingH == kHaierAcYrw02SwingHAuto) {
                this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
            } else if (swingV != kHaierAcYrw02SwingVOff && swingH == kHaierAcYrw02SwingHAuto) {
                this->swing_mode = climate::CLIMATE_SWING_BOTH;
            } else {
                this->swing_mode = climate::CLIMATE_SWING_OFF;
            }
            
            this->publish_state();
            
            ESP_LOGI(TAG, "Received state: %s", this->ac_.toString().c_str());
            
            return true;
        }

        void HaierClimate::send()
        {
            uint8_t *message = this->ac_.getRaw();
            uint8_t length = kHaierAC176StateLength;

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
                this->ac_.setPower(false);
            }
            else
            {
                this->ac_.setPower(true);
                this->ac_.setTemp(this->target_temperature);

                switch (this->mode)
                {
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
                default:
                    this->ac_.setMode(kHaierAcYrw02Auto);
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
                    this->ac_.setSwingV(kHaierAcYrw02SwingVOff);
                    this->ac_.setSwingH(kHaierAcYrw02SwingHMiddle);
                }
            }

            ESP_LOGI(TAG, "Sending state: %s", this->ac_.toString().c_str());
        }

    } // namespace haier
} // namespace esphome