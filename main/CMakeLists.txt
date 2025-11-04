idf_build_get_property(target IDF_TARGET)

# Required components for all targets
set(priv_requires 
    nvs_flash 
    esp_event 
    esp_netif 
    lwip 
    esp_http_server 
    esp_wifi
    esp_eth
    esp_timer
    log
)

# Add target-specific requirements if needed
if(${target} STREQUAL "linux")
    # Linux-specific components can be added here if needed
endif()

# Automatically find all .c files
file(GLOB SOURCES "*.c" "src/*.c")

idf_component_register(
    SRCS ${SOURCES}
    PRIV_REQUIRES ${priv_requires}
    INCLUDE_DIRS "." "include")
