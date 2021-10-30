/*
  sonoff_d1_light.cpp - Sonoff D1 Dimmer support for ESPHome

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
  https://github.com/arendst/Tasmota/blob/development/tasmota/xdrv_37_sonoff_d1.ino
  https://github.com/alryaz/esphome-components/blob/main/components/sonoff_d1/README.md

  -----

  THANK YOU!
  Thanks to the team over at Tasmota for providing the serial codes to control the dimmer!
  View the source: https://github.com/arendst/Tasmota/blob/development/tasmota/xdrv_37_sonoff_d1.ino
*/

#include "sonoff_d1_light.h"

#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace sonoff_d1
    {

        void SonoffD1Light::execute_command(const bool &switch_state, const uint8_t &dimmer_value)
        {
            uint8_t tx_buffer[17] = {
                0xAA,
                0x55,
                0x01,
                0x04,
                0x00,
                0x0A,
                switch_state,  // idx: 6
                dimmer_value,  // idx: 7
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0xFF,
                0x00
            };

            for (uint32_t i = 0; i < sizeof(tx_buffer); i++)
            {
                if ((i > 1) && (i < sizeof(tx_buffer) - 1))
                {
                    tx_buffer[16] += tx_buffer[i];
                }

                // ESP_LOGD("sonoff_d1", "Sending byte %d", tx_buffer[i]);

                this->write_byte(tx_buffer[i]);
            }

            this->flush();
        }

        void SonoffD1Light::setup_state(light::LightState *state) { this->state_ = state; }

        void SonoffD1Light::write_state(light::LightState *state)
        {
            this->state_ = state;

            if (this->state_received_)
            {
                this->state_received_ = false;
            }
            else
            {
                bool switch_state;
                float esphome_dimmer_value;

                state->current_values_as_binary(&switch_state);
                state->current_values_as_brightness(&esphome_dimmer_value);

                const uint8_t dimmer_value = round(esphome_dimmer_value * 64);

                this->execute_command(switch_state, dimmer_value);
            }
        }

        void SonoffD1Light::loop()
        {
            if (this->available())
            {
                // Read the incoming byte:
                uint8_t serial_in_byte;

                this->read_byte(&serial_in_byte);

                // Say what you got:

                if (0xAA == serial_in_byte)
                {
                    this->read_length_ = 7;
                    this->read_pos_ = 0;

                    this->read_buffer_[this->read_pos_++] = serial_in_byte;

                    // ESP_LOGD("sonoff_d1", "Read message start (0xAA)");
                }
                else if (this->read_pos_ == sizeof(this->read_buffer_))
                {
                    ESP_LOGW("sonoff_d1", "Overflow prevented");
                    this->read_pos_ = 0;
                    this->read_length_ = 0;  // handle potential overflows
                }
                else if (this->read_length_)
                {
                    this->read_buffer_[this->read_pos_++] = serial_in_byte;

                    // ESP_LOGD("sonoff_d1", "Read regular byte %d (pos: %d, len: %d)", serial_in_byte, this->read_pos_, this->read_length_);

                    if (this->read_pos_ == 6)
                    {
                        this->read_length_ += serial_in_byte;
                    }

                    if (this->read_pos_ == this->read_length_)
                    {
                        uint8_t crc = 0;
                        for (uint32_t i = 2; i < this->read_length_ - 1; i++)
                        {
                            crc += this->read_buffer_[i];
                        }
                        if (crc == this->read_buffer_[this->read_length_ - 1])
                        {
                            if (this->read_pos_ < 8)
                            {
                                // ESP_LOGD("sonoff_d1", "Received ACK: %s", this->read_buffer_);
                            }
                            else
                            {
                                bool new_state = this->read_buffer_[6],
                                     old_state;
                                float new_value = this->read_buffer_[7],
                                      old_value;

                                if (new_value > 100) {
                                    ESP_LOGW("sonoff_d1", "Received abnormal dimmer value: %d", new_value);
                                    new_value = 100;
                                }

                                this->state_->current_values_as_binary(&old_state);
                                this->state_->current_values_as_brightness(&old_value);

                                old_value = round(old_value * 100);

                                if ((new_state != old_state) || (new_value != old_value))
                                {
                                    state_received_ = true;
                                    auto call = this->state_->make_call();
                                    if (old_state != new_state)
                                    {
                                        call.set_state(new_state);
                                    }
                                    if (old_value != new_value)
                                    {
                                        call.set_brightness(new_value / 100);
                                    }
                                    call.set_transition_length(0); // effectively cancels looping caused by transition
                                    call.perform();
                                }
                            }
                        }
                        this->read_pos_ = 0;
                        this->read_length_ = 0;
                    }
                }
            }
        }

    } // namespace sonoff_d1
} // namespace esphome