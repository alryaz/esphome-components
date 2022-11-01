/*
  ps16dz_light.cpp - PS-16-DZ Dimmer support for ESPHome

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

#include "ps16dz_light.h"

namespace esphome
{
    namespace ps16dz
    {

        static const char *const TAG = "ps16dz";

        void PS16DZLight::serial_send_(const char *tx_buffer)
        {
            ESP_LOGD(TAG, "Ctrl: %s", tx_buffer);

            this->write_str(tx_buffer);
            this->write_byte(0x1B);
            this->flush();
        }

        void PS16DZLight::serial_send_ok_()
        {
            char tx_buffer[16];
            snprintf(
                tx_buffer,
                sizeof(tx_buffer),
                "AT+SEND=ok");
            this->serial_send_(tx_buffer);
        }

        light::LightTraits PS16DZLight::get_traits()
        {
            auto traits = light::LightTraits();
            traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
            return traits;
        }

        void PS16DZLight::write_state(light::LightState *state)
        {
            bool new_binary;
            float esphome_brightness;

            state->current_values_as_binary(&new_binary);
            state->current_values_as_brightness(&esphome_brightness);

            const uint8_t new_brightness = (uint8_t)remap<float, float>(
                esphome_brightness,
                0.0, 1.0,
                (float)this->min_value_,
                (float)this->max_value_);

            char tx_buffer[80];
            snprintf(
                tx_buffer,
                sizeof(tx_buffer),
                "AT+UPDATE=\"sequence\":\"%lld\",\"switch\":\"%s\",\"bright\":%d",
                ++this->sequence_number,
                new_binary ? "on" : "off",
                new_brightness);

            this->serial_send_(tx_buffer);

            this->last_binary_ = new_binary;
            this->last_brightness_ = new_brightness;
        }

        void PS16DZLight::dump_config()
        {
            ESP_LOGCONFIG(TAG, "PS-16-DZ Dimmer: '%s'", this->state_ ? this->state_->get_name().c_str() : "");
            ESP_LOGCONFIG(TAG, "  Minimal brightness: %d", this->min_value_);
            ESP_LOGCONFIG(TAG, "  Maximal brightness: %d", this->max_value_);
        }

        void PS16DZLight::loop()
        {
            uint8_t serial_in_byte;
            while (this->available())
            {
                this->read_byte(&serial_in_byte);
                if (serial_in_byte != 0x1B)
                {
                    // read another character of the message (0x1B is stop sign)
                    if (serial_in_byte < 0x80)
                    {
                        // consider only printable characters
                        if (this->read_pos_ == PS16DZ_READ_BUFFER_LENGTH)
                        {
                            this->read_pos_ = 0;
                        }
                        if ((this->read_pos_ > 0) || ('A' == serial_in_byte))
                        {
                            this->read_buffer_[this->read_pos_++] = serial_in_byte;
                        }
                    }
                }
                else
                {
                    // message is finished, attempt to parse
                    this->read_buffer_[this->read_pos_++] = 0x00;

                    ESP_LOGD(TAG, "Rcvd %s", this->read_buffer_);

                    if (!strncmp(this->read_buffer_ + 3, "RESULT", 6))
                    {
                    }
                    else if (!strncmp(this->read_buffer_ + 3, "UPDATE", 6))
                    {
                        char *end_str;
                        char *string = this->read_buffer_ + 10;
                        char *token = strtok_r(string, ",", &end_str);

                        // prepare state
                        bool new_binary = this->last_binary_;
                        uint8_t new_brightness = this->last_brightness_;

                        while (token != nullptr)
                        {
                            char *end_token;
                            char *token2 = strtok_r(token, ":", &end_token);
                            char *token3 = strtok_r(nullptr, ":", &end_token);

                            if (!strncmp(token2, "\"switch\"", 8))
                            {
                                new_binary = !strncmp(token3, "\"on\"", 4);
                                // ESP_LOGD(TAG, "New state %d", new_state);
                            }
                            else if (!strncmp(token2, "\"bright\"", 8))
                            {
                                // clamp new brightness value if required
                                new_brightness = (uint8_t)atoi(token3);

                                if (new_brightness > this->max_value_)
                                {
                                    ESP_LOGD(TAG, "Brightness clamped down: %d -> %d", new_brightness, this->max_value_);
                                    new_brightness = this->max_value_;
                                }
                                else if (new_brightness < this->min_value_)
                                {
                                    ESP_LOGD(TAG, "Brightness clamped up: %d -> %d", new_brightness, this->min_value_);
                                    new_brightness = this->min_value_;
                                }
                            }
                            else if (!strncmp(token2, "\"sequence\"", 10))
                            {
                                // ...
                            }
                            token = strtok_r(nullptr, ",", &end_str);
                        }

                        if (this->last_brightness_ == new_brightness)
                        {
                            // ESP_LOGD(TAG, "Update");
                            this->serial_send_ok_();
                        }

                        if (new_binary != this->last_binary_)
                        {
                            auto call = this->state_->make_call();
                            call.set_state(new_binary);

                            if (!new_binary)
                            {
                                call.set_transition_length(0);
                            }

                            call.perform();
                        }
                        else if (new_brightness != this->last_brightness_)
                        {
                            float esphome_brightness = remap<float, float>(
                                (float)new_brightness,
                                (float)this->min_value_,
                                (float)this->max_value_,
                                0.0, 1.0);
                            auto call = this->state_->make_call();
                            call.set_brightness(esphome_brightness);
                            call.set_state(esphome_brightness != 0);
                            call.set_transition_length(0);
                            call.perform();
                        }
                    }
                    else if (!strncmp(this->read_buffer_ + 3, "SETTING", 7))
                    {
                        if (strncmp(this->read_buffer_ + 10, "=exit", 5))
                        {
                            if (!this->in_settings_mode_)
                            {
                                this->in_settings_mode_ = true;
                                ESP_LOGD(TAG, "Entered settings mode");
                                this->settings_enter_callback_.call();
                            }
                        }
                        else
                        {
                            this->in_settings_mode_ = false;
                            ESP_LOGD(TAG, "Exited settings mode");
                            this->settings_exit_callback_.call();
                        }
                    }

                    // clear read
                    memset(this->read_buffer_, 0, PS16DZ_READ_BUFFER_LENGTH);
                    this->read_pos_ = 0;
                }
            }
        }

        void PS16DZLight::add_on_settings_enter_callback(std::function<void()> &&callback)
        {
            this->settings_enter_callback_.add(std::move(callback));
        }

        void PS16DZLight::add_on_settings_exit_callback(std::function<void()> &&callback)
        {
            this->settings_exit_callback_.add(std::move(callback));
        }
    } // namespace ps16dz
} // namespace esphome