## General information

The dimmer is a ESP8285 device interacting with a custom MCU over serial connection at 9600b/s.

The device is implemented with data gathered from [Tasmota information page on the device](https://templates.blakadder.com/sonoff_D1.html).

## Basic device configuration
_Complete example may be viewed inside [`example.yaml` file](https://github.com/alryaz/esphome-sonoff_d1/blob/main/components/sonoff_d1/example.yaml)._

```yaml
# Enable logging, but disable it over serial; otherwise,
# serial communication with the dimmer part of the
# assembly will be impossible.
logger:
  baud_rate: 0

# Add repository with this component
external_components:
  - source:
      type: git
      url: https://github.com/alryaz/esphome-components
      ref: main
    components: [ sonoff_d1 ]

# UART must be set to 19200 baud rate (required by Nuvoton chip)
uart:
  baud_rate: 9600
  tx_pin: TX  # GPIO1 pin
  rx_pin: RX  # GPIO3 pin

light:
    # Create test light
  - platform: sonoff_d1
    name: "Sonoff D1 Dimmer"
    
    # Status LED (optional, located on GPIO13 for my test device)
  - platform: status_led
    name: "Status LED"
    id: status_led_1
    pin:
      number: GPIO13
      inverted: true
```
