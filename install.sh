#!/bin/bash

xcodebuild -project ammonia.xcodeproj -target opener -configuration clean build
xcodebuild -project ammonia.xcodeproj -target libinfect -configuration clean build
xcodebuild -project ammonia.xcodeproj -target extravagant -configuration clean build
xcodebuild -project ammonia.xcodeproj -target ammonia -configuration clean 

mkdir /usr/local/bin/ammonia/

cp ./fridagum.dylib /usr/local/bin/ammonia/

mv ./Build/Release/ammonia /usr/local/bin/ammonia/
mv ./Build/Release/liblibinfect.dylib /usr/local/bin/ammonia/
mv ./Build/Release/libopener.dylib /usr/local/bin/ammonia/

# Define the path to the plist file
plist_file="/Library/LaunchDaemons/com.amm.loader.plist"

# Check if the file exists
if [ ! -f "$plist_file" ]; then
    # Create the plist file if it doesn't exist
    cat <<EOF | sudo tee "$plist_file" > /dev/null
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.amm.loader</string>
    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/ammonia/ammonia</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <false/>
</dict>
</plist>
EOF
fi

# Load the launchd job
launchctl load -w "$plist_file"

