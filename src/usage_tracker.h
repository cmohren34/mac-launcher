#ifndef USAGE_TRACKER_H
#define USAGE_TRACKER_H

void init_usage_tracker(void);

void track_launch(const char* bundle_id);

void get_usage_stats(int days);

void cleanup_usage_tracker(void);

#endif