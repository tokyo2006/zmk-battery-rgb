# zmk-battery-rgb

ZMK module that drives a single WS2812 LED to show the **minimum battery level**
of a split keyboard's peripherals (as seen from the central/dongle side).

- 0% → red
- 50% → orange/yellow
- 100% → green
- Below 20% → blinks

Linear HSV hue ramp (0° red → 120° green).

## Usage

Add to `config/west.yml`:

```yaml
- name: zmk-battery-rgb
  remote: tokyo2006
  revision: main
```

Enable in the central's `.conf`:

```conf
CONFIG_ZMK_BATTERY_RGB=y
```

Define the LED node in the central's overlay (or board overlay):

```dts
/ {
    battery_rgb: battery_rgb {
        compatible = "worldsemi,ws2812-gpio";
        chain-length = <1>;
        color-mapping = <LED_COLOR_ID_GREEN LED_COLOR_ID_RED LED_COLOR_ID_BLUE>;
        gpios = <&gpio0 11 GPIO_ACTIVE_HIGH>;
    };
};
```

Requires the central to fetch peripheral battery levels
(`CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=y`, auto-enabled by the
Prospector adapter).
