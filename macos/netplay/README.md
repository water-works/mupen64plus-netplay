# macos/netplay/

## Directory overview

`Sources/Netplay/`: Business logic to communicate with the netplay server, the emulator, and the emulator plugins. Tests are at `Tests/Netplay/`.

`NetplayApp/`: Cocoa UI and UI logic. It does not live in `Sources/` because this was generated by xcode, whereas `Netplay/` was generated with the swift CLI.

TODO: A swift target to wrap the emulator binary and plugins.

## Getting started

From this directory,

`swift build` to pull in deps (requires swift CLI version 4.0+). Then open `netplay.xcodeproj`.
