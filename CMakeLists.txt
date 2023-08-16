idf_component_register(
    SRC_DIRS "common/src" "esp-idf/src"
    INCLUDE_DIRS "common/include" "esp-idf/include"
    PRIV_INCLUDE_DIRS "common/private_include" "esp-idf/private_include"
    REQUIRES cxx esp_event esp_netif esp_wifi freertos mqtt nvs_flash
)
