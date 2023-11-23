#!/bin/bash

#!/bin/bash

FRIDA_VERSION=16.1.4

mkdir -p temp
cd temp

# Download and extract x86_64 frida-core library
if test -f "frida-core-x86_64.xz"; then
    echo frida-core-x86_64.xz already exists, skipping...
else
    echo Downloading frida-core-x86_64.xz...
    curl -L --output frida-core-x86_64.xz https://github.com/frida/frida/releases/download/$FRIDA_VERSION/frida-core-devkit-$FRIDA_VERSION-macos-x86_64.tar.xz
fi

# Download and extract arm64e frida-core library
if test -f "frida-core-arm64e.xz"; then
    echo frida-core-arm64e.xz already exists, skipping...
else
    echo Downloading frida-core-arm64e.xz...
    curl -L --output frida-core-arm64e.xz https://github.com/frida/frida/releases/download/$FRIDA_VERSION/frida-core-devkit-$FRIDA_VERSION-macos-arm64e.tar.xz
fi

# Download and extract arm64 frida-core library
if test -f "frida-core-arm64.xz"; then
    echo frida-core-arm64.xz already exists, skipping...
else
    echo Downloading frida-core-arm64.xz...
    curl -L --output frida-core-arm64.xz https://github.com/frida/frida/releases/download/$FRIDA_VERSION/frida-core-devkit-$FRIDA_VERSION-macos-arm64.tar.xz
fi

# Extract x86_64 libfrida-core.a
tar xf frida-core-x86_64.xz libfrida-core.a
mv libfrida-core.a libfrida-core-x86_64.a

# Extract arm64e libfrida-core.a
tar xf frida-core-arm64e.xz libfrida-core.a
mv libfrida-core.a libfrida-core-arm64e.a

# Extract arm64 libfrida-core.a
tar xf frida-core-arm64.xz libfrida-core.a
mv libfrida-core.a libfrida-core-arm64.a

# Check if all libraries were extracted successfully
if ! test -f "libfrida-core-x86_64.a" || ! test -f "libfrida-core-arm64e.a" || ! test -f "libfrida-core-arm64.a"; then
    echo Failed to extract all libfrida-core libraries
    exit 1
fi

# Create a FAT libfrida-core.a containing all architectures
lipo -create libfrida-core-x86_64.a libfrida-core-arm64e.a libfrida-core-arm64.a -output libfrida-core-x86_64-arm64e-arm64.a

# Check if the FAT library was created successfully
if ! test -f "libfrida-core-x86_64-arm64e-arm64.a"; then
    echo Failed to create libfrida-core-x86_64-arm64e-arm64.a
    exit 1
fi

# Download and extract x86_64 frida-gum library
if test -f "frida-gum-x86_64.xz"; then
    echo frida-gum-x86_64.xz already exists, skipping...
else
    echo Downloading frida-gum-x86_64.xz...
    curl -L --output frida-gum-x86_64.xz https://github.com/frida/frida/releases/download/$FRIDA_VERSION/frida-gum-devkit-$FRIDA_VERSION-macos-x86_64.tar.xz
fi

# Download and extract arm64e frida-gum library
if test -f "frida-gum-arm64e.xz"; then
    echo frida-gum-arm64e.xz already exists, skipping...
else
    echo Downloading frida-gum-arm64e.xz...
    curl -L --output frida-gum-arm64e.xz https://github.com/frida/frida/releases/download/$FRIDA_VERSION/frida-gum-devkit-$FRIDA_VERSION-macos-arm64e.tar.xz
fi

# Download and extract arm64 frida-gum library
if test -f "frida-gum-arm64.xz"; then
    echo frida-gum-arm64.xz already exists, skipping...
else
    echo Downloading frida-gum-arm64.xz...
    curl -L --output frida-gum-arm64.xz https://github.com/frida/frida/releases/download/$FRIDA_VERSION/frida-gum-devkit-$FRIDA_VERSION-macos-arm64.tar.xz
fi

# Extract x86_64 libfrida-gum.a
tar xf frida-gum-x86_64.xz libfrida-gum.a
mv libfrida-gum.a libfrida-gum-x86_64.a

# Extract arm64e libfrida-gum.a
tar xf frida-gum-arm64e.xz libfrida-gum.a
mv libfrida-gum.a libfrida-gum-arm64e.a

# Extract arm64 libfrida-gum.a
tar xf frida-gum-arm64.xz libfrida-gum.a
mv libfrida-gum.a libfrida-gum-arm64.a

# Check if all libraries were extracted successfully
if ! test -f "libfrida-gum-x86_64.a" || ! test -f "libfrida-gum-arm64e.a" || ! test -f "libfrida-gum-arm64.a"; then
    echo Failed to extract all libfrida-gum libraries
    exit 1
fi

# Create a FAT libfrida-gum.a containing all architectures
lipo -create libfrida-gum-x86_64.a libfrida-gum-arm64e.a libfrida-gum-arm64.a -output libfrida-gum-x86_64-arm64e-arm64.a

# Check if the FAT library was created successfully
if ! test -f "libfrida-gum-x86_64-arm64e-arm64.a"; then
    echo Failed to create libfrida-gum-x86_64-arm64e-arm64.a
    exit 1
fi

echo "Cleaning up..."

# Remove downloaded archives and temporary files
rm frida-core-arm64e.xz
rm frida-core-arm64.xz
rm frida-core-x86_64.xz
rm frida-gum-arm64e.xz
rm frida-gum-arm64.xz
rm frida-gum-x86_64.xz
rm libfrida-core-arm64e.a
rm libfrida-core-arm64.a
rm libfrida-core-x86_64.a
rm libfrida-gum-arm64e.a
rm libfrida-gum-arm64.a
rm libfrida-gum-x86_64.a

# Copy the FAT libraries to the desired location
cp libfrida-core-x86_64-arm64e-arm64.a ..
cp libfrida-gum-x86_64-arm64e-arm64.a ..

# Navigate back to the previous directory and remove the temporary directory
cd ..
rm -rf temp
