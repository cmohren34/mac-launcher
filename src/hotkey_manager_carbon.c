#include "hotkey_manager.h"
#include "app_controller.h"
#include "usage_tracker.h"
#include <stdio.h>

// globals for callback access
static AppMapping* g_mappings = NULL;
static int g_count = 0;
static EventHandlerUPP g_handler_upp = NULL;
static EventHotKeyRef* g_hotkey_refs = NULL;
static EventHandlerRef g_handler_ref = NULL;

OSStatus hotkey_handler(EventHandlerCallRef handler, EventRef event, void* userdata) {
    printf("DEBUG: Hotkey event received!\n");  // ADD THIS
    
    EventHotKeyID key_id;
    OSStatus status = GetEventParameter(
        event,
        kEventParamDirectObject,
        typeEventHotKeyID,
        NULL,
        sizeof(EventHotKeyID),
        NULL,
        &key_id
    );
    
    if (status != noErr) {
        printf("DEBUG: Failed to get event parameter: %d\n", (int)status);
        return status;
    }
    
    // key_id.id is our array index
    UInt32 index = key_id.id;
    printf("DEBUG: Hotkey index: %u\n", index);  // ADD THIS
    
    if (index < g_count && g_mappings) {
        printf("Launching: %s\n", g_mappings[index].display_name);
        
        // launch the app
        launch_or_switch_app(g_mappings[index].bundle_id);
        
        // track usage
        track_launch(g_mappings[index].bundle_id);
    }
    
    return noErr;
}

void init_hotkey_manager(AppMapping* mappings, int count) {
    if (!mappings || count == 0) {
        fprintf(stderr, "No mappings to register\n");
        return;
    }
    
    // store references
    g_mappings = mappings;
    g_count = count;
    
    // allocate hotkey refs array
    g_hotkey_refs = calloc(count, sizeof(EventHotKeyRef));
    
    // install event handler - try different approach
    EventTypeSpec event_specs[2];
    event_specs[0].eventClass = kEventClassKeyboard;
    event_specs[0].eventKind = kEventHotKeyPressed;
    event_specs[1].eventClass = kEventClassKeyboard;
    event_specs[1].eventKind = kEventHotKeyReleased;
    
    g_handler_upp = NewEventHandlerUPP(hotkey_handler);
    
    // install on event dispatcher target (more reliable)
    OSStatus status = InstallEventHandler(
        GetEventDispatcherTarget(),  // CHANGED FROM GetApplicationEventTarget()
        g_handler_upp,
        2,  // handle both press and release
        event_specs,
        NULL,
        &g_handler_ref  // STORE THE REF
    );
    
    if (status != noErr) {
        fprintf(stderr, "Failed to install event handler: %d\n", (int)status);
        
        // try fallback
        status = InstallApplicationEventHandler(
            g_handler_upp,
            2,
            event_specs,
            NULL,
            &g_handler_ref
        );
        
        if (status != noErr) {
            fprintf(stderr, "Fallback also failed: %d\n", (int)status);
            return;
        }
    }
    
    printf("DEBUG: Event handler installed successfully\n");
    
    // register each hotkey with debug output
    for (int i = 0; i < count; i++) {
        EventHotKeyID key_id;
        key_id.signature = 'LNCH';
        key_id.id = i;
        
        // make sure we're using the right modifier
        UInt32 modifiers = cmdKey | optionKey;  // TRY CMD+OPTION INSTEAD
        
        status = RegisterEventHotKey(
            mappings[i].keycode,
            modifiers,  // CHANGED
            key_id,
            GetEventDispatcherTarget(),  // CHANGED
            0,
            &g_hotkey_refs[i]
        );
        
        if (status != noErr) {
            fprintf(stderr, "Failed to register hotkey for %s (keycode: %u, status: %d)\n", 
                    mappings[i].display_name, mappings[i].keycode, (int)status);
            
            // try with just option key
            status = RegisterEventHotKey(
                mappings[i].keycode,
                optionKey,
                key_id,
                GetEventDispatcherTarget(),
                0,
                &g_hotkey_refs[i]
            );
            
            if (status == noErr) {
                printf("Registered with Option only: Option+%d -> %s\n", 
                       (i < 9 ? i + 1 : 0), mappings[i].display_name);
            }
        } else {
            printf("Registered: Cmd+Option+%d -> %s (keycode: %u)\n", 
                   (i < 9 ? i + 1 : 0), mappings[i].display_name, mappings[i].keycode);
        }
    }
}

void cleanup_hotkey_manager(void) {
    // unregister hotkeys
    if (g_hotkey_refs) {
        for (int i = 0; i < g_count; i++) {
            if (g_hotkey_refs[i]) {
                UnregisterEventHotKey(g_hotkey_refs[i]);
            }
        }
        free(g_hotkey_refs);
        g_hotkey_refs = NULL;
    }
    
    // remove event handler
    if (g_handler_ref) {
        RemoveEventHandler(g_handler_ref);
        g_handler_ref = NULL;
    }
    
    // dispose handler
    if (g_handler_upp) {
        DisposeEventHandlerUPP(g_handler_upp);
        g_handler_upp = NULL;
    }
    
    g_mappings = NULL;
    g_count = 0;
}