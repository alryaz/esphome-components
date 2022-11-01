/*
  ps16dz_light.h - PS-16-DZ Dimmer support for ESPHome

  Copyright © 2021 Alexander Ryazanov

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
  and associated documentation files (the “Software”), to deal in the Software without
  restriction, including without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.
  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  -----

  If modifying this file, in addition to the license above, please ensure to include links back to the original code:
  https://github.com/arendst/Tasmota/blob/development/tasmota/xdrv_19_ps16dz_dimmer.ino
  https://github.com/alryaz/esphome-components/blob/main/components/sonoff_d1/README.md

  -----

  THANK YOU!
  Thanks to the team over at Tasmota for providing the serial codes to control the dimmer!
  View the source: https://github.com/arendst/Tasmota/blob/development/tasmota/xdrv_19_ps16dz_dimmer.ino
*/

#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_traits.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include <cstring>

namespace esphome
{
    namespace ps16dz
    {
        class PS16DZSettingsEnterTrigger;
        class PS16DZSettingsExitTrigger;

        const uint8_t PS16DZ_READ_BUFFER_LENGTH = 80;

        class PS16DZLight : public uart::UARTDevice, public light::LightOutput, public Component
        {
        private:
            bool in_settings_mode_{false};
            light::LightState *state_{nullptr};
            bool last_binary_{false};
            uint8_t last_brightness_{0};
            uint64_t sequence_number{0};

        protected:
            char read_buffer_[PS16DZ_READ_BUFFER_LENGTH];
            size_t read_pos_{0};
            CallbackManager<void()> settings_enter_callback_{};
            CallbackManager<void()> settings_exit_callback_{};
            uint8_t min_value_{0};
            uint8_t max_value_{100};

            void serial_send_(const char *tx_buffer);
            void serial_send_ok_(void);
            void execute_command_(const bool &switch_state, const int &dimmer_value);

        public:
            // LightOutput methods
            light::LightTraits get_traits() override;
            void write_state(light::LightState *state) override;
            void setup_state(light::LightState *state) override { this->state_ = state; };
            
            // Component methods
            void setup() override{};
            void loop() override;
            void dump_config() override;
            float get_setup_priority() const override { return esphome::setup_priority::DATA; }

            // Callback handlers
            void add_on_settings_enter_callback(std::function<void()> &&callback);
            void add_on_settings_exit_callback(std::function<void()> &&callback);
        };

        class PS16DZSettingsEnterTrigger : public Trigger<>
        {
            public:
                explicit PS16DZSettingsEnterTrigger(PS16DZLight *parent) {
                    parent->add_on_settings_enter_callback([this]() { this->trigger(); });
                }
        };

        class PS16DZSettingsExitTrigger : public Trigger<>
        {
            public:
                explicit PS16DZSettingsExitTrigger(PS16DZLight *parent) {
                    parent->add_on_settings_exit_callback([this]() { this->trigger(); });
                }
        };

    } // namespace ps16dz
} // namespace esphome