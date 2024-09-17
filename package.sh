#!/bin/sh

# Check if the variable $glowbuildfolder is set and not empty
if [ -z "$ammoniabuildfolder" ]; then
    # Check if libglow.dylib exists in the current directory
    if [ -f "./Build/Release/ammonia" ]; then
        # Set $glowbuildfolder to the current directory
        ammoniabuildfolder=$(pwd)
    else
        # Ask the user to enter the build directory's path
        echo "Please enter the build directory's path:"
        read ammoniabuildfolder
    fi
fi

# Create a directory for the scripts
mkdir $ammoniabuildfolder/scripts

# Create a postinstall script that copies sets up the LaunchDaemon
echo '#!/bin/sh\n\necho "<?xml version=\\"1.0\\" encoding=\\"UTF-8\\"?>\\n<!DOCTYPE plist PUBLIC \\"-//Apple//DTD PLIST 1.0//EN\\" \\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\\">\\n<plist version=\\"1.0\\">\\n<dict>\\n	<key>Label</key>\\n	<string>com.bedtime.ammonia</string>\\n	<key>ProgramArguments</key>\\n	<array>\\n		<string>/usr/local/bin/ammonia/ammonia</string>\\n	</array>\\n	<key>RunAtLoad</key>\\n	<true/>\\n	<key>KeepAlive</key>\\n	<false/>\\n</dict>\\n</plist>" > /Library/LaunchDaemons/com.bedtime.ammonia.plist\nlaunchctl load com.bedtime.ammonia\nexit 0' > $ammoniabuildfolder/scripts/postinstall

# Make the preinstall script executable
chmod +x $ammoniabuildfolder/scripts/postinstall

# Create a temporary directory and setup the installation files in it.
mkdir $ammoniabuildfolder/temp
mkdir $ammoniabuildfolder/temp/tweaks
cp $ammoniabuildfolder/./fridagum.dylib $ammoniabuildfolder/temp/
cp $ammoniabuildfolder/./Build/Release/ammonia $ammoniabuildfolder/temp/
cp $ammoniabuildfolder/./Build/Release/liblibinfect.dylib $ammoniabuildfolder/temp/
cp $ammoniabuildfolder/./Build/Release/libopener.dylib $ammoniabuildfolder/temp/

# Build the package
sudo pkgbuild --install-location /usr/local/bin/ammonia/ --root $ammoniabuildfolder/temp --scripts ./scripts --identifier net.bedtime.ammonia "$ammoniabuildfolder/ammonia.pkg"
rm -r $ammoniabuildfolder/scripts/

# Remove the temporary directory
rm -r $ammoniabuildfolder/temp