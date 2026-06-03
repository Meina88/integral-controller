#include "comms/wifi/wifi_manager.h"
#include <string.h>

static char s_ssid[64]     = "";
static char s_pass[64]     = "";

void wifi_init_sta(void) {}
void wifi_connect(const char *ssid, const char *password)
{
    if (ssid)     strncpy(s_ssid, ssid,     sizeof(s_ssid) - 1);
    if (password) strncpy(s_pass, password, sizeof(s_pass) - 1);
}
void wifi_disconnect(void) {}

bool         wifi_is_connected(void)     { return false; }
wifi_state_t wifi_get_state(void)        { return WIFI_STATE_IDLE; }
const char  *wifi_get_ip_string(void)    { return "---"; }
const char  *wifi_get_ssid(void)         { return s_ssid; }

void         wifi_start_scan(void)       {}
bool         wifi_is_scan_done(void)     { return true; }
int          wifi_get_scan_count(void)   { return 0; }
const char  *wifi_get_scan_ssid(int i)   { (void)i; return ""; }
const char  *wifi_get_last_error(void)   { return ""; }
void         wifi_clear_last_error(void) {}
