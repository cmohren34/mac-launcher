#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <Carbon/Carbon.h> 
#include <CoreFoundation/CoreFoundation.h>

#define MAX_APPS 10
#define BUNDLE_ID_LENGTH 256
#define VERSION "1.0.0"

typedef struct {
    char bundle_id[BUNDLE_ID_LENGTH];
    char display_name[256];
    UInt32 keycode;
    UInt32 modifiers;
} AppMapping;

typedef enum {
    MODE_DOCK,
    MODE_SWITCHER,
    MODE_CUSTOM
} LauncherMode;

#endif