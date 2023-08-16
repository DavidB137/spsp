idf_component_register(
    SRC_DIRS "common/src" "espidf/src"
    INCLUDE_DIRS "common/include" "espidf/include"
    PRIV_INCLUDE_DIRS "common/private_include" "espidf/private_include"
    REQUIRES cxx esp_event esp_netif esp_wifi freertos mqtt nvs_flash
)
