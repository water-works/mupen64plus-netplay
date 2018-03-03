# [GRPC Netplay]

This project uses GRPC as a transport mechanism to implement a deterministic 
lockstep netplay for the mupen64plus emulator.

## Building - Mac OS

Install dependencies. This assumes you use [homebrew](https://brew.sh/) as a
package manager:

```shell
# Install necessary packages
brew install cmake automake libtool automake autoconf gettext sdl sdl2 pkg-config freetype libpng
```

From the App Store, install the [latest version of Xcode](https://itunes.apple.com/us/app/xcode/id497799835?mt=12) (necessary to get the MacOS Platform command line tools).

Clone and build the repo:

```shell
git clone https://github.com/water-works/mupen64plus-netplay.git
cd mupen64plus-netplay

# Pull down all dependencies.
git submodule update --init --recursive

# Build
cmake .
make -j8
```

Run tests. Note the DYLD_FALLBACK_LIBRARY_PATH is necessary because we don't 
build in the location of many libraries. This is a known issue:

```shell
cd netplay
DYLD_FALLBACK_LIBRARY_PATH=$(pwd)/../usr make test
```

### Building MacosNetplayApp

After following the above instructions, go to mupen64plus-netplay/third-party/grpc-swift/ and `make`.

Open the Xcode workspace file located in mupen64plus-netplay/macos/.  Add the files `Sources/CgRPC/shim/cgrpc.h` and `Sources/CgRPC/shim/internal.h` to the GrpcSwift project.  Add a new "Headers" build phase to GrpcSwift and expose those two files publically.  Then build the MacosNetplayApp project through Xcode.

## Building - Linux (Debian/Ubuntu)

Install dependencies:

```shell
sudo apt-get install build-essential git cmake libsdl2-dev autoconf \
                     libtool shtool libpng-dev libfreetype6-dev nasm
```

Clone and build the repo:

```shell
git clone https://github.com/water-works/mupen64plus-netplay.git
cd mupen64plus-netplay

# Pull down all dependencies.
git submodule update --init --recursive

# Build
cmake .
make -j8
```

Run tests:

```shell
cd netplay
make test
```
