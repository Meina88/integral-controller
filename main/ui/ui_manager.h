#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ui_start(void);
void ui_update(void);
void ui_set_active_profile(const char *code);
void ui_rebuild(void);

#ifdef __cplusplus
}
#endif