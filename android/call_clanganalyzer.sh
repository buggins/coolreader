#!/bin/sh

ndk-build clean

export NDK_ANALYZE=1
#export NDK_ANALYZER_OUT="./analyzer"

ndk-build -j5
