[GRPC Netplay]
==============

This project uses GRPC as a transport mechanism to implement a deterministic 
lockstep netplay for the mupen64plus emulator.

Building - MacOS
-----------------

Install dependencies. This assumes you use [homebrew](https://brew.sh/) as a
package manager:

    # Install necessary packages
    brew install cmake automake libtool automake autoconf gettext sdl sdl2 pkg-config freetype libpng nasm

Add `export PATH="/usr/local/bin:$PATH"` to your shell config file (~/.bash_profile for bash users), which will use the version of nasm installed by Homebrew instead of the standard version that comes with MacOS.

From the App Store, install the [latest version of Xcode](https://itunes.apple.com/us/app/xcode/id497799835?mt=12) (necessary to get the MacOS Platform command line tools).

Force the xcode command line tools to use the Xcode app which contains necessary features: `sudo xcode-select -switch /Applications/Xcode.app/`.

Clone and build the repo:

    git clone https://github.com/water-works/mupen64plus-netplay.git
    cd mupen64plus-netplay

    # Pull down all dependencies.
    git submodule update --init --recursive

    # Build
    cmake .
    make -j8

Run tests. Note the DYLD_FALLBACK_LIBRARY_PATH is necessary because we don't 
build in the location of many libraries. This is a known issue:

    cd netplay
    DYLD_FALLBACK_LIBRARY_PATH=$(pwd)/../usr make test

Building - Linux (Debian/Ubuntu)
--------------------------------

Install dependencies:

    sudo apt-get install build-essential git cmake libsdl2-dev autoconf \
                         libtool shtool libpng-dev libfreetype6-dev nasm

Clone and build the repo:

    git clone https://github.com/water-works/mupen64plus-netplay.git
    cd mupen64plus-netplay

    # Pull down all dependencies.
    git submodule update --init --recursive

    # Build
    cmake .
    make -j8

Run tests:

    cd netplay
    make test
