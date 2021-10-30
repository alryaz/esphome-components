My custom components for ESPHome
==================================================

> Various ESPHome components implemented for my home devices:
> - **PS-16-DZ Dimmer**: Light switch with dimmer functionality
>
> [![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
> [![Maintained](https://img.shields.io/badge/Maintained-yes-green.svg)](https://github.com/alryaz/esphome-ps16dz/graphs/commit-activity)
>
> [![Donations accepted via](https://img.shields.io/badge/Donations%20accepted%20via-Yandex-red.svg)](https://money.yandex.ru/to/410012369233217)
> [![Donations accepted via](https://img.shields.io/badge/Donations%20accepted%20via-Paypal-blueviolet.svg)](https://www.paypal.me/alryaz)

## PS-16-DZ-Dimmer - `ps16dz`

### Basic device configuration

_Complete example may be viewed inside [`example.yaml` file](https://github.com/alryaz/esphome-ps16dz/blob/main/example.yaml)._

```yaml
# Enable logging, but disable it over serial
# Since the device utilizes GPIO1 (TX) and GPIO3 (RX) pins for communication,
# this will interfere with its operation.
logger:
  baud_rate: 0

# Add repository with this component
external_components:
  - source:
      type: git
      url: https://github.com/alryaz/esphome-ps16dz
      ref: main
    components: [ ps16dz ]

# Time platform is mandatory due to synchronization specifics
time:
  - platform: homeassistant
  - platform: sntp

# UART must be set to 19200 baud rate (required by Nuvoton chip)
uart:
  baud_rate: 19200
  tx_pin: TX  # GPIO1 pin
  rx_pin: RX  # GPIO3 pin

light:
    # Create test light
  - platform: ps16dz
    name: "PS-16-DZ Dimmer"
    on_settings_enter:
      # Hook: on settings enter (3-5 second central button push)
      then:
        if:
          condition:
            light.is_off: status_led_1
          then:
            light.turn_on: status_led_1
    on_settings_exit:
      # Hook: on settings exit (3-5 second central button push)
      then:
        if:
          condition:
            light.is_on: status_led_1
          then:
            light.turn_off: status_led_1
    
    # Status LED (optional, located on GPIO13 for my test device)
  - platform: status_led
    name: "Status LED"
    id: status_led_1
    pin:
      number: GPIO13
      inverted: true
```
