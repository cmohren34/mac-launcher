#include "dock_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int parse_dock_array(CFArrayRef apps, AppMapping* mappings, int max) {
    int count = 0;
    CFIndex total = CFArrayGetCount(apps);
    
    for (CFIndex i = 0; i < total && count < max; i++) {
        CFDictionaryRef app = CFArrayGetValueAtIndex(apps, i);
        if (!app || CFGetTypeID(app) != CFDictionaryGetTypeID()) continue;
        
        // get tile-data
        CFDictionaryRef tile_data = CFDictionaryGetValue(app, CFSTR("tile-data"));
        if (!tile_data) continue;
        
        // get bundle id
        CFStringRef bundle = CFDictionaryGetValue(tile_data, CFSTR("bundle-identifier"));
        if (!bundle) continue;
        
        // get display name
        CFStringRef label = CFDictionaryGetValue(tile_data, CFSTR("file-label"));
        
        // convert to c strings
        Boolean success = CFStringGetCString(
            bundle,
            mappings[count].bundle_id,
            BUNDLE_ID_LENGTH,
            kCFStringEncodingUTF8
        );
        
        if (success) {
            // set display name
            if (label) {
                CFStringGetCString(
                    label,
                    mappings[count].display_name,
                    256,
                    kCFStringEncodingUTF8
                );
            } else {
                // fallback to bundle id
                strncpy(mappings[count].display_name, mappings[count].bundle_id, 255);
            }
            
            // assign hotkey (1-9, then 0)
            if (count < 9) {
                mappings[count].keycode = kVK_ANSI_1 + count;
            } else {
                mappings[count].keycode = kVK_ANSI_0;
            }
            mappings[count].modifiers = optionKey;
            
            count++;
        }
    }
    
    return count;
}

int read_dock_apps(AppMapping* mappings, int max_count) {
    char plist_path[512];
    snprintf(plist_path, sizeof(plist_path), 
             "%s/Library/Preferences/com.apple.dock.plist", getenv("HOME"));
    
    // create url for file
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        (const UInt8*)plist_path,
        strlen(plist_path),
        false
    );
    
    if (!url) {
        fprintf(stderr, "failed to create URL for dock plist\n");
        return 0;
    }
    
    // open stream
    CFReadStreamRef stream = CFReadStreamCreateWithFile(kCFAllocatorDefault, url);
    if (!stream) {
        CFRelease(url);
        return 0;
    }
    
    CFReadStreamOpen(stream);
    
    // read plist
    CFPropertyListRef plist = CFPropertyListCreateWithStream(
        kCFAllocatorDefault,
        stream,
        0,
        kCFPropertyListImmutable,
        NULL,
        NULL
    );
    
    int count = 0;
    if (plist && CFGetTypeID(plist) == CFDictionaryGetTypeID()) {
        CFArrayRef apps = CFDictionaryGetValue(plist, CFSTR("persistent-apps"));
        if (apps && CFGetTypeID(apps) == CFArrayGetTypeID()) {
            count = parse_dock_array(apps, mappings, max_count);
        }
        CFRelease(plist);
    }
    
    CFReadStreamClose(stream);
    CFRelease(stream);
    CFRelease(url);
    
    return count;
}

int get_running_apps(AppMapping* mappings, int max_count) {
    FILE* pipe = popen("osascript -e 'tell application \"System Events\" to get bundle identifier of every process whose background only is false' 2>/dev/null", "r");
    
    if (!pipe) {
        fprintf(stderr, "Failed to get running apps\n");
        return 0;
    }
    
    char buffer[4096];
    int count = 0;
    
    if (fgets(buffer, sizeof(buffer), pipe)) {
        // parse comma-separated bundle ids
        char* token = strtok(buffer, ", \n");
        while (token && count < max_count) {

            // clean up bundle id (remove quotes if present)
            char clean_id[256];
            sscanf(token, "%255s", clean_id);
            
            if (strlen(clean_id) > 0) {
                strncpy(mappings[count].bundle_id, clean_id, BUNDLE_ID_LENGTH - 1);
                strncpy(mappings[count].display_name, clean_id, 255);
                
                // assign keys
                if (count < 9) {
                    mappings[count].keycode = kVK_ANSI_1 + count;
                } else {
                    mappings[count].keycode = kVK_ANSI_0;
                }
                mappings[count].modifiers = optionKey;
                
                count++;
            }
            
            token = strtok(NULL, ", \n");
        }
    }
    
    pclose(pipe);
    return count;
}

int load_custom_config(AppMapping* mappings, int max_count) {
    char config_path[512];
    snprintf(config_path, sizeof(config_path), 
             "%s/.launcher/config.txt", getenv("HOME"));
    
    FILE* file = fopen(config_path, "r");
    if (!file) {
        
        // create default config
        char dir_path[512];
        snprintf(dir_path, sizeof(dir_path), "%s/.launcher", getenv("HOME"));
        mkdir(dir_path, 0755);
        
        file = fopen(config_path, "w");
        if (file) {
            fprintf(file, "# Launcher config file\n");
            fprintf(file, "# Format: bundle_id,display_name,key_number\n");
            fprintf(file, "com.apple.Terminal,Terminal,1\n");
            fprintf(file, "com.apple.Safari,Safari,2\n");
            fclose(file);
            fprintf(stderr, "Created default config at %s\n", config_path);
        }
        return 0;
    }
    
    int count = 0;
    char line[512];
    
    while (fgets(line, sizeof(line), file) && count < max_count) {
        // skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // format: bundle_id,display_name,key
        char* bundle = strtok(line, ",");
        char* name = strtok(NULL, ",");
        char* key_str = strtok(NULL, ",\n");
        
        if (bundle && key_str) {
            strncpy(mappings[count].bundle_id, bundle, BUNDLE_ID_LENGTH - 1);
            
            if (name && strlen(name) > 0) {
                strncpy(mappings[count].display_name, name, 255);
            } else {
                strncpy(mappings[count].display_name, bundle, 255);
            }
            
            int key = atoi(key_str);
            if (key == 0) {
                mappings[count].keycode = kVK_ANSI_0;
            } else if (key >= 1 && key <= 9) {
                mappings[count].keycode = kVK_ANSI_1 + (key - 1);
            } else {
                continue; // skip invalid keys
            }
            
            mappings[count].modifiers = optionKey;
            count++;
        }
    }
    
    fclose(file);
    return count;
}