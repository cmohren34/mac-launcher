#include "hotkey_manager.h"
#include "app_controller.h"
#include "usage_tracker.h"
#include <stdio.h>
#include <ApplicationServices/ApplicationServices.h>

static AppMapping* g_mappings = NULL;
static int g_count = 0;
static CFMachPortRef g_event_tap = NULL;
static CFRunLoopSourceRef g_run_loop_source = NULL;

// cgevent callback
CGEventRef event_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* userdata) {
    if (type != kCGEventKeyDown) {
        return event;
    }
    
    CGEventFlags flags = CGEventGetFlags(event);
    
    // check for option key (alt)
    if (!(flags & kCGEventFlagMaskAlternate)) {
        return event;
    }
    
    int64_t keycode = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    
    // check if this matches any of our mappings
    for (int i = 0; i < g_count; i++) {
        if (g_mappings[i].keycode == keycode) {
            printf("hotkey triggered: %s\n", g_mappings[i].display_name);
            
            // launch app
            launch_or_switch_app(g_mappings[i].bundle_id);
            track_launch(g_mappings[i].bundle_id);
            
            // consume the event
            return NULL;
        }
    }
    
    return event;
}

void init_hotkey_manager(AppMapping* mappings, int count) {
    if (!mappings || count == 0) {
        fprintf(stderr, "no mappings to register\n");
        return;
    }
    
    g_mappings = mappings;
    g_count = count;
    
    // create event tap
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown);
    
    g_event_tap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        event_callback,
        NULL
    );
    
    if (!g_event_tap) {
        fprintf(stderr, "failed to create event tap. make sure Accessibility is enabled.\n");
        return;
    }
    
    // create run loop source
    g_run_loop_source = CFMachPortCreateRunLoopSource(
        kCFAllocatorDefault, g_event_tap, 0);
    
    // add to current run loop
    CFRunLoopAddSource(CFRunLoopGetCurrent(), g_run_loop_source, 
                      kCFRunLoopCommonModes);
    
    // enable the tap
    CGEventTapEnable(g_event_tap, true);
    
    printf("Event tap installed. hotkeys ready.\n");
    
    for (int i = 0; i < count; i++) {
        printf("  Option+%d -> %s\n", 
               (i < 9 ? i + 1 : 0), mappings[i].display_name);
    }
}

void cleanup_hotkey_manager(void) {
    if (g_event_tap) {
        CGEventTapEnable(g_event_tap, false);
        CFRelease(g_event_tap);
        g_event_tap = NULL;
    }
    
    if (g_run_loop_source) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), g_run_loop_source, 
                             kCFRunLoopCommonModes);
        CFRelease(g_run_loop_source);
        g_run_loop_source = NULL;
    }
    
    g_mappings = NULL;
    g_count = 0;
}