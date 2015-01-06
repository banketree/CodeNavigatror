ndk_build_path=`which ndk-build`
if test "$?" != "0" ; then
     echo "ndk-build not found in path. Please specify path to android NDK as first argument of $0."
     exit 127
fi
NDK_PATH=`dirname $ndk_build_path`
if test -z "NDK_PATH" ; then
     echo "Path to Android NDK not set, please specify it as first argument of $0."
     exit 127
fi
echo "using $NDK_PATH as android NDK"

PROJECT_PATH=`pwd`
cd $NDK_PATH/platforms/android-17/arch-arm/usr
mv lib lib.old

cp -rf  $PROJECT_PATH/jni/lib_a20.tgz lib.tgz
tar zxvf lib.tgz

