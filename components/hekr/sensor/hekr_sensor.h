#pragma once

#include "esphome/core/component.h"
#include "../hekr.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome
{
    namespace hekr
    {

        class HekrSensor : public sensor::Sensor, public Component, public Parented<Hekr>
        {
        public:
            void setup() override;
            void dump_config() override;

            void process_command(const uint8_t *data, const uint8_t length);

            void set_command(uint8_t command) { this->command_ = command; }
            void set_position(uint8_t position) { this->position_ = position; }
            void set_length(uint8_t length) { this->length_ = length; }
            void set_reversed(bool reversed) { this->reversed_ = reversed; }

        protected:
            uint8_t command_{0};
            uint8_t position_{0};
            uint8_t length_{1};
            bool reversed_{false};
        };

    } // namespace hekr
} // namespace esphome
