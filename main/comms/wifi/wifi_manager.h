#pragma once

#include <stdbool.h>

// =========================
// INIT
// =========================
void wifi_init_sta(void);

// =========================
// CONTROL
// =========================
void wifi_connect(const char *ssid, const char *password);
void wifi_disconnect(void);

// =========================
// STATUS
// =========================
bool wifi_is_connected(void);

const char *wifi_get_ip_string(void);
const char *wifi_get_ssid(void);

// =========================
// SCAN
// =========================
void wifi_start_scan(void);

int wifi_get_scan_count(void);

const char *wifi_get_scan_ssid(int index);