/*
  sherlock_s2.h - Sonoff D1 Dimmer support for ESPHome

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
  https://github.com/alryaz/esphome-components/blob/main/components/sherlock_s2/README.md
*/

#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include <cstring>

namespace esphome
{
    namespace sherlock_s2
    {
        const uint8_t SHERLOCK_S2_READ_BUFFER_SIZE = 80;

        class SherlockS2LockStateTrigger : public Trigger<float>
        {};

        class SherlockS2Component : public uart::UARTDevice, public Component
        {
        protected:
            uint8_t read_pos_{0};
            char read_buffer_[SHERLOCK_S2_READ_BUFFER_SIZE]{};
            sensor::Sensor *voltage_sensor_{nullptr};
            sensor::Sensor *battery_level_sensor_{nullptr};
            binary_sensor::BinarySensor *lock_state_sensor_{nullptr};

            bool is_performing_action_{false};
            bool is_checking_for_stall_{false};
            bool is_unlocking_{false};
            float last_unstall_duration_{0};

            CallbackManager<void()> action_start_callbacks_{};

            std::vector<SherlockS2LockStateTrigger *> lock_triggers_{};
            std::vector<SherlockS2LockStateTrigger *> unlock_triggers_{};

        public:
            void set_voltage_sensor(sensor::Sensor *voltage_sensor) { this->voltage_sensor_ = voltage_sensor; }
            void set_battery_level_sensor(sensor::Sensor *battery_level_sensor) { this->battery_level_sensor_ = battery_level_sensor; }

            void add_on_action_start_callback(std::function<void()> &&callback) { this->action_start_callbacks_.add(std::move(callback)); }
            void add_on_lock_trigger(SherlockS2LockStateTrigger *trig) { this->lock_triggers_.push_back(trig); }
            void add_on_unlock_trigger(SherlockS2LockStateTrigger *trig) { this->unlock_triggers_.push_back(trig); }

            void loop() override;
        };

        class SherlockS2ActionStartTrigger : public Trigger<>
        {
        public:
            explicit SherlockS2ActionStartTrigger(SherlockS2Component *parent)
            {
                parent->add_on_action_start_callback([this]()
                                                     { this->trigger(); });
            }
        };

    } // namespace sherlock_s2
} // namespace esphome