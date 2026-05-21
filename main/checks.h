#pragma once
#include "sdkconfig.h"

#ifndef CONFIG_BT_ENABLED
#error "Bluetooth must be enabled (CONFIG_BT_ENABLED)"
#endif

#ifndef CONFIG_BT_CONTROLLER_ENABLED
#error "Bluetooth Controller must be enabled (CONFIG_BT_CONTROLLER_ENABLED)"
#endif

#ifndef CONFIG_BT_NIMBLE_ENABLED
#error "NimBLE must be enabled (CONFIG_BT_NIMBLE_ENABLED)"
#endif

#ifdef CONFIG_BT_NIMBLE_ROLE_CENTRAL
#error "NimBLE Central Role is not supported and should be disabled (CONFIG_BT_NIMBLE_ROLE_CENTRAL)"
#endif

#ifdef CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#error "NimBLE Peripheral Role is not supported and should be disabled (CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)"
#endif

#ifdef CONFIG_BT_NIMBLE_ROLE_BROADCASTER
#error "NimBLE Broadcaster Role is not supported and should be disabled (CONFIG_BT_NIMBLE_ROLE_BROADCASTER)"
#endif

#ifndef CONFIG_BT_NIMBLE_ROLE_OBSERVER
#error "NimBLE Observer Role is should be enabled (CONFIG_BT_NIMBLE_ROLE_OBSERVER)"
#endif

#ifdef CONFIG_BT_NIMBLE_SECURITY_ENABLE
#error "NimBLE Security Mobule (SM) should be disabled (CONFIG_BT_NIMBLE_SECURITY_ENABLE)"
#endif

#ifdef CONFIG_BT_NIMBLE_HS_PVCY
#error "NimBLE privacy related API should be disabled (CONFIG_BT_NIMBLE_HS_PVCY)"
#endif

#ifndef CONFIG_BT_NIMBLE_50_FEATURE_SUPPORT
#error "BLE 5 Feature should be enabled (CONFIG_BT_NIMBLE_50_FEATURE_SUPPORT)"
#endif
#ifndef CONFIG_BT_NIMBLE_LL_CFG_FEAT_LE_2M_PHY
#error "BLE 5 2M PHY should be enabled (CONFIG_BT_NIMBLE_LL_CFG_FEAT_LE_2M_PHY)"
#endif
#ifndef CONFIG_BT_NIMBLE_LL_CFG_FEAT_LE_CODED_PHY
#error "BLE 5 Coded PHY should be enabled (CONFIG_BT_NIMBLE_LL_CFG_FEAT_LE_CODED_PHY)"
#endif
#ifdef CONFIG_BT_LE_SCAN_DUPL
#error "BLE Scan duplicates should be disabled (CONFIG_BT_LE_SCAN_DUPL)"
#endif

#ifndef CONFIG_BT_NIMBLE_EXT_ADV
#error "BLE Extended advertisments must be enabled (CONFIG_BT_NIMBLE_EXT_ADV)"
#endif

#ifdef CONFIG_BT_NIMBLE_ENABLE_PERIODIC_SYNC
#error "BLE Periodic sync should be disabled (CONFIG_BT_NIMBLE_ENABLE_PERIODIC_SYNC)"
#endif

#ifdef CONFIG_BT_NIMBLE_ENABLE_PERIODIC_ADV
#error "BLE Periodic adv should be disabled (CONFIG_BT_NIMBLE_ENABLE_PERIODIC_SYNC)"
#endif

#ifndef CONFIG_BT_NIMBLE_EXT_SCAN
#error "BLE Extended scan must be enabled (CONFIG_BT_NIMBLE_EXT_SCAN)"
#endif

#ifndef CONFIG_ESP_COEX_SW_COEXIST_ENABLE
#error "Wireless Coexistence must be enabled (CONFIG_ESP_COEX_SW_COEXIST_ENABLE)"
#endif

#ifdef CONFIG_ESP_COEX_EXTERNAL_COEXIST_ENABLE
#error "External Wireless Coexistence must be disabled (CONFIG_ESP_COEX_EXTERNAL_COEXIST_ENABLE)"
#endif

#ifndef CONFIG_LWIP_ENABLE
#error "LWIP must be enabled (CONFIG_LWIP_ENABLE)"
#endif

#ifdef CONFIG_LWIP_DNS_SUPPORT_MDNS_QUERIES
#error "LWIP mDNS should be disabled (CONFIG_LWIP_DNS_SUPPORT_MDNS_QUERIES)"
#endif

#ifdef CONFIG_LWIP_DHCPS
#error "LWIP DHCP Server should be disabled (CONFIG_LWIP_DHCPS)"
#endif

#ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
#error "SoftAP support should be disabled (CONFIG_ESP_WIFI_SOFTAP_SUPPORT)"
#endif

#ifndef CONFIG_ESP_WIFI_11R_SUPPORT
#error "Wi-Fi 802.11R support should be enabled (CONFIG_ESP_WIFI_11R_SUPPORT)"
#endif

#ifdef CONFIG_ESP_WIFI_NVS_ENABLED
#error "Wi-Fi NVS support should be disabled (CONFIG_ESP_WIFI_NVS_ENABLED)"
#endif

#ifdef CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT
#error "Wi-Fi Enterprise support should be disabled (CONFIG_ESP_WIFI_ENTERPRISE_SUPPORT)"
#endif

#if CONFIG_LWIP_SNTP_MAX_SERVERS < 2
#error "There must be at least 2 (preferrable 3) SNTP servers allowed in LWIP (CONFIG_LWIP_SNTP_MAX_SERVERS)"
#endif

#ifndef CONFIG_LWIP_DHCP_GET_NTP_SRV
#error "Allow SNTP servers from DHCP (CONFIG_LWIP_DHCP_GET_NTP_SRV)"
#endif

#ifndef CONFIG_LWIP_DHCP_MAX_NTP_SERVERS
#error "Set SNTP servers from DHCP to at least 1 (CONFIG_LWIP_DHCP_MAX_NTP_SERVERS)"
#endif

#if CONFIG_LWIP_DHCP_MAX_NTP_SERVERS < 1
#error "Set SNTP servers from DHCP to at least 1 (CONFIG_LWIP_DHCP_MAX_NTP_SERVERS)"
#endif

#if CONFIG_LWIP_SNTP_MAX_SERVERS - CONFIG_LWIP_DHCP_MAX_NTP_SERVERS < 1
#error "Allow place for predefined SNTP server (CONFIG_LWIP_SNTP_MAX_SERVERS > CONFIG_LWIP_DHCP_MAX_NTP_SERVERS)"
#endif

#ifndef CONFIG_ESP_HTTP_CLIENT_ENABLE_HTTPS
#error "Enable HTTPS support in ESP HTTPS client"
#endif

#ifndef CONFIG_ESP_HTTP_CLIENT_ENABLE_BASIC_AUTH
#error "Enable Basic Auth support in ESP HTTPS client"
#endif