# The ARMv7 is significanly faster due to the use of the hardware FPU
APP_OPTIM := debug
#APP_OPTIM := release

#APP_ABI := armeabi
#APP_ABI := armeabi-v7a
#APP_ABI := mips
#APP_ABI := x86
#APP_ABI := x86_64
APP_ABI := arm64-v8a

#APP_ABI := x86 x86_64

#APP_ABI := armeabi armeabi-v7a mips x86
#APP_ABI := x86_64 arm64-v8a
#APP_ABI := armeabi armeabi-v7a mips x86 x86_64 arm64-v8a
#armeabi-v7a mips
#mips mips-r2
#x86
#armeabi-v7a
APP_PLATFORM := android-8
#APP_PLATFORM := android-3

APP_STL := c++_static
