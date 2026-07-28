#pragma once

// Copy to secrets.h and fill in for your LAN (do not commit secrets.h)
#ifndef FAMILY_HUB_SECRETS_H
#define FAMILY_HUB_SECRETS_H

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// endeavor LAN address — no secrets, configurable via NVS at runtime
#define DEFAULT_SERVER_HOST "192.168.1.134"
#define DEFAULT_SERVER_PORT 3020

// Public browser URL shown on the App tab + QR (server public_app_url overrides).
#define DEFAULT_PUBLIC_APP_URL "https://butler.3d-design-labs.com"

// Optional: set WRITE_TOKEN via NVS (write_tok), Diagnostics → Settings, or
// build flag -DFAMILY_HUB_WRITE_TOKEN=\"...\" (home builds only).
// Leave empty for LAN trust mode (no x-family-hub-token header sent).
//
// Wi-Fi + server host/port/token are also editable on-panel:
// Diagnostics → Open Settings (NVS: wifi_ssid, wifi_pass, srv_host, srv_port, write_tok).
// NVS Wi-Fi credentials override WIFI_SSID / WIFI_PASSWORD when present.
//
// Public-prep builds (`pio run -e waveshare7b-public`) ignore compile-time
// WIFI_* / FAMILY_HUB_WRITE_TOKEN and require NVS (C5 hashed auth stays gated).

// Shared panel capability root used to derive the device-bound display token.
// It must match the server's PANEL_TOKEN. Leave empty only for an unconfigured
// development server; production display routes fail closed without it.
#define FAMILY_HUB_WRITE_TOKEN ""

#endif
