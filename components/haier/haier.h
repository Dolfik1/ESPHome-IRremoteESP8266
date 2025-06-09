#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/remote_base/remote_base.h"
#include "ir_Haier.h"

namespace esphome
{
    namespace haier
    {
        enum Model {
            V9014557_A = haier_ac176_remote_model_t::V9014557_A,
            V9014557_B = haier_ac176_remote_model_t::V9014557_B
        };

        class HaierClimate : public climate::Climate, public Component, public remote_base::RemoteReceiverListener
        {
        public:
            void setup() override;
            void dump_config() override;
            void set_model(const Model model);
            void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) { this->transmitter_ = transmitter; }
            void set_receiver(remote_base::RemoteReceiverBase *receiver);
            
            climate::ClimateTraits traits() override;
            
            // RemoteReceiverListener interface
            bool on_receive(remote_base::RemoteReceiveData data) override;

        protected:
            void control(const climate::ClimateCall &call) override;

        private:
            void transmit_state();
            void apply_state();
            void send_ir();

            IRHaierAC176 ac_ = IRHaierAC176(255); // pin is not used
            remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};
            remote_base::RemoteReceiverBase *receiver_{nullptr};
        };

    } // namespace haier
} // namespace esphome