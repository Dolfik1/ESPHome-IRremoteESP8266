#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/remote_receiver/remote_receiver.h"
#include "ir_Haier.h"

namespace esphome
{
    namespace haier
    {
        enum Model {
            V9014557_A = haier_ac176_remote_model_t::V9014557_A,
            V9014557_B = haier_ac176_remote_model_t::V9014557_B
        };

        class HaierClimate : public climate::Climate, public Component
        {
        public:
            void setup() override;
            void dump_config() override;
            void set_model(const Model model);
            void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) { this->transmitter_ = transmitter; }
            void set_receiver(remote_receiver::RemoteReceiverComponent *receiver);
            
            climate::ClimateTraits traits() override;

        protected:
            void control(const climate::ClimateCall &call) override;

        private:
            void transmit_state();
            void apply_state();
            void send_ir();
            void on_receive(remote_receiver::RemoteReceiveData data);

            IRHaierAC176 ac_ = IRHaierAC176(255); // pin is not used
            remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};
            remote_receiver::RemoteReceiverComponent *receiver_{nullptr};
        };

    } // namespace haier
} // namespace esphome