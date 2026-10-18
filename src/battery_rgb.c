/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BATTERY_RGB_NODE DT_NODELABEL(battery_rgb)
#define LOW_BATTERY_THRESHOLD 20
#define UPDATE_PERIOD_MS 500

/* Battery level per peripheral (0-100). Updated by the battery-state event. */
static uint8_t battery_levels[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

static int battery_rgb_battery_cb(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (ev && ev->source < ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        battery_levels[ev->source] = ev->state_of_charge;
    }
    return 0;
}

ZMK_LISTENER(battery_rgb_battery, battery_rgb_battery_cb);
ZMK_SUBSCRIPTION(battery_rgb_battery, zmk_peripheral_battery_state_changed);

/* Linear HSV hue 0deg (red) .. 120deg (green); 50% -> 60deg (orange/yellow). */
static struct led_rgb color_for_level(uint8_t level) {
    uint16_t hue = (uint16_t)level * 120 / 100;
    struct led_rgb color = {0};

    if (hue < 60) {
        color.r = 255;
        color.g = (uint8_t)(255 * hue / 60);
        color.b = 0;
    } else {
        color.r = (uint8_t)(255 * (120 - hue) / 60);
        color.g = 255;
        color.b = 0;
    }

    return color;
}

static uint8_t min_battery_level(void) {
    uint8_t min = 100;
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        if (battery_levels[i] < min) {
            min = battery_levels[i];
        }
    }
    return min;
}

static void battery_rgb_thread(void *a, void *b, void *c) {
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    const struct device *led = DEVICE_DT_GET(BATTERY_RGB_NODE);
    if (!device_is_ready(led)) {
        LOG_ERR("battery_rgb device is not ready");
        return;
    }

    const struct led_rgb off = {0};
    bool led_on = true;

    while (true) {
        uint8_t battery = min_battery_level();
        struct led_rgb color = color_for_level(battery);

        if (battery < LOW_BATTERY_THRESHOLD) {
            led_on = !led_on;
        } else {
            led_on = true;
        }

        const struct led_rgb *out = led_on ? &color : &off;
        int err = led_strip_update_rgb(led, out, 1);
        if (err != 0) {
            LOG_ERR("led_strip_update_rgb failed: %d", err);
        }

        k_sleep(K_MSEC(UPDATE_PERIOD_MS));
    }
}

K_THREAD_DEFINE(battery_rgb_tid, 1024, battery_rgb_thread, NULL, NULL, NULL,
                K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
