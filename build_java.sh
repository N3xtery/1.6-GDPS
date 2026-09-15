#!/bin/bash
set -e

javac -source 1.5 -target 1.5 -cp "../AndroidBuilding/platforms/android-8/android.jar:../AndroidBuilding/jsoup-1.13.1.jar:../AndroidBuilding/mp3agic-0.8.1.jar:../AndroidBuilding/core-1.58.0.0.jar:../AndroidBuilding/prov-1.58.0.0.jar:../AndroidBuilding/bctls-jdk15on-1.58.0.0.jar:../AndroidBuilding/okhttp-1.5.4.jar" -d ./out ./java/*.java
../AndroidBuilding/build-tools/22.0.1/dx --dex --output=./bin/classes.dex ./out
java -jar ../AndroidBuilding/baksmali.jar disassemble ./bin/classes.dex -o ./bin/gd-unpacked/smali
