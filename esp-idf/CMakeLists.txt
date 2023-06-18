idf_component_register(
    SRC_DIRS "../src" "src"
    INCLUDE_DIRS "../include" "include"
    PRIV_INCLUDE_DIRS "../private_include" "private_include"
    REQUIRES cxx esp_event esp_netif esp_wifi mbedtls mqtt nvs_flash
)
