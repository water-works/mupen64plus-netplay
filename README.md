[GRPC Netplay]
==============

This project uses GRPC as a transport mechanism to implement a deterministic 
lockstep netplay for the mupen64plus emulator.

Building - Mac OS
-----------------

Install dependencies. This assumes you use [homebrew](https://brew.sh/) as a
package manager:

    # Install necessary packages
    brew install cmake automake libtool automake autoconf gettext sdl sdl2 pkg-config freetype libpng

    # Install the latest version of XCode
    xcode-select --install

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
