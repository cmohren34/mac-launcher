#!/bin/bash

echo "Uninstalling launcher..."

# Stop service
launchctl unload ~/Library/LaunchAgents/com.launcher.plist 2>/dev/null

# Remove files
sudo rm -f /usr/local/bin/launcher
rm -f ~/Library/LaunchAgents/com.launcher.plist
rm -rf ~/.launcher

echo "✅ Launcher uninstalled"