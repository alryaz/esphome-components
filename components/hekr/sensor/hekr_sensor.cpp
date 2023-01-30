#include "esphome/core/log.h"
#include "hekr_sensor.h"

using namespace std;

namespace esphome
{
    namespace hekr
    {

        static const char *const TAG = "hekr.sensor";

        void HekrSensor::setup()
        {
            this->parent_->register_listener(this->command_, [this](const uint8_t *data, const uint8_t length)
                                             { this->process_command(data, length); });
        }

        void HekrSensor::process_command(const uint8_t *data, const uint8_t length)
        {
            if (this->length_ < 1)
            {
                ESP_LOGE(TAG, "Sensor ended up with zero length");
                return;
            }

            if (this->length_ > 8)
            {
                ESP_LOGE(TAG, "Sensor length too long");
                return;
            }

            if ((this->position_ + this->length_) > length)
            {
                ESP_LOGE(TAG, "Sensor value extract exceeds length");
                return;
            }

            const uint8_t *value = data + this->position_;

            uint64_t sum = 0;

            ESP_LOGV(TAG, "will publish hex value: %s", format_hex_pretty(value, this->length_).c_str());
            
            if (this->reversed_) {
                memcpy(&sum, value, this->length_ * sizeof(uint8_t));
            } else {
                // @TODO: optimize
                uint8_t *rev_arr = new uint8_t[this->length_];
                for (uint8_t i = 0; i < this->length_; i++)
                    rev_arr[i] = value[this->length_ - i - 1];
                memcpy(&sum, rev_arr, this->length_ * sizeof(uint8_t));
                delete rev_arr;
            }

            this->publish_state((float) sum);
        }

        void HekrSensor::dump_config()
        {
            LOG_SENSOR("", "Hekr Sensor", this);
            ESP_LOGCONFIG(TAG, "  Sensor uses command %u", this->command_);
            ESP_LOGCONFIG(TAG, "  Sensor uses position %u", this->position_);
            ESP_LOGCONFIG(TAG, "  Sensor uses length %u", this->length_);

            // @TODO: check if true
            if (this->reversed_)
            {
                ESP_LOGCONFIG(TAG, "  Sensor is little endian");
            }
            else
            {
                ESP_LOGCONFIG(TAG, "  Sensor is big endian");
            }
        }

    } // namespace hekr
} // namespace esphome
