#ifndef DOCK_READER_H
#define DOCK_READER_H

#include "launcher.h"

int read_dock_apps(AppMapping* mappings, int max_count);

int get_running_apps(AppMapping* mappings, int max_count);

int load_custom_config(AppMapping* mappings, int max_count);

#endif