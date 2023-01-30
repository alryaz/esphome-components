Hekr Serial Communication Protocol
===

## HIKING DDS238-4 W & DTS238-7 W Power Meters

- **Single phase (DDS238-4 W)**
  <details>
    <summary>Front Side Picture</summary>
    <img src="images/dds238-4-w.webp" alt="HIKING DDS238-4 W Image">
  </details>

   - Purchase Link 1: https://aliexpress.com/item/1005001496686330.html
   - Purchase Link 2: https://aliexpress.com/item/1005004221786681.html
   - Purchase Link 3: https://aliexpress.com/item/1005003783545141.html

- **Triple phase (DTS238-7 W)**
  <details>
    <summary>Front Side Picture</summary>
    <img src="images/dts238-7-w.jpg" alt="HIKING DTS238-7 W Image">
  </details>

   - Purchase Link 1: https://aliexpress.com/item/4001233900800.html
   - Purchase Link 2: https://www.alibaba.com/product-detail/Smart-Wifi-Meter-DTS238-7-W_62266426106.html

The following configuration is for a single-phase power meter:

```yaml
substitutions:
  board_name: root-energy-meter
  friendly_name: Root Energy Meter

packages:
  ## Only one of the following options must be enabled!
  power_meter: github://alryaz/esphome-components/components/hekr/packages/power_meter/single_phase.yaml@main
  #power_meter: github://alryaz/esphome-components/components/hekr/packages/power_meter/triple_phase.yaml@main

# ...
# Add your own ESPHome configuration somewhere around here
# ...
```