#pragma once

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.1.0"
#endif
#define DEVICE_ID_PREFIX "familyhub-panel"

// Polling interval when connected (ms)
#define POLL_INTERVAL_MS 5000
#define POLL_INTERVAL_FAST_MS 3000

// Backoff limits (ms)
#define BACKOFF_INITIAL_MS 1000
#define BACKOFF_MAX_MS 30000

// Cache file on SPIFFS
#define CACHE_PATH "/dashboard_cache.json"

// NVS keys
#define NVS_NAMESPACE "familyhub"
#define NVS_KEY_SERVER_HOST "srv_host"
#define NVS_KEY_SERVER_PORT "srv_port"
#define NVS_KEY_WRITE_TOKEN "write_tok"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"
#define NVS_KEY_CHILD_MODE "child_mode"
#define NVS_KEY_CHILD_ID "child_id"
#define NVS_KEY_CHILD_PAGE "child_page"
#define NVS_KEY_CHILD_SYNC "child_sync"
#define NVS_KEY_SKILL_CELEBRATION "skill_celebrate"
#define CHILD_CACHE_PATH "/child_cache.json"

// Wi-Fi reconnect interval when disconnected (ms)
#define WIFI_RECONNECT_INTERVAL_MS 10000
