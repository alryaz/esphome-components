#include "esphome/core/log.h"
#include "hekr_switch.h"

using namespace std;

namespace esphome
{
    namespace hekr
    {
        static const char *const TAG = "hekr.switch";

        void HekrSwitch::setup()
        {
            if (this->has_command_)
            {
                this->parent_->register_listener(this->command_, [this](const uint8_t *data, const uint8_t length)
                                                 { this->process_command(data, length); });
            }
        }

        void HekrSwitch::process_command(const uint8_t *data, const uint8_t length)
        {
            if (this->position_ >= length)
            {
                ESP_LOGE(TAG, "Switch value extract exceeds length");
                return;
            }

            const uint8_t value = *(data + this->position_);
            if (value == this->value_on_)
            {
                this->publish_state(true);
            }
            else if (value == this->value_off_)
            {
                this->publish_state(false);
            }
            else
            {
                ESP_LOGW(TAG, "Switch value unexpected: %u", value);
            }
        }

        void HekrSwitch::write_state(const bool state)
        {
            auto command = state ? (this->command_on_is_template_ ? this->command_on_template_() : this->command_on_static_) : (this->command_off_is_template_ ? this->command_off_template_() : this->command_off_static_);
            this->parent_->send_frame_command(&command[0], command.size());

            if (this->optimistic_)
                this->publish_state(state);
        }

        void HekrSwitch::dump_config()
        {
            LOG_SWITCH("", "Hekr Switch", this);

            ESP_LOGCONFIG(TAG, "  Switch uses turn off command %s", this->command_off_is_template_ ? "template" : format_hex_pretty(this->command_off_static_).c_str());
            ESP_LOGCONFIG(TAG, "  Switch uses turn on command %s", this->command_on_is_template_ ? "template" : format_hex_pretty(this->command_on_static_).c_str());

            if (this->has_command_)
            {
                ESP_LOGCONFIG(TAG, "  Switch uses state command %u", this->command_);
                ESP_LOGCONFIG(TAG, "  Switch uses byte position %u", this->position_);
                ESP_LOGCONFIG(TAG, "  Switch uses value %u to assume on", this->value_on_);
                ESP_LOGCONFIG(TAG, "  Switch uses value %u to assume off", this->value_off_);
            }
            else
            {
                ESP_LOGCONFIG(TAG, "  Switch does not provide data updates");
            }
        }

    } // namespace hekr
} // namespace esphome
