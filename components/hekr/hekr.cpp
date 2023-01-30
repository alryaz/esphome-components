#include "hekr.h"

namespace esphome
{
    namespace hekr
    {
        static const char *const TAG = "hekr";

        void Hekr::setup() {}

        void Hekr::loop()
        {
            if (this->available())
            {
                if (this->read_pos_ == sizeof(this->read_buffer_))
                {
                    // check if buffer has shifted
                    ESP_LOGW(TAG, "frame overflow");
                    this->read_pos_ = 0;
                }

                // prepare byte placeholder, read length
                uint8_t read_byte, read_length = (this->read_pos_ > 0) ? this->read_buffer_[1] : 0;

                // read byte from uart
                this->read_byte(&read_byte);

                if (this->read_pos_ == 0)
                {
                    if (read_byte == FRAME_START)
                    {
                        ESP_LOGV(TAG, "found frame start");

                        // initialize frame reading
                        this->read_buffer_[0] = FRAME_START; // add frame
                        this->read_buffer_[1] = 0;           // reset read length
                        this->read_pos_ = 1;
                    }
                }
                else if (this->read_pos_ == 1)
                {
                    // record length
                    if ((read_byte < 0x06) || (read_byte > 0xFE))
                    {
                        // length does not fit the boundaries, discard
                        ESP_LOGW(TAG, "read length invalid");
                        this->read_pos_ = 0;
                    }
                    else
                    {
                        // append length to total buffer
                        this->read_buffer_[this->read_pos_++] = read_byte;
                    }
                }
                else if (this->read_pos_ < read_length)
                {
                    // continue frame reading
                    this->read_buffer_[this->read_pos_++] = read_byte;
                }

                // prepare for frame processing
                if ((this->read_pos_ > 0) && (this->read_pos_ == read_length))
                {
                    ESP_LOGV(TAG, "handling frame: %s", format_hex_pretty(this->read_buffer_, read_length).c_str());

                    // finish reading frame, initialize handling
                    this->handle_frame(this->read_buffer_);

                    // reset frame to initial position
                    this->read_pos_ = 0;
                }
            }
        }

        void Hekr::register_listener(const uint8_t command, const std::function<void(const uint8_t*, const uint8_t)> &func)
        {
            auto listener = HekrDeviceReportListener{
                .command = command,
                .func = func,
            };
            this->listeners_.push_back(listener);
        }

        void Hekr::send_frame(const uint8_t frame_type, const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
            auto *frame = new uint8_t[length + 5];
            uint8_t sum = 0;

            // build surrounding data
            sum += (frame[0] = FRAME_START);
            sum += (frame[1] = length + 5);
            sum += (frame[2] = frame_type);
            sum += (frame[3] = (frame_id == 0 ? ++this->last_frame_id_ : this->last_frame_id_ = frame_id));
            for (uint8_t i = 0; i < length; i++)
                sum += (frame[4 + i] = data[i]);
            frame[4 + length] = sum;

            ESP_LOGD(TAG, "sending frame: %s", format_hex_pretty(frame, length + 5).c_str());

            // commit frame
            this->write_array(frame, length + 5);

            // free up memory
            delete frame;
        }

        void Hekr::handle_frame(const uint8_t *buffer)
        {
            // validate frame checksum
            uint8_t checksum = 0;
            const uint8_t frame_length = buffer[1];

            if (frame_length < 5)
            {
                ESP_LOGW(TAG, "invalid frame: too short");
                return;
            }

            uint8_t frame_id = buffer[3];

            for (uint8_t i = 0; i < (frame_length - 1); i++)
            {
                checksum += buffer[i];
            }

            if (checksum != buffer[frame_length - 1])
            {
                ESP_LOGW(TAG, "invalid frame: bad checksum (%d != %d)", buffer[frame_length - 1], checksum);
                this->send_frame_error(frame_id, 0x03);
                return;
            }

#ifdef USE_TEXT_SENSOR
            if (this->last_frame_sensor_ != nullptr)
            {
                this->last_frame_sensor_->publish_state(format_hex(buffer, frame_length));
            }
#endif

            // process frame contents
            const uint8_t *data = buffer + 4, length = frame_length - 5;

            switch (buffer[2])
            {
            case 0x01:
                this->handle_frame_report(frame_id, data, length);
                break;
            case 0x02:
                this->handle_frame_command(frame_id, data, length);
                break;
            case 0xFE:
                this->handle_frame_control(frame_id, data, length);
                break;
            case 0xFF:
                this->handle_frame_error(frame_id, data, length);
                break;
            default:
                this->send_frame_error_unsupported(frame_id);
                break;
            }
        }

