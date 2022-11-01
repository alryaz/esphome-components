Sherlock S2 Smart Lock
===


## Example configuration
```yaml
substitutions:
  friendly_name: Front Door Lock
  board_name: front-door-lock

esphome:
  platform: ESP8266
  board: esp01_1m
  on_boot:
    then:
      lock.template.publish:
        id: lock_switch
        state: !lambda 'return LOCK_STATE_LOCKED;'

ota:

api:

external_components:
  - source:
      type: git
      url: https://github.com/alryaz/esphome-components
      ref: main
    components: [ sherlock_s2 ]
    refresh: 1s

uart:
  - id: uart_bus
    tx_pin: 1
    rx_pin: 3
    baud_rate: 115200
    
globals:
  - id: lock_is_active
    type: "bool"
    initial_value: "false"
    restore_value: false
    
sensor:
  - platform: sherlock_s2
    id: sherlock_s2_sensor
    battery_voltage:
      name: "${friendly_name} Battery Voltage"
    battery_level:
      name: "${friendly_name} Battery Level"
    on_action_start:
      then:
        - logger.log: "Lock is active"
        - globals.set:
            id: lock_is_active
            value: "true"
        - if:
            condition:
              lock.is_locked: lock_switch
            then:
              lock.template.publish:
                id: lock_switch
                state: !lambda 'return LOCK_STATE_UNLOCKING;'
            else:
              lock.template.publish:
                id: lock_switch
                state: !lambda 'return LOCK_STATE_LOCKING;'
    on_unlock:
      then:
        - globals.set:
            id: lock_is_active
            value: "false"
        - if:
            condition:
              - lock.is_locked: lock_switch
              - lambda: "return duration < 1;"
            then:
              - logger.log:
                  format: "Lock failed to unlock! (duration: %f seconds)"
                  args: [ "duration" ]
                  level: warn
              - lock.template.publish:
                  id: lock_switch
                  state: !lambda 'return LOCK_STATE_JAMMED;'
            else:
              - logger.log:
                  format: "Lock unlock in %f seconds"
                  args: [ "duration" ]
              - lock.template.publish:
                  id: lock_switch
                  state: !lambda 'return LOCK_STATE_UNLOCKED;'
    on_lock:
      then:
        - globals.set:
            id: lock_is_active
            value: "false"
        - if:
            condition:
              - lock.is_unlocked: lock_switch
              - lambda: "return duration < 1;"
            then:
              - logger.log:
                  format: "Lock failed to lock! (duration: %f seconds)"
                  args: [ "duration" ]
                  level: warn
              - lock.template.publish:
                  id: lock_switch
                  state: !lambda 'return LOCK_STATE_JAMMED;'
            else:
              - logger.log:
                  format: "Lock locked in %f seconds"
                  args: [ "duration" ]
              - lock.template.publish:
                  id: lock_switch
                  state: !lambda 'return LOCK_STATE_LOCKED;'
                

switch:
  # Lock driver control
  - id: lock_driver_output
    platform: gpio
    internal: true
    restore_mode: always off
    pin:
      number: GPIO2
      inverted: yes

lock:
  # Publishing switch
  - id: lock_switch
    name: ${friendly_name}
    platform: template
    lock_action:
      if:
        condition:
          lambda: "return !id(lock_is_active);"
        then:
          - switch.turn_on: lock_driver_output
          - delay: 15ms
          - switch.turn_off: lock_driver_output
        else:
          logger.log:
            format: "Lock is active!"
            level: warn
    unlock_action:
      if:
        condition:
          lambda: "return !id(lock_is_active);"
        then:
          - switch.turn_on: lock_driver_output
          - delay: 50ms
          - switch.turn_off: lock_driver_output
        else:
          logger.log:
            format: "Lock is active!"
            level: warn

binary_sensor:
    # Faceplate swipe button
  - id: lock_sensor_button
    platform: gpio
    internal: true
    pin: GPIO0
    filters:
      - invert
    on_click:
      - min_length: 5ms
        max_length: 34ms
        then:
          lock.lock: lock_switch
      - min_length: 35ms
        max_length: 60ms
        then:
          lock.unlock: lock_switch
```
