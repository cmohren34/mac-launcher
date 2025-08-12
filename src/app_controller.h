#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

void launch_or_switch_app(const char* bundle_id);

int is_app_running(const char* bundle_id);

void force_quit_app(const char* bundle_id);

const char* get_app_name(const char* bundle_id);

void activate_app_with_options(const char* bundle_id, int all_windows);

void hide_app(const char* bundle_id);

void minimize_app_windows(const char* bundle_id);

#endif