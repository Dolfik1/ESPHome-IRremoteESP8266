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

        void HaierClimate::setup()
        {
            this->ac_.begin();
            this->apply_state();
            
            if (this->receiver_ != nullptr) {
                this->receiver_->register_listener(this);
            }
        }

        void HaierClimate::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Haier AC Climate:");
            ESP_LOGCONFIG(TAG, "  Model: %s", this->ac_.getModel() == haier_ac176_remote_model_t::V9014557_A ? "V9014557_A" : "V9014557_B");
        }

        void HaierClimate::set_model(const Model model)
        {
            this->ac_.setModel((haier_ac176_remote_model_t) model);
        }

        void HaierClimate::set_receiver(remote_base::RemoteReceiverBase *receiver)
        {
            this->receiver_ = receiver;
        }

        climate::ClimateTraits HaierClimate::traits()
        {
            auto traits = climate::ClimateTraits();
            
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

        void HaierClimate::control(const climate::ClimateCall &call)
        {
            if (call.get_mode().has_value()) {
                this->mode = *call.get_mode();
            }
            if (call.get_target_temperature().has_value()) {
                this->target_temperature = *call.get_target_temperature();
            }
            if (call.get_fan_mode().has_value()) {
                this->fan_mode = *call.get_fan_mode();
            }
            if (call.get_swing_mode().has_value()) {
                this->swing_mode = *call.get_swing_mode();
            }

            this->transmit_state();
            this->publish_state();
        }

        void HaierClimate::transmit_state()
        {
            this->apply_state();
            this->send_ir();
        }

        bool HaierClimate::on_receive(remote_base::RemoteReceiveData data)
        {
            ESP_LOGD(TAG, "Received IR data");
            // Для простоты пока возвращаем false
            // В будущем здесь можно добавить декодирование протокола Haier
            return false;
        }

        void HaierClimate::send_ir()
        {
            if (this->transmitter_ == nullptr) {
                ESP_LOGW(TAG, "No transmitter configured");
                return;
            }

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