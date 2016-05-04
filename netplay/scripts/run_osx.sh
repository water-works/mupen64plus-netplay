#!/bin/bash

if [ "$1" == "--help" ] || [ -z $1 ] || [ -z $2 ]
then
  echo
  echo "Usage: $0 [path to m64p bundle] [ROM file]"
  echo
  echo "You can download a mupen64plus bundle from the mupen64plus-core"
  echo "releases page: https://github.com/mupen64plus/mupen64plus-core"
  echo
  echo "Note this script assumes it's being run from the root of a fully-built"
  echo "mupen64plus-netplay repository."

  exit
fi

M64P_BUNDLE_ROOT=$1/mupen64plus.app/Contents
ROM_FILE=$2

M64P_CONFIG_TEMPLATE=netplay/scripts/data/mupen64plus.cfg-netplay-template.txt
M64P_CONFIG_DATADIR=/tmp/mupen64plus-netplay-configdir-tmp

mkdir -p $M64P_CONFIG_DATADIR
cp $M64P_CONFIG_TEMPLATE $M64P_CONFIG_DATADIR/mupen64plus.cfg

DYLD_FALLBACK_LIBRARY_PATH=./usr/lib:$M64P_BUNDLE_ROOT/libs:$DYLD_FALLBACK_LIBRARY_PATH \
  mupen64plus-ui-console/projects/unix/mupen64plus \
  --corelib mupen64plus-core/projects/unix/libmupen64plus.dylib \
  --configdir /tmp/mupen64plus-netplay-configdir-tmp \
  --datadir $M64P_BUNDLE_ROOT/Resources \
  --gfx $M64P_BUNDLE_ROOT/MacOS/mupen64plus-video-glide64mk2.dylib \
  --audio $M64P_BUNDLE_ROOT/MacOS/mupen64plus-audio-sdl.dylib \
  --input $M64P_BUNDLE_ROOT/MacOS/mupen64plus-input-sdl.dylib \
  --rsp $M64P_BUNDLE_ROOT/MacOS/mupen64plus-rsp-hle.dylib \
  --netplay netplay/build/lib/libMupen64Plugin.dylib \
  $ROM_FILE
