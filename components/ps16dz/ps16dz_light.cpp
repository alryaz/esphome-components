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

        void PS16DZLight::setup_state(light::LightState *state) { this->state_ = state; }

        void PS16DZLight::write_state(light::LightState *state)
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

                const int dimmer_value = round(esphome_dimmer_value * 100);

                execute_command(switch_state, dimmer_value);
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

                        bool old_state;
                        float old_value;

                        this->state_->current_values_as_binary(&old_state);
                        this->state_->current_values_as_brightness(&old_value);

                        old_value = round(old_value * 100);

                        bool new_state = old_state;
                        float new_value = old_value;

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
                                new_value = atoi(token3);
                                // ESP_LOGD("ps16dz", "New brightness %f%%", new_value);
                            }
                            else if (!strncmp(token2, "\"sequence\"", 10))
                            {
                                // ESP_LOGD("ps16dz", "New timestamp %s", token3);
                            }
                            token = strtok_r(nullptr, ",", &end_str);
                        }

                        if (old_value == new_value)
                        {
                            // ESP_LOGD("ps16dz", "Update");
                            serial_send_ok();
                        }

                        if ((old_state != new_state) || (old_value != new_value))
                        {
                            // ESP_LOGD("ps16dz", "Values: %s ? %s, %f ? %f", old_state ? "on" : "off", new_state ? "on" : "off", old_value, new_value);
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
                    else if (!strncmp(this->read_buffer_ + 3, "SETTING", 7))
                    {
                        bool is_entered = strncmp(this->read_buffer_ + 10, "=exit", 5);
                        
                        if (is_entered) {
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