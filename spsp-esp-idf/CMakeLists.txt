idf_component_register(
    SRC_DIRS "../common/src" "src"
    INCLUDE_DIRS "../common/include" "include"
    PRIV_INCLUDE_DIRS "../common/private_include" "private_include"
    REQUIRES cxx esp_event esp_netif esp_wifi freertos mqtt nvs_flash
)
