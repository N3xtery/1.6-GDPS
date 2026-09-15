#!/bin/bash -i
set -e

if [ "$1" == "all" ]; then
    ndk-build
    mv "./libs/x86/libgdps16.so" "./bin/gd-unpacked/lib/x86/libgdps16.so"
    #mv "./libs/armeabi/libgdps16.so" "./bin/gd-unpacked/lib/armeabi/libgdps16.so"
    mv "./libs/armeabi-v7a/libgdps16.so" "./bin/gd-unpacked/lib/armeabi-v7a/libgdps16.so"
else
    if [ "$1" == "x86" ]; then
        arch="x86"
    elif [ "$1" == "armv5" ]; then
        arch="armeabi"
    else
        arch="armeabi-v7a"
    fi
    ndk-build "APP_ABI=$arch"
    mv "./libs/$arch/libgdps16.so" "./bin/gd-unpacked/lib/$arch/libgdps16.so"
fi
apktool b "./bin/gd-unpacked" -o ./bin/temp_apk_unsigned.apk
../AndroidBuilding/build-tools/22.0.1/zipalign -v 4 ./bin/temp_apk_unsigned.apk ./bin/temp_apk_aligned.apk
../AndroidBuilding/build-tools/android-7.0/apksigner sign --key ./bin/apkeasytool.pk8 --cert ./bin/apkeasytool.pem --out "./bin/1.6 GDPS.apk" ./bin/temp_apk_aligned.apk
rm -f ./bin/temp_apk_unsigned.apk ./bin/temp_apk_aligned.apk
if [ "$1" == "x86" ]; then
    adb install -r "./bin/1.6 GDPS.apk"
else
    adb install --bypass-low-target-sdk-block "./bin/1.6 GDPS.apk"
fi
