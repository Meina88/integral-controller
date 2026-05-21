#pragma once

#include <stdbool.h>

// =========================
// ESTADO
// =========================
typedef enum {
    WIFI_STATE_IDLE,         // sin credenciales configuradas
    WIFI_STATE_DISCONNECTED, // tiene credenciales pero desconectado manualmente
    WIFI_STATE_CONNECTING,   // tiene SSID, intentando conectar
    WIFI_STATE_CONNECTED,    // IP obtenida
    WIFI_STATE_AUTH_FAIL,    // password incorrecta (tras N intentos)
    WIFI_STATE_NO_AP,        // red no encontrada (tras N intentos)
    WIFI_STATE_ERROR,        // otro error (tras N intentos)
} wifi_state_t;

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
bool          wifi_is_connected(void);
wifi_state_t  wifi_get_state(void);

const char *wifi_get_ip_string(void);
const char *wifi_get_ssid(void);

// =========================
// SCAN
// =========================
void wifi_start_scan(void);
int  wifi_get_scan_count(void);
const char *wifi_get_scan_ssid(int index);

// Mantiene compatibilidad con código existente
const char *wifi_get_last_error(void);
void        wifi_clear_last_error(void);
