#pragma once
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome
{
    namespace hekr
    {
        static const uint8_t FRAME_START = 0x48;

        struct HekrDeviceReportListener
        {
            uint8_t command;
            std::function<void(const uint8_t*, const uint8_t)> func;
        };

        class Hekr : public Component, public uart::UARTDevice
        {
        public:
            float get_setup_priority() const override { return setup_priority::LATE; }
            void setup() override;
            void loop() override;

            void register_listener(const uint8_t command, const std::function<void(const uint8_t*, const uint8_t)> &func);

            void send_frame(const uint8_t frame_type, const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void send_frame(const uint8_t frame_type, const uint8_t *data, const uint8_t length)
            {
                this->send_frame(frame_type, ++this->last_frame_id_, data, length);
            }

#ifdef USE_TEXT_SENSOR
            // entity related
            void set_last_frame_sensor(text_sensor::TextSensor *last_frame_sensor) { this->last_frame_sensor_ = last_frame_sensor; }
#endif

            // command frame related
            void send_frame_command(const uint8_t *data, const uint8_t length)
            {
                this->send_frame(0x02, data, length);
            }
            void send_frame_command(const uint8_t command_id)
            {
                this->send_frame_command(nullptr, 0);
            }
            void send_frame_command(const uint8_t command_id, const uint8_t *command_data, const uint8_t command_length)
            {
                uint8_t length = command_length + 1, *data = new uint8_t[length];
                data[0] = command_id;
                memcpy(data + 1, command_data, command_length * sizeof(uint8_t));
                this->send_frame_command(data, length);
                delete data;
            }

            // error frame related
            void send_frame_error(const uint8_t *data, const uint8_t length)
            {
                this->send_frame(0xFF, data, length);
            }
            void send_frame_error(const uint8_t frame_id, const uint8_t error_type, const uint8_t extra_data)
            {
                uint8_t error_data[] = {error_type, extra_data};
                this->send_frame_error(error_data, 2);
            }
            void send_frame_error(const uint8_t frame_id, const uint8_t error_type)
            {
                this->send_frame_error(frame_id, error_type, 0x00);
            }
            void send_frame_error_unsupported(const uint8_t frame_id)
            {
                this->send_frame_error(frame_id, 0x04);
            }

        protected:
            void handle_frame(const uint8_t *buffer);

            // command frame related
            void handle_frame_command(const uint8_t frame_id, const uint8_t *data, const uint8_t length);

            // report frame related
            void handle_frame_report(const uint8_t frame_id, const uint8_t *data, const uint8_t length);

            // control frame related
            void handle_frame_control(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_module_query(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_soft_restart(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_factory_reset(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_enter_config(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_enter_sleep(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_exit_sleep(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_version_request(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_validate_product_key(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_factory_test(const uint8_t frame_id, const uint8_t *data, const uint8_t length);
            void handle_frame_control_set_product_key(const uint8_t frame_id, const uint8_t *data, const uint8_t length);

            // error frame related
            void handle_frame_error(const uint8_t frame_id, const uint8_t *data, const uint8_t length);

#ifdef USE_TEXT_SENSOR
            text_sensor::TextSensor *last_frame_sensor_{nullptr};
#endif

            uint8_t read_buffer_[0xFF]{0};
            uint8_t read_pos_{0};
            uint8_t last_frame_id_{0};
            std::vector<HekrDeviceReportListener> listeners_;
        };
    }
}
