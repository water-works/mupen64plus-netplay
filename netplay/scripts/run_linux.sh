#!/bin/bash

if [ "$1" == "--help" ] || [ -z $1 ] || [ -z $2 ]
then
  echo
  echo "Usage: $0 [path to m64p bundle] [ROM file]"
  echo
  echo "You can download a mupen64plus bundle from the mupen64plus-core"
  echo "releases page: https://github.com/mupen64plus/mupen64plus-core/releases"
  echo "Note this script assumes the standalone archive, not the one made for"
  echo "Ubuntu."
  echo
  echo "Note this script assumes it's being run from the root of a fully-built"
  echo "mupen64plus-netplay repository."

  exit
fi

M64P_BUNDLE_ROOT=$1
ROM_FILE=$2

M64P_CONFIG_TEMPLATE=netplay/scripts/data/mupen64plus.cfg-netplay-template.txt
M64P_CONFIG_DATADIR=/tmp/mupen64plus-netplay-configdir-tmp

mkdir -p $M64P_CONFIG_DATADIR
cp $M64P_CONFIG_TEMPLATE $M64P_CONFIG_DATADIR/mupen64plus.cfg

./mupen64plus-ui-console/projects/unix/mupen64plus \
    --corelib mupen64plus-core/projects/unix/libmupen64plus.so.2 \
    --configdir $M64P_CONFIG_DATADIR \
    --datadir $M64P_BUNDLE_ROOT/ \
    --gfx $M64P_BUNDLE_ROOT/mupen64plus-video-rice.so \
    --audio $M64P_BUNDLE_ROOT/mupen64plus-audio-sdl.so \
    --input $M64P_BUNDLE_ROOT/mupen64plus-input-sdl.so \
    --rsp $M64P_BUNDLE_ROOT/mupen64plus-rsp-hle.so \
    --netplay netplay/build/lib/mupen64plus-netplay.so \
    $ROM_FILE
