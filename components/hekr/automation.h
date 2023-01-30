#pragma once

#include "hekr.h"
#include "esphome/core/automation.h"

#include <vector>

namespace esphome
{
    namespace hekr
    {

        template <typename... Ts>
        class HekrCommandAction : public Action<Ts...>, public Parented<Hekr>
        {
        public:
            void set_command_template(std::function<std::vector<uint8_t>(Ts...)> func)
            {
                this->command_func_ = func;
            }
            void set_command_static(const std::vector<uint8_t> &data)
            {
                this->command_is_static_ = true;
                this->command_static_ = data;
            }

            void play(Ts... x) override
            {
                auto data = this->command_is_static_ ? this->command_static_ : this->command_func_(x...);
                this->parent_->send_frame_command(&data[0], data.size());
            }

        protected:
            bool command_is_static_{false};
            std::function<std::vector<uint8_t>(Ts...)> command_func_{};
            std::vector<uint8_t> command_static_{};
        };

    } // namespace hekr
} // namespace esphome
