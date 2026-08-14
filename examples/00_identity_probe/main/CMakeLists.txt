idf_component_register(SRCS "identity_probe.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_psram spi_flash esp_system esp_hw_support)
