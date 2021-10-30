/*********************************************************************************************\
 * PS 16 DZ Serial Dimmer
\*********************************************************************************************/

#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_traits.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

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
            bool state_received_{false};
            bool in_settings_mode_{false};
            light::LightState *state_{nullptr};
            time::RealTimeClock *time_{nullptr};

        protected:
            char read_buffer_[PS16DZ_READ_BUFFER_LENGTH];
            size_t read_pos_{0};
            CallbackManager<void()> settings_enter_callback_{};
            CallbackManager<void()> settings_exit_callback_{};

        public:
            void set_time(time::RealTimeClock *time) { time_ = time; }
            time::RealTimeClock *get_time() const { return time_; }
            light::LightTraits get_traits() override
            {
                auto traits = light::LightTraits();
                traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
                return traits;
            }

            void loop() override;
            void serial_send(const char *tx_buffer);
            void serial_send_ok(void);
            void execute_command(const bool &switch_state, const int &dimmer_value);
            void write_state(light::LightState *state);
            void setup_state(light::LightState *state);

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