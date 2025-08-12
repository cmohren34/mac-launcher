#include "app_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ApplicationServices/ApplicationServices.h>

void launch_or_switch_app(const char* bundle_id) {
    if (!bundle_id || strlen(bundle_id) == 0) {
        fprintf(stderr, "invalid bundle ID\n");
        return;
    }
    
    // first check if app is already running
    if (is_app_running(bundle_id)) {
        // bring to front using applescript
        char script[512];
        snprintf(script, sizeof(script),
                "osascript -e 'tell application id \"%s\" to activate' 2>/dev/null",
                bundle_id);
        
        int result = system(script);
        if (result == 0) {
            printf("switched to: %s\n", bundle_id);
            return;
        }
    }
    
    // app is not running, need to launch
    CFStringRef bundle_ref = CFStringCreateWithCString(
        kCFAllocatorDefault,
        bundle_id,
        kCFStringEncodingUTF8
    );
    
    if (!bundle_ref) {
        fprintf(stderr, "failed to create CFString for bundle: %s\n", bundle_id);
        return;
    }
    
    // modern method: use LSCopyApplicationURLsForBundleIdentifier
    CFArrayRef urls = LSCopyApplicationURLsForBundleIdentifier(bundle_ref, NULL);
    
    if (urls && CFArrayGetCount(urls) > 0) {
        CFURLRef app_url = CFArrayGetValueAtIndex(urls, 0);
        
        // launch using ls open
        LSLaunchURLSpec launch_spec = {
            .appURL = app_url,
            .itemURLs = NULL,
            .passThruParams = NULL,
            .launchFlags = kLSLaunchDefaults,
            .asyncRefCon = NULL
        };
        
        OSStatus status = LSOpenFromURLSpec(&launch_spec, NULL);
        
        if (status == noErr) {
            printf("launched: %s\n", bundle_id);
        } else {
            fprintf(stderr, "LSOpenFromURLSpec failed: %d\n", (int)status);
        }
        
        CFRelease(urls);
    } else {
        // fallback: use system command
        char command[512];
        snprintf(command, sizeof(command), "open -b %s 2>/dev/null", bundle_id);
        
        int result = system(command);
        if (result == 0) {
            printf("launched via open: %s\n", bundle_id);
        } else {
            fprintf(stderr, "failed to launch app: %s\n", bundle_id);
        }
    }
    
    CFRelease(bundle_ref);
}

int is_app_running(const char* bundle_id) {
    CFStringRef bundle_ref = CFStringCreateWithCString(
        kCFAllocatorDefault,
        bundle_id,
        kCFStringEncodingUTF8
    );
    
    if (!bundle_ref) return 0;
    
    // use system command as simple solution for checking
    char command[512];
    snprintf(command, sizeof(command), 
             "pgrep -f %s > /dev/null 2>&1", bundle_id);
    
    int result = system(command);
    CFRelease(bundle_ref);
    
    return (result == 0);
}

void force_quit_app(const char* bundle_id) {
    if (!bundle_id) return;
    
    // use osascript to quit app
    char command[512];
    snprintf(command, sizeof(command),
             "osascript -e 'tell application id \"%s\" to quit' 2>/dev/null",
             bundle_id);
    
    int result = system(command);
    if (result == 0) {
        printf("quit: %s\n", bundle_id);
    } else {
        snprintf(command, sizeof(command),
                "pkill -f %s 2>/dev/null", bundle_id);
        system(command);
    }
}

const char* get_app_name(const char* bundle_id) {
    static char name[256];
    
    CFStringRef bundle_ref = CFStringCreateWithCString(
        kCFAllocatorDefault,
        bundle_id,
        kCFStringEncodingUTF8
    );
    
    if (!bundle_ref) {
        return bundle_id;
    }
    
    CFArrayRef urls = LSCopyApplicationURLsForBundleIdentifier(bundle_ref, NULL);
    
    if (urls && CFArrayGetCount(urls) > 0) {
        CFURLRef app_url = CFArrayGetValueAtIndex(urls, 0);
        
        // get app name from url
        CFStringRef app_name = CFURLCopyLastPathComponent(app_url);
        if (app_name) {
            CFStringGetCString(app_name, name, sizeof(name), kCFStringEncodingUTF8);
            
            // remove .app extension
            char* dot = strstr(name, ".app");
            if (dot) *dot = '\0';
            
            CFRelease(app_name);
        }
        CFRelease(urls);
    } else {
        // fallback
        const char* last_dot = strrchr(bundle_id, '.');
        strncpy(name, last_dot ? last_dot + 1 : bundle_id, sizeof(name) - 1);
    }
    
    CFRelease(bundle_ref);
    return name;
}

void activate_app_with_options(const char* bundle_id, int all_windows) {
    char script[1024];
    
    if (all_windows) {
        snprintf(script, sizeof(script),
                "osascript -e '"
                "tell application id \"%s\"\n"
                "  activate\n"
                "  reopen\n"
                "end tell' 2>/dev/null",
                bundle_id);
    } else {
        snprintf(script, sizeof(script),
                "osascript -e 'tell application id \"%s\" to activate' 2>/dev/null",
                bundle_id);
    }
    
    system(script);
}

void hide_app(const char* bundle_id) {
    char script[512];
    snprintf(script, sizeof(script),
            "osascript -e 'tell application \"System Events\" to "
            "set visible of process id \"%s\" to false' 2>/dev/null",
            bundle_id);
    system(script);
}

void minimize_app_windows(const char* bundle_id) {
    char script[512];
    snprintf(script, sizeof(script),
            "osascript -e 'tell application id \"%s\" to "
            "set miniaturized of every window to true' 2>/dev/null",
            bundle_id);
    system(script);
}