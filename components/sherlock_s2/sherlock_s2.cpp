/*
  sherlock_s2.cpp - Sherlock S2 lock support for ESPHome

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

#include "sherlock_s2.h"

#include "esphome/core/component.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace sherlock_s2
    {

        void SherlockS2Component::loop()
        {
            uint8_t serial_in_byte;
            if (this->available())
            {
                this->read_byte(&serial_in_byte);
                if (serial_in_byte == '\n')
                {
                    return;
                }
                if (serial_in_byte == '\r')
                {
                    // message is finished, attempt to parse
                    this->read_buffer_[this->read_pos_++] = 0x00;

                    ESP_LOGD("sherlock_s2", "Rcvd %s", this->read_buffer_);

                    char *end_str;
                    char *string = this->read_buffer_;
                    char *token = strtok_r(string, ",", &end_str);

                    bool is_unstall_message = false, finished_performing_action = false;

                    while (token != nullptr)
                    {
                        char *end_token;
                        char *token2 = strtok_r(token, ":", &end_token);
                        char *token3 = strtok_r(nullptr, ":", &end_token);

                        if (!strncmp(token2, "Start Stall Check", 17))
                        {
                            this->is_checking_for_stall_ = true;
                            this->is_performing_action_ = true;
                            this->is_unlocking_ = false;
                            this->last_unstall_duration_ = 0;

                            ESP_LOGD("sherlock_s2", "Stall check started (lock is moving)");

                            this->action_start_callbacks_.call();
                        }
                        else if (!strncmp(token2, "Stall Int Flag", 14))
                        {
                            this->is_checking_for_stall_ = false;

                            is_unstall_message = true;

                            ESP_LOGD("sherlock_s2", "Lock stalled (move finished)");
                        }
                        else if (!strncmp(token2, "time", 4))
                        {
                            if (is_unstall_message)
                            {
                                this->last_unstall_duration_ = ((float)atoi(token3)) / 100;

                                ESP_LOGD("sherlock_s2", "Lock stalled after %f seconds", this->last_unstall_duration_);
                            }
                        }
                        else if (!strncmp(token2, "Open Lock", 9))
                        {
                            // Actual message: Open Lock Delay Hori Finish
                            this->is_unlocking_ = true;
                            ESP_LOGD("sherlock_s2", "Marking action as an unlocking action");
                        }
                        else if (!strncmp(token2, "Start Hori Det", 14))
                        {
                            ESP_LOGD("sherlock_s2", "Reversing to determine horizontal position");
                        }
                        else if (!strncmp(token2, "Hori Det Int", 13))
                        {
                            ESP_LOGD("sherlock_s2", "Horizontal position determined");
                            finished_performing_action = true;
                        }
                        else if (!strncmp(token2, "Led_Mode", 8))
                        {
                            ESP_LOGV("sherlock_s2", "LED State: %d", atoi(token3));
                        }
                        else if (!strncmp(token2, "Average_Val", 11))
                        {
                            ESP_LOGV("sherlock_s2", "%s == %d", token2, atoi(token3));
                        }
                        else if (!strncmp(token2, "Batt_V", 6))
                        {
                            float battery_voltage = atoi(token3);
                            battery_voltage /= 1000;

                            ESP_LOGV("sherlock_s2", "Battery voltage: %f", battery_voltage);

                            if (this->battery_voltage_sensor_ != nullptr)
                            {
                                this->battery_voltage_sensor_->publish_state(battery_voltage);
                            }
                        }
                        else if (!strncmp(token2, "Vol_Per", 7))
                        {
                            int battery_percentage = atoi(token3);

                            ESP_LOGV("sherlock_s2", "Battery percentage: %d%%", atoi(token3));

                            if (this->battery_level_sensor_ != nullptr)
                            {
                                battery_percentage = battery_percentage > 100 ? 100 : battery_percentage;
                                battery_percentage = battery_percentage < 0 ? 0 : battery_percentage;

                                this->battery_level_sensor_->publish_state(battery_percentage);
                            }
                        }
                        else
                        {
                            ESP_LOGW("sherlock_s2", "Unknown token: %s", token);
                        }
                        token = strtok_r(nullptr, ",", &end_str);
                    }

                    if (finished_performing_action)
                    {
                        this->is_performing_action_ = false;
                        
                        for (auto *trigger : (this->is_unlocking_ ? this->unlock_triggers_ : this->lock_triggers_))
                            trigger->trigger(this->last_unstall_duration_);

                        ESP_LOGD("sherlock_s2", "New lock state: %s", (this->is_unlocking_ ? "unlocked" : "locked"));
                    }

                    // clear read
                    memset(this->read_buffer_, 0, sizeof(this->read_buffer_));
                    this->read_pos_ = 0;
                }
                else if (serial_in_byte < 0x80)
                {
                    // read until message is finished
                    // treat only valid printable characters, ignore noise
                    if (this->read_pos_ == sizeof(this->read_buffer_))
                    {
                        this->read_pos_ = 0;
                    }
                    this->read_buffer_[this->read_pos_++] = serial_in_byte;
                }
            }
        }

    } // namespace sherlock_s2
} // namespace esphome
