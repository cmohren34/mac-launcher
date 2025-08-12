#!/bin/bash

echo "mac app launcher installer"
echo "=========================="

# Check if running as root
if [ "$EUID" -eq 0 ]; then 
   echo "Don't run this as root/sudo"
   exit 1
fi

# Build
echo "Building launcher..."
make clean && make
if [ $? -ne 0 ]; then
    echo "Build FAILED"
    exit 1
fi

echo "installing binary..."
sudo cp build/launcher /usr/local/bin/
sudo chmod 755 /usr/local/bin/launcher

echo "creating launch agent..."
cat > ~/Library/LaunchAgents/com.launcher.plist << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" 
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.launcher</string>
    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/launcher</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <dict>
        <key>SuccessfulExit</key>
        <false/>
    </dict>
    <key>StandardOutPath</key>
    <string>/tmp/launcher.log</string>
    <key>StandardErrorPath</key>
    <string>/tmp/launcher.error.log</string>
</dict>
</plist>
EOF

# Unload if already loaded
launchctl unload ~/Library/LaunchAgents/com.launcher.plist 2>/dev/null

# Load the agent
echo "starting launcher service..."
launchctl load ~/Library/LaunchAgents/com.launcher.plist

echo ""
echo "installation COMPLETE!"
echo ""
echo "IMPORTANT: Grant Accessibility permissions"
echo "   1. Open System Settings"
echo "   2. Go to Privacy & Security → Accessibility"
echo "   3. Add 'launcher' to the list and enable it"
echo ""
echo "Usage:"
echo "   Option+1 through Option+9 to launch apps"
echo ""
echo "Check logs:"
echo "   tail -f /tmp/launcher.log"
echo ""
echo "To uninstall:"
echo "   ./uninstall.sh"