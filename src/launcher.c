#include "launcher.h"
#include "dock_reader.h"
#include "hotkey_manager.h"
#include "app_controller.h"
#include "usage_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <CoreFoundation/CoreFoundation.h>

// global for signal handling
static CFRunLoopRef g_run_loop = NULL;

// signal handler for clean shutdown
void handle_signal(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    if (g_run_loop) {
        CFRunLoopStop(g_run_loop);
    }
}

int main(int argc, char* argv[]) {
    AppMapping mappings[MAX_APPS];
    int count = 0;
    
    // parse cl arguments
    int daemon_mode = 0;
    LauncherMode mode = MODE_DOCK;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            daemon_mode = 1;
        }
        if (strcmp(argv[i], "-m") == 0 && i+1 < argc) {
            if (strcmp(argv[i+1], "switcher") == 0) mode = MODE_SWITCHER;
            if (strcmp(argv[i+1], "custom") == 0) mode = MODE_CUSTOM;
            i++; // skip next arg
        }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("  -d           Run as daemon\n");
            printf("  -m <mode>    Set mode (dock, switcher, custom)\n");
            printf("  -h, --help   Show this help\n");
            return 0;
        }
    }
    
    // daemonize if needed
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        if (pid > 0) {
            exit(EXIT_SUCCESS);
        }
        
        // child continues as daemon
        if (setsid() < 0) {
            perror("setsid failed");
            exit(EXIT_FAILURE);
        }
        
        // close standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }
    
    // load configuration based on mode
    printf("Loading configuration (mode: %d)...\n", mode);
    
    switch (mode) {
        case MODE_DOCK:
            count = read_dock_apps(mappings, MAX_APPS);
            break;
        case MODE_SWITCHER:
            count = get_running_apps(mappings, MAX_APPS);
            break;
        case MODE_CUSTOM:
            count = load_custom_config(mappings, MAX_APPS);
            break;
    }
    
    if (count == 0) {
        fprintf(stderr, "No apps loaded. Exiting.\n");
        return 1;
    }
    
    printf("Loaded %d apps\n", count);
    
    // initialize subsystems
    init_usage_tracker();
    init_hotkey_manager(mappings, count);
    
    signal(SIGINT, handle_signal);  // Ctrl+C
    signal(SIGTERM, handle_signal); // kill command
    
    printf("Launcher running. Press Ctrl+C to quit.\n");
    printf("Registered hotkeys:\n");
    for (int i = 0; i < count; i++) {
        printf("  Option+%d -> %s\n", 
               (i < 9 ? i + 1 : 0), 
               mappings[i].display_name);
    }
    
    // run event loop - listen for hotkeys
    g_run_loop = CFRunLoopGetCurrent();
    CFRunLoopRun();
    
    // mem clean
    printf("Cleaning up...\n");
    cleanup_hotkey_manager();
    cleanup_usage_tracker();
    
    printf("Launcher terminated.\n");
    return 0;
}