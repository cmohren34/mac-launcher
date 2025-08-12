#include "usage_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

static FILE* g_usage_file = NULL;
static char g_usage_path[512];

void init_usage_tracker(void) {
    // create .launcher directory if needed
    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/.launcher", getenv("HOME"));
    mkdir(dir_path, 0755); // create if doesn't exist
    
    // open usage file
    snprintf(g_usage_path, sizeof(g_usage_path), "%s/.launcher/usage.csv", getenv("HOME"));
    
    // open in append mode
    g_usage_file = fopen(g_usage_path, "a");
    
    if (g_usage_file) {
        // write header if file is empty
        fseek(g_usage_file, 0, SEEK_END);
        long size = ftell(g_usage_file);
        
        if (size == 0) {
            fprintf(g_usage_file, "timestamp,bundle_id,date,time\n");
        }
    } else {
        fprintf(stderr, "Failed to open usage file: %s\n", g_usage_path);
    }
}

void track_launch(const char* bundle_id) {
    if (!g_usage_file || !bundle_id) return;
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    // write timestamp, bundle_id, date, time
    fprintf(g_usage_file, "%ld,%s,%04d-%02d-%02d,%02d:%02d:%02d\n",
            now,
            bundle_id,
            tm_info->tm_year + 1900,
            tm_info->tm_mon + 1,
            tm_info->tm_mday,
            tm_info->tm_hour,
            tm_info->tm_min,
            tm_info->tm_sec);
    
    fflush(g_usage_file);
}

void get_usage_stats(int days) {
    if (!g_usage_path[0]) return;
    
    // count launches per app
    FILE* file = fopen(g_usage_path, "r");
    if (!file) {
        printf("No usage data available\n");
        return;
    }
    
    // calculate cutoff time
    time_t cutoff = time(NULL) - (days * 24 * 60 * 60);
    
    // count launches per app
    typedef struct {
        char bundle_id[256];
        int count;
    } AppCount;
    
    AppCount counts[100];
    int app_count = 0;
    
    char line[512];
    fgets(line, sizeof(line), file); // skip header
    
    while (fgets(line, sizeof(line), file)) {
        long timestamp;
        char bundle_id[256];
        
        if (sscanf(line, "%ld,%255[^,]", &timestamp, bundle_id) == 2) {
            if (timestamp < cutoff) continue;
            
            // find or add app
            int found = 0;
            for (int i = 0; i < app_count; i++) {
                if (strcmp(counts[i].bundle_id, bundle_id) == 0) {
                    counts[i].count++;
                    found = 1;
                    break;
                }
            }
            
            if (!found && app_count < 100) {
                strncpy(counts[app_count].bundle_id, bundle_id, 255);
                counts[app_count].count = 1;
                app_count++;
            }
        }
    }
    
    fclose(file);
    
    // sort by count (bubble sort)
    for (int i = 0; i < app_count - 1; i++) {
        for (int j = 0; j < app_count - i - 1; j++) {
            if (counts[j].count < counts[j + 1].count) {
                AppCount temp = counts[j];
                counts[j] = counts[j + 1];
                counts[j + 1] = temp;
            }
        }
    }
    
    // print stats
    printf("\nusage stats (last %d days):\n", days);
    printf("%-40s %s\n", "application", "launches");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < app_count; i++) {
        printf("%-40s %d\n", counts[i].bundle_id, counts[i].count);
    }
}

void cleanup_usage_tracker(void) {
    if (g_usage_file) {
        fclose(g_usage_file);
        g_usage_file = NULL;
    }
}