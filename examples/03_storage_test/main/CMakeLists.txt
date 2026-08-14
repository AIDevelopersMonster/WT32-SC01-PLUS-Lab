idf_component_register(SRCS "storage_test.c"
                       INCLUDE_DIRS "."
                       REQUIRES sdmmc esp_driver_sdspi esp_driver_spi esp_system)
