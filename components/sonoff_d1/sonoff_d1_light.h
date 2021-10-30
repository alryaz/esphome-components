/*
  sonoff_d1_light.h - Sonoff D1 Dimmer support for ESPHome

  Copyright © 2020 Jeff Rescignano
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
  https://jeffresc.dev/blog/2020-10-10
  https://github.com/JeffResc/Sonoff-D1-Dimmer
  https://github.com/arendst/Tasmota/blob/2d4a6a29ebc7153dbe2717e3615574ac1c84ba1d/tasmota/xdrv_37_sonoff_d1.ino#L119-L131
  https://github.com/alryaz/esphome-components/blob/main/components/sonoff_d1/README.md

  -----

  THANK YOU!
  Thanks to the team over at Tasmota for providing the serial codes to control the dimmer!
  View the source: https://github.com/arendst/Tasmota/blob/2d4a6a29ebc7153dbe2717e3615574ac1c84ba1d/tasmota/xdrv_37_sonoff_d1.ino#L119-L131
*/

#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_traits.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include <cstring>

namespace esphome
{
    namespace sonoff_d1
    {
        const uint8_t SONOFF_D1_READ_BUFFER_LENGTH = 80;

        class SonoffD1Light : public uart::UARTDevice, public light::LightOutput, public Component
        {
        protected:
            bool state_received_{false};
            uint8_t read_length_{0};
            uint8_t read_pos_{0};
            uint8_t read_buffer_[SONOFF_D1_READ_BUFFER_LENGTH]{};
            light::LightState *state_{nullptr};

        public:
            light::LightTraits get_traits() override
            {
                auto traits = light::LightTraits();
                traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
                return traits;
            }

            void loop() override;
            void execute_command(const bool &switch_state, const uint8_t &dimmer_value);
            void write_state(light::LightState *state);
            void setup_state(light::LightState *state);
        };

    } // namespace sonoff_d1
} // namespace esphome