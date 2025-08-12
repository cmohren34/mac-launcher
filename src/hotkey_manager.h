#ifndef HOTKEY_MANAGER_H
#define HOTKEY_MANAGER_H

#include "launcher.h"

void init_hotkey_manager(AppMapping* mappings, int count);

void cleanup_hotkey_manager(void);

OSStatus hotkey_handler(EventHandlerCallRef handler, EventRef event, void* userdata);

#endif