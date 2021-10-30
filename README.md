_PS-16-DZ Dimmer_ for _ESPHome_
==================================================

> Custom component for ESPHome that offers support for PS-16-DZ dimmer devices.
>
> [![License](https://img.shields.io/badge/%D0%9B%D0%B8%D1%86%D0%B5%D0%BD%D0%B7%D0%B8%D1%8F-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
> [![Maintained](https://img.shields.io/badge/%D0%9F%D0%BE%D0%B4%D0%B4%D0%B5%D1%80%D0%B6%D0%B8%D0%B2%D0%B0%D0%B5%D1%82%D1%81%D1%8F%3F-%D0%B4%D0%B0-green.svg)](https://github.com/alryaz/hass-lkcomu-interrao/graphs/commit-activity)
>
> [![Donations accepted via](https://img.shields.io/badge/%D0%9F%D0%BE%D0%B6%D0%B5%D1%80%D1%82%D0%B2%D0%BE%D0%B2%D0%B0%D0%BD%D0%B8%D0%B5-Yandex-red.svg)](https://money.yandex.ru/to/410012369233217)
> [![Donations accepted via](https://img.shields.io/badge/%D0%9F%D0%BE%D0%B6%D0%B5%D1%80%D1%82%D0%B2%D0%BE%D0%B2%D0%B0%D0%BD%D0%B8%D0%B5-Paypal-blueviolet.svg)](https://www.paypal.me/alryaz)

## Basic device configuration

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
