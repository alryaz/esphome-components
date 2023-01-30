#pragma once

#include "esphome/core/component.h"
#include "../hekr.h"
#include "esphome/components/switch/switch.h"

namespace esphome
{
    namespace hekr
    {

        class HekrSwitch : public switch_::Switch, public Component, public Parented<Hekr>
        {
        public:
            void setup() override;
            void dump_config() override;

            void process_command(const uint8_t *data, const uint8_t length);

            void set_optimistic(bool optimistic) { this->optimistic_ = optimistic; }

            void set_command(uint8_t command)
            {
                this->command_ = command;
                this->has_command_ = true;
            }
            void set_position(uint8_t position) { this->position_ = position; }

            void set_command_on_static(const std::vector<uint8_t> command_on_static)
            {
                this->command_on_static_ = command_on_static;
                this->command_on_is_template_ = false;
            }
            void set_command_on_dynamic(const std::function<std::vector<uint8_t>()> func)
            {
                this->command_on_template_ = func;
                this->command_on_is_template_ = true;
            }

            void set_command_off_static(const std::vector<uint8_t> command_off_static)
            {
                this->command_off_static_ = command_off_static;
                this->command_off_is_template_ = false;
            }
            void set_command_off_dynamic(const std::function<std::vector<uint8_t>()> func)
            {
                this->command_off_template_ = func;
                this->command_off_is_template_ = true;
            }

            void set_value_off(const uint8_t value_off)
            {
                this->value_off_ = value_off;
            }

            void set_value_on(const uint8_t value_on)
            {
                this->value_on_ = value_on;
            }

        protected:
            void write_state(bool state) override;

            bool has_command_{false};
            uint8_t command_{0};
            uint8_t position_{0};
            uint8_t value_on_{0};
            uint8_t value_off_{1};
            bool optimistic_{false};

            bool command_on_is_template_{false};
            std::vector<uint8_t> command_on_static_{};
            std::function<std::vector<uint8_t>()> command_on_template_{};

            bool command_off_is_template_{false};
            std::vector<uint8_t> command_off_static_{};
            std::function<std::vector<uint8_t>()> command_off_template_{};
        };

    } // namespace hekr
} // namespace esphome
