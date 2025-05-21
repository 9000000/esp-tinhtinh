idf_component_register(SRCS "sound.c" "main.c" "ble_ancs.c" "bank.c" "sound.c"
                    INCLUDE_DIRS  ".")

spiffs_create_partition_image(spiffs ../spiffs FLASH_IN_PROJECT)