        void Hekr::handle_frame_command(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
            const uint8_t command_id = data[0], *command_data = data + 1;

            if (length > 0)
            {
                const uint8_t command = *data, *command_data = data + 1, command_length = length - 1;
                ESP_LOGV(TAG, "command: %u, data: %s", command, format_hex_pretty(command_data, command_length).c_str());
            }
            else
            {
                ESP_LOGV(TAG, "empty acknowledgement received");
            }
        }

        void Hekr::handle_frame_report(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
            if (length > 0)
            {
                const uint8_t command = *data, *command_data = data + 1, command_length = length - 1;
                ESP_LOGV(TAG, "command: %u, data: %s", command, format_hex_pretty(command_data, command_length).c_str());

                for (auto &listener : this->listeners_)
                {
                    if (listener.command == command) {
                        ESP_LOGV(TAG, "delegating command to listener");
                        listener.func(command_data, command_length);
                    }
                }
            }
            else
            {
                ESP_LOGV(TAG, "empty report received");
            }

            // @TODO: acknowledgement
        }

        void Hekr::handle_frame_control(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
            const uint8_t *fc_data = data + 1, fc_length = length - 1;

            switch (*data)
            {
            case 0x01:
                this->handle_frame_control_module_query(frame_id, fc_data, fc_length);
                break;
            case 0x02:
                this->handle_frame_control_soft_restart(frame_id, fc_data, fc_length);
                break;
            case 0x03:
                this->handle_frame_control_factory_reset(frame_id, fc_data, fc_length);
                break;
            case 0x04:
                this->handle_frame_control_enter_config(frame_id, fc_data, fc_length);
                break;
            case 0x05:
                this->handle_frame_control_enter_sleep(frame_id, fc_data, fc_length);
                break;
            case 0x06:
                this->handle_frame_control_exit_sleep(frame_id, fc_data, fc_length);
                break;
            case 0x10:
                this->handle_frame_control_factory_test(frame_id, fc_data, fc_length);
                break;
            case 0x11:
                this->handle_frame_control_validate_product_key(frame_id, fc_data, fc_length);
                break;
            case 0x20:
                this->handle_frame_control_version_request(frame_id, fc_data, fc_length);
                break;
            case 0x21:
                this->handle_frame_control_validate_product_key(frame_id, fc_data, fc_length);
                break;
            default:
                this->send_frame_error_unsupported(frame_id);
                break;
            }
        }

        void Hekr::handle_frame_control_module_query(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
        }

        void Hekr::handle_frame_control_soft_restart(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
        }

        void Hekr::handle_frame_control_factory_reset(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
        }

        void Hekr::handle_frame_control_enter_config(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
        }

        void Hekr::handle_frame_control_factory_test(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
            if (length != 2)
            {
                // @TODO: error reporting
                return;
            }
            ESP_LOGD(TAG, "factory test requested");
            uint8_t response_length = length + 1,
                    *response_frame = new uint8_t[response_length];
            memcpy(response_frame, data, length * sizeof(uint8_t));
            if (data[1])
                ESP_LOGD(TAG, "factory test mode disabled");
            else
                ESP_LOGD(TAG, "factory test mode enabled");
        }

        void Hekr::handle_frame_control_validate_product_key(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
            // validate product key
            // - device sends product key
            // - module responds with 0x01 for success, 0x00 for error
            // @TODO: actually use this logic
            ESP_LOGD(TAG, "product key validation requested");
            uint8_t response_length = 2,
                    *response_frame = new uint8_t[response_length]{0x11, 0x01};
            response_length = 2;
        }

        void Hekr::handle_frame_control_enter_sleep(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
        }

        void Hekr::handle_frame_control_exit_sleep(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
        }

        void Hekr::handle_frame_control_version_request(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
            ESP_LOGD(TAG, "version request");
            uint8_t response_length = 6,
                    *response_frame = new uint8_t[response_length]{0x20, 0x04, 0x01, 0x0F, 0x01, 0x01};
            this->send_frame(frame_id, response_frame, response_length);
            delete response_frame;
        }

        void Hekr::handle_frame_control_set_product_key(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
            if (length < 2)
            {
                return;
            }

            ESP_LOGD(TAG, "product key: 0x%s", format_hex(data, length).c_str());
            uint8_t response_length = length + 1,
                    *response_frame = new uint8_t[response_length];
            response_frame[0] = 0x21;
            memcpy(response_frame + 1, data, length * sizeof(uint8_t));
            this->send_frame(frame_id, response_frame, response_length);
            delete response_frame;
        }

        void Hekr::handle_frame_error(const uint8_t frame_id, const uint8_t *data, const uint8_t length)
        {
        }
    }
}
