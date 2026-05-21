FROM espressif/idf:v6.0.1 AS builder
RUN echo "source /opt/esp/idf/export.sh" >> /root/.bashrc
COPY main nvs partitions.csv sdkconfig.defaults sdkconfig.defaults.* CMakeLists.txt build.esp*.*.sh ./ 

