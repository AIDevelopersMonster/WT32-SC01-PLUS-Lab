idf_component_register(SRCS "lvgl_demo.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd esp_driver_gpio esp_driver_i2c esp_lvgl_port lvgl__lvgl)
