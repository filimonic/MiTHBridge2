FROM espressif/idf:v6.0.1 AS builder
WORKDIR /app
RUN echo "source /opt/esp/idf/export.sh" >> /root/.bashrc

