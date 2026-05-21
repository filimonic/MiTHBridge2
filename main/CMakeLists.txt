set(
    mithbridge2_priv_requires
    console
    nvs_flash
    esp_http_client 
    esp_netif
    esp_wifi
    esp_timer
    esp_driver_tsens
    esp_driver_gpio
    esp_driver_usb_serial_jtag
    lwip
    mbedtls
    bt
    console
)

set(
    mithbridge2_srcs
    main.c
    sensor.c
    sensor_ble.c
    sensor_sender_influx_line_https.c
    configurator.c
    sensor_esp32.c
    led_flasher.c
    configurator_console.c
)

idf_component_register(
                    SRCS "${mithbridge2_srcs}"
                    PRIV_REQUIRES "${mithbridge2_priv_requires}"
                    INCLUDE_DIRS ".")

