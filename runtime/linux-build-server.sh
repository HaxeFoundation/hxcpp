#!/bin/sh

# Script for building as many libs as possible from a linux64 box

cd ..
export HXCPP=$PWD
cd runtime

echo "HXCPP build " $HXCPP

rm -rf obj


echo "--- Android -----"
rm -rf ../bin/Android
mkdir ../bin/Android
neko ../run.n BuildLibs.xml -Dandroid

echo "--- Android v7 -----"
neko ../run.n BuildLibs.xml -Dandroid -DHXCPP_ARMV7

echo "--- Linux32 -----"
rm -rf ../bin/Linux
mkdir ../bin/Linux
neko ../run.n BuildLibs.xml -DNEKOPATH=libs/nekoapi/linux32

echo "--- Linux64 -----"
rm -rf ../bin/Linux64
mkdir ../bin/Linux64
neko ../run.n BuildLibs.xml -DHXCPP_M64

echo "Done."
