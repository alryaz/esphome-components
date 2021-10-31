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

#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace ps16dz
    {

        void PS16DZLight::serial_send(const char *tx_buffer)
        {
            this->write_str(tx_buffer);
            this->write_byte(0x1B);
            this->flush();
        }

        void PS16DZLight::serial_send_ok()
        {
            char tx_buffer[16];
            snprintf_P(
                tx_buffer,
                sizeof(tx_buffer),
                PSTR("AT+SEND=ok"));
            serial_send(tx_buffer);
        }

        void PS16DZLight::execute_command(const bool &switch_state, const int &dimmer_value)
        {
            auto now = this->time_->now();

            int adj_brightness = (dimmer_value < 10) ? 10 : dimmer_value;
            adj_brightness = (dimmer_value > 100) ? 100 : dimmer_value;

            char tx_buffer[80];
            snprintf_P(
                tx_buffer,
                sizeof(tx_buffer),
                PSTR("AT+UPDATE=\"sequence\":\"%ld%03d\",\"switch\":\"%s\",\"bright\":%d"),
                now.timestamp,
                millis() % 1000,
                switch_state ? "on" : "off",
                adj_brightness);

            // ESP_LOGD("ps16dz", "Ctrl: %s", tx_buffer);

            this->serial_send(tx_buffer);
        }

        void PS16DZLight::setup_state(light::LightState *state) {
            this->state_ = state;
            this->current_state_ = state->current_values.is_on();

            float esphome_brightness;
            
            if (this->current_state_) {
                state->current_values_as_brightness(&esphome_brightness);
            } else {
                // save original brightness to the device
                esphome_brightness = gamma_correct(state->current_values.get_brightness(), state->get_gamma_correct());
            }

            this->current_brightness_ = esphome_brightness * 100;
        }

        void PS16DZLight::write_state(light::LightState *state)
        {
            this->setup_state(state);

            if (this->state_received_)
            {
                this->state_received_ = false;
            }
            else
            {
                execute_command(this->current_state_, this->current_brightness_);
            }
        }

        void PS16DZLight::add_on_settings_enter_callback(std::function<void()> &&callback) {
            this->settings_enter_callback_.add(std::move(callback));
        }

        void PS16DZLight::add_on_settings_exit_callback(std::function<void()> &&callback) {
            this->settings_exit_callback_.add(std::move(callback));
        }

        void PS16DZLight::loop()
        {
            uint8_t serial_in_byte;
            while (this->available())
            {
                this->read_byte(&serial_in_byte);
                if (serial_in_byte != 0x1B)
                {
                    // read until message is finished
                    if (this->read_pos_ == PS16DZ_READ_BUFFER_LENGTH)
                    {
                        this->read_pos_ = 0;
                    }
                    if ((this->read_pos_ > 0) || ('A' == serial_in_byte))
                    {
                        this->read_buffer_[this->read_pos_++] = serial_in_byte;
                    }
                }
                else
                {
                    // message is finished, attempt to parse
                    this->read_buffer_[this->read_pos_++] = 0x00;

                    // ESP_LOGD("ps16dz", "Rcvd %s", this->read_buffer_);

                    if (!strncmp(this->read_buffer_ + 3, "RESULT", 6))
                    {
                    }
                    else if (!strncmp(this->read_buffer_ + 3, "UPDATE", 6))
                    {
                        char *end_str;
                        char *string = this->read_buffer_ + 10;
                        char *token = strtok_r(string, ",", &end_str);

                        // prepare state
                        bool new_state = this->current_state_;
                        int new_brightness = this->current_brightness_;

                        while (token != nullptr)
                        {
                            char *end_token;
                            char *token2 = strtok_r(token, ":", &end_token);
                            char *token3 = strtok_r(nullptr, ":", &end_token);

                            if (!strncmp(token2, "\"switch\"", 8))
                            {
                                new_state = !strncmp(token3, "\"on\"", 4);
                                // ESP_LOGD("ps16dz", "New state %d", new_state);
                            }
                            else if (!strncmp(token2, "\"bright\"", 8))
                            {
                                new_brightness = atoi(token3);
                                
                                new_brightness = new_brightness > 100 ? 100 : new_brightness;
                                new_brightness = new_brightness < 0 ? 0 : new_brightness;

                                // ESP_LOGD("ps16dz", "New brightness %f%%", new_value);
                            }
                            else if (!strncmp(token2, "\"sequence\"", 10))
                            {
                                // ESP_LOGD("ps16dz", "New timestamp %s", token3);
                            }
                            token = strtok_r(nullptr, ",", &end_str);
                        }

                        if (this->current_brightness_ == new_brightness)
                        {
                            // ESP_LOGD("ps16dz", "Update");
                            serial_send_ok();
                        }

                        if ((this->current_brightness_ != new_brightness) || (this->current_state_ != new_state))
                        {
                            this->state_received_ = true;
                            auto call = this->state_->make_call();
                            call.set_state(new_state);
                            call.set_brightness(((float) new_brightness) / 100);
                            call.set_transition_length(0); // effectively cancels looping caused by transition
                            call.perform();
                        }
                    }
                    else if (!strncmp(this->read_buffer_ + 3, "SETTING", 7))
                    {
                        if (strncmp(this->read_buffer_ + 10, "=exit", 5)) {
                            if (!this->in_settings_mode_) {
                                this->in_settings_mode_ = true;
                                ESP_LOGD("ps16dz", "Entered settings mode");
                                this->settings_enter_callback_.call();
                            }
                        } else {
                            this->in_settings_mode_ = false;
                            ESP_LOGD("ps16dz", "Exited settings mode");
                            this->settings_exit_callback_.call();
                        }
                    }

                    // clear read
                    memset(this->read_buffer_, 0, PS16DZ_READ_BUFFER_LENGTH);
                    this->read_pos_ = 0;
                }
            }
        }
    } // namespace ps16dz
} // namespace esphome