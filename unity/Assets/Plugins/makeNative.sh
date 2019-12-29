# Script for recreating native libraries.
# Developed on Linux, Fedora 28.

# Remember: you may need to stop and start unity (actually shut it down) to have
# it reload the libraries.

# Build the android version of the library.
(
  export PATH=$PATH:/opt/gradle/gradle-5.5.1/bin
  cd ../../../android
  ### FIXME: change this to the local of your android sdk installation.
  export ANDROID_HOME=~/android-sdks/
  gradle build -x lint
)

# Link the library into the unity project.
cp ../../../android/app/build/intermediates/cmake/debug/obj/armeabi-v7a/libcr3engine-3-2-X.so Android/libs/armeabi-v7a/libCREngine.so
cp ../../../android/app/build/intermediates/cmake/debug/obj/arm64-v8a/libcr3engine-3-2-X.so Android/libs/arm64-v8a/libCREngine.so

# Build the x86_64 version of the library.
(
  cd ../../..
  mkdir qtbuild
  cd qtbuild
  cmake -D GUI=QT -D CMAKE_BUILD_TYPE=Release -D MAX_IMAGE_SCALE_MUL=2 -D DOC_DATA_COMPRESSION_LEVEL=3 -D DOC_BUFFER_SIZE=0x1400000 -D CMAKE_INSTALL_PREFIX=/usr -D CR3_JPEG=1 ..
  make
)

# Link the library into the unity project.
g++ -shared -fPIC -o x86_64/libCREngine.so -Wl,--whole-archive ../../../qtbuild/crengine/libcrengine.a ../../../qtbuild/thirdparty/antiword/libantiword.a ../../../qtbuild/thirdparty/chmlib/libchmlib.a ../../../qtbuild/thirdparty/libjpeg/libjpeg.a -Wl,--no-whole-archive

