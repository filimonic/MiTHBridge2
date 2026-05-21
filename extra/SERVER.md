# Server partial config

## Telegraf: 

```
# telegraf.conf
[agent]
    interval = "10s"
    round_interval = true
    metric_batch_size = 1000
    metric_buffer_limit = 10000
    flush_interval = "10s"
    precision = "0s"
    omit_hostname = true
[[inputs.http_listener_v2]]
    service_address = ":8080"
    paths = ["/"]
    data_format = "influx"
[[outputs.prometheus_client]]
    listen = ":9273"
    path = "/metrics"
    expiration_interval = "120s"
    export_timestamp = true
```

## SWS \ Mapping

Extra data may be imported to Prometheus to make asociations. Example files:. Files need to be placed somewhere: 

```
sensor_metadata.prom.txt:
    content: |
    sensor_metadata{id="A4:C1:38:11:11:11", location="pluto", model="ZG-227Z", battery_type="CR2450", label="01", placement="Outdoow, Window 1"} 1
    sensor_metadata{id="A4:C1:38:22:22:22", location="pluto", model="ZG-227Z", battery_type="CR2450", label="02", placement="Outdoow, Window 2"} 1
bluetooth_le_metadata.prom.txt:
    content: |
    bluetooth_le_phy_name{kind="primary", id=1, name="1M"} 1
    bluetooth_le_phy_name{kind="primary", id=3, name="CODED"} 1
    bluetooth_le_phy_name{kind="secondary", id=1, name="1M"} 1
    bluetooth_le_phy_name{kind="secondary", id=2, name="2M"} 1
    bluetooth_le_phy_name{kind="secondary", id=3, name="CODED"} 1
```