idf_component_register(
    SRC_DIRS "src/common" "src/espidf"
    INCLUDE_DIRS "include/common" "include/espidf"
    PRIV_INCLUDE_DIRS "priv_include/common" "priv_include/espidf"
    REQUIRES cxx esp_event esp_netif esp_wifi freertos mqtt nvs_flash
)
