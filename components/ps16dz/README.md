## General nformation

The dimmer is a ESP8285 device interacting with Nuvoton N76E003 over serial connection at 19200b/s.

The device is implemented with data gathered from [Tasmota information page on the device](https://tasmota.github.io/docs/devices/PS-16-DZ-Dimmer/).

## Basic device configuration
_Complete example may be viewed inside [`example.yaml` file](https://github.com/alryaz/esphome-ps16dz/blob/main/components/ps16dz/example.yaml)._

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
      url: https://github.com/alryaz/esphome-components
      ref: main
    components: [ ps16dz ]

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
