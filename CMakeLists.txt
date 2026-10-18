if(CONFIG_ZMK_BATTERY_RGB)
  zephyr_library()
  zephyr_library_sources(src/battery_rgb.c)
endif()
