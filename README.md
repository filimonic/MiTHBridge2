# MiTHBridge2

**MiTHBridge2** is a tiny device that listens for [pvvx/ATC_MiThermometer](https://github.com/pvvx/ATC_MiThermometer) BLE advertisements in **pvvx (custom)** and **atc1441** format and sends data over Wi-Fi as [InfluxDB Line protocol](https://docs.influxdata.com/influxdb3/core/reference/line-protocol/) over HTTP\[S\], optionally with authentication.

[InfluxDB Line protocol](https://docs.influxdata.com/influxdb3/core/reference/line-protocol/) can be perfectly accepted by [Telegraf](https://www.influxdata.com/time-series-platform/telegraf/) and then exported to [Prometheus](https://prometheus.io/)'s scraper.

Successor of [MiTHBridge1](https://github.com/filimonic/MiTHBridge).

## Differences from MiTHBridge1

- No host software required to run. Device listens and posts data to server itself (Wi-Fi needed)
- No more CollectD format. Now it is [InfluxDB Line protocol](https://docs.influxdata.com/influxdb3/core/reference/line-protocol/) to help sending over internet
- Added configuration for Wi-Fi and Endpoint. It is made using CLI\SerialMonitor\Any serial communication software.
- Added **atc1441** advertisment format support (experimental)
- Sensor IDs are now MAC-addresses (in correct byute order)
- Data from multiple sensors send in bulk each 45 seconds, not as soon as data was received (sent withdout timestamps, but it is still good enough)

## Hardware

Project is oriented to be used with 

- [XIAO ESP32C6](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C6-p-5884.html), enabling external (built-in) XIAO Amplifier.
- Mandatory external antenna, connected to IPEX connector. You can buy U.FL/UFL/IPX/IPEX to SMA or RP-SMA pigtail adapter and SMA or RP-SMA Wi-Fi 2.4 GHz antenna. [See details](./extra/esp32c6.xiao_external-antenna/ANTENNA.md)

## Setup

Configuration is made using CLI only. Here I will use [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/). 

* **Flash**:
  * Get correct firmware bin at Releases.
    * `mithbridge2-full.*.bin` is full flash image and should be flashed to `0x0` flash offset (`bootloader`).
      * This will erase all settings
      * When flashing for first time, you need this
    * `mithbridge2-app.*.bin` is app-only image and should be flashed to `0x10000` flash offset ( `factory` )
      * This will not erase settings
      * When upgrading app, you need this to keep your settings (see upgrade notes)
  * Connect your XIAO ESP32C6 using USB to PC.
  * Flash firmware file: [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/) => Connect -> Flash Tools -> Flash Firmware file (.bin) at `0x0` Flash offset (Bootloader), erasing entire flash while flashing.
* **Configure**:
  * Start serial monitor: [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/) => Connect -> Serial monitor -> Start
  * Send any command (literally: any, `(space)` is OK) while console writes prompt `=== Hit ENTER to configure. ** seconds left ===`.  If you missed, pres **Reset** button in web UI to restart device.SD
  * You will see `config >` prompt. Send `help` command. 
    * `MiTHBridge2` supports 4 commands:
        * `help`: print help
        * `restart`: restart device
        * `get <param>`: read value of `param` stored in NVS
        * `set <param> <value>`: replace value of `param` in NVS
          * `<value>` may be specified inside double quotes: `"value"`, `"value with space"`
    * `<param>`s supported:
        * `W_SSID`: Wi-Fi name (SSID)
        * `W_PSK`: Wi-Fi password (Pre-Shared Key)
        * `ILH_URL`: HTTP or HTTPS URL of InfluxDB Line Protocol
        * `ILH_AUTH`: Information that willbe sent as `Authorization` header. For `Basic` authorization you may use [DebugBear's Basic Auth Header Generator](https://www.debugbear.com/basic-auth-header-generator)
* Configure your MiTHBridge2 by sending commands:
   * `set W_SSID "My-WiFi-Name"` : Set Wi-Fi name to this. Note Wi-Fi names are case sensitive.
   * `set W_PSK "my-wifi-assword"` : Set Wi-Fi password to this. Note Wi-Fi passwords are case sensitive.
   * `set ILH_URL "https://example.com/telegraf/post/influx-line"`: Set InfluxDB Line Handler URL
   * `set ILH_AUTH "Basic eW91cl91c2VybmFtZTp5b3VyX3Bhc3N3b3Jk"`: Set InfluxDB Line Handler URL authorization to this type and creds
   * `restart`: restart device
* Check device is sending data to your endpoint:
    * Your device should connect Wi-Fi and obtain IP from DHCP:
      * `I (24798) esp_netif_handlers: sta ip: 192.168.0.92, mask: 255.255.255.192, gw: 192.168.0.126`
    * Your device should send data successfully:
      * `I (116108) sensor_sender_influx_line_https_send: write successful. Status: 204`
    * You should see data in your system

## Data coillecting and sending example:

Data from sensors is collected all the time. Based on sensor `id` (MAC), it is deduplicated (for each sensor there is only last value stored).
Data is not timestamped by MiTHBridge: timestamps are created on receiver (server) side at the moment when data is received by server.

Data is transmitted over HTTP or HTTPS in chunked flow (`Transfer-Encoding: chunked`) every 45 seconds.
Each chunk is all sensor data from specific sensor device.

For HTTPS, certificates are checked against internal IDF built-in CA list.

After each send, stored values are cleared from memory.

Data example:
```
### Tags:
#
# sender_id: factory MAC of your MiTHBridge2 device. Can be 6 or 8 2-hex pairs (currently 6, but in future can be 8)
# id: MAC address of sensor, as specified in BLE ADV package. Can be 6 or 8 2-hex pairs (currently 6, but in future can be 8)

### Merics:
# All metrics are optional. There MAY be no metrics for some devices.
# temperature:   sensor temperature
# humidity:      sensor humidity
# voltage:       sensor voltage (battery voltage)
# battery_level: sensor battery level
# rssi:          signal level (Received Signal Strength Indicator) in dbM. higher = better.
#
# uptime:        uptime in seconds (since boot), for MiTHBridge2 device
#
# ble_primary_phy:   primary PHY.   1 = 1M, 2 = 2M, 3 = CODED
# ble_secondary_phy: secondary PHY. 1 = 1M, 2 = 2M, 3 = CODED
 
sensor,sender_id=99:99:99:99:99:99,id=A4:C1:38:11:11:11 temperature=23.59,humidity=59.59,voltage=3.048,battery_level=100i,rssi=-82i,ble_primary_phy=3i,ble_secondary_phy=3i
sensor,sender_id=99:99:99:99:99:99,id=A4:C1:38:22:22:22 temperature=22.29,humidity=63.40,voltage=2.581,battery_level=47i,rssi=-94i,ble_primary_phy=1i
# For device itself, id = sender_id
sensor,sender_id=99:99:99:12:34:56,id=99:99:99:12:34:56 temperature=46.90,uptime=108i

```

## Casing

3D Printed casing variant is located in [housing](./housing) folder.

## LED Indicator

LED Indicator flashes twice each valid (data from sensor) is received.

## Stat server config examples:

I'm using Telegraf + Prometheus + Nginx for my own

```

```