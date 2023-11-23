#!/bin/bash

xcodebuild -project ammonia.xcodeproj -target opener -configuration clean build
xcodebuild -project ammonia.xcodeproj -target libinfect -configuration clean build
xcodebuild -project ammonia.xcodeproj -target extravagant -configuration clean build
xcodebuild -project ammonia.xcodeproj -target ammonia -configuration clean 

mkdir /usr/local/bin/ammonia/

mv ./Build/Release/ammonia /usr/local/bin/ammonia/
mv ./Build/Release/liblibinfect.dylib /usr/local/bin/ammonia/
mv ./Build/Release/libopener.dylib /usr/local/bin/ammonia/
