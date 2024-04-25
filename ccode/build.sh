read -p "[*] build linux/android/remove/copyandroid-example? l/a/rm/cac: " input
#input="l"

if [ "$input" == "l" ]; then
   read -p "[+] build module? y/n: " i_module
   if [ "$i_module" == "y" ]; then
      cd afl_proxy/module
      gcc -fPIC -shared module.c  -o libmodule.so -lpthread
      cd -
   fi

   read -p "[+] build afl-proxy? y/n: " i_afl
   if [ "$i_afl" == "y" ]; then
      gcc -g afl_proxy/afl-proxy.c -o afl_proxy/afl-proxy afl_proxy/module/libmodule.so -pthread
   fi


elif [ "$input" == "rm" ]; then
   read -p "[+] remove linux? y/n: " i_rm
   if [ "$i_rm" == "y" ]; then
      rm afl_proxy/module/libmodule.so
      rm afl_proxy/afl-proxy
   fi
   read -p "[+] remove android? y/n: " i_rm
   if [ "$i_rm" == "y" ]; then
      rm -r "AFL/AFLplusplus-4.06c/.build_android"
      rm -r "afl_proxy/.build_android"
      rm -r "afl_proxy/module/.build_android"
      rm -r ".tmp/android_copy"
   fi
   read -p "[+] remove android_copy? y/n: " i_rm
   if [ "$i_rm" == "y" ]; then
      rm -r ".tmp/android_copy"
   fi


elif [ "$input" == "a" ]; then
   TMPATH=".tmp/android_copy"
   if [ ! -d $TMPATH ]; then
      mkdir -p $TMPATH
   fi

   read -p "[+] build module? y/n: " i_module
   if [ "$i_module" == "y" ]; then
      APATH=afl_proxy/module/.build_android
      rm -R $APATH
      mkdir $APATH
      cd $APATH
      cmake \
        -DANDROID_PLATFORM=25 \
        -DCMAKE_TOOLCHAIN_FILE=/opt/cross/android-ndk-r25c/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=arm64-v8a ..
      make
      cd -
      cp "afl_proxy/module/.build_android/libmodule.so" $TMPATH
   fi

   read -p "[+] build afl-proxy? y/n: " i_aflproxy
   if [ "$i_aflproxy" == "y" ]; then
      APATH=afl_proxy/.build_android
      rm -r $APATH
      mkdir -p $APATH
      cd $APATH
      cmake \
	-DANDROID_PLATFORM=25 \
	-DCMAKE_TOOLCHAIN_FILE=/opt/cross/android-ndk-r25c/build/cmake/android.toolchain.cmake \
	-DANDROID_ABI=arm64-v8a ..
      make
      cd -
      cp "afl_proxy/.build_android/afl-proxy" $TMPATH
   fi

   read -p "[+] build afl-fuzz+trace? y/n: " i_aflfuzz
   if [ "$i_aflfuzz" == "y" ]; then
      APATH=AFL/AFLplusplus-4.06c
      echo "[+] copy gum..."
      rm -R $APATH/.build_android
      mkdir $APATH/.build_android
      cp -R $APATH/gum-binary $APATH/.build_android
      mv $APATH/.build_android/gum-binary $APATH/.build_android/frida-16.0.13
      cd $APATH/.build_android
      cmake -DANDROID_PLATFORM=25 \
        -DCMAKE_TOOLCHAIN_FILE=/opt/cross/android-ndk-r25c/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=arm64-v8a ..
      make -j2
      cd -
      cp "AFL/AFLplusplus-4.06c/.build_android/afl-fuzz" $TMPATH
      cp "AFL/AFLplusplus-4.06c/.build_android/afl-frida-trace.so" $TMPATH
   fi


elif [ "$input" == "cac" ]; then
   LIBPATH=".tmp/android_copy/examples/libandroid"
   PIDPATH=".tmp/android_copy/examples/pidandroid"
   TMPATH=".tmp/android_copy"

   rm -R ".tmp/android_copy"
   mkdir -p ".tmp/android_copy"
   mkdir -p $LIBPATH
   mkdir -p $PIDPATH

   echo "[+] copying fuzzer"
   cp "AFL/AFLplusplus-4.06c/.build_android/afl-fuzz" $TMPATH
   cp "AFL/AFLplusplus-4.06c/.build_android/afl-frida-trace.so" $TMPATH
   cp "afl_proxy/module/.build_android/libmodule.so" $TMPATH

   echo "[+] copying library fuzzing"
   cp "../examples/android/libandroid/build/harness" $LIBPATH
   cp "../examples/android/libandroid/afl.js" $LIBPATH
   cp "../examples/android/libandroid/run.sh" $LIBPATH
   cp "../examples/android/libandroid/lib/libblogfuzz.so" $LIBPATH

   echo "[+] copying pid fuzzing"
   cp "afl_proxy/.build_android/afl-proxy" $PIDPATH
   cp "../examples/android/pidandroid/run-afl.sh" $PIDPATH
   cp "../examples/android/pidandroid/build/tesPid" $PIDPATH

   echo "[+] upload using ssh"
   scp -P 8022 -r .tmp/android_copy l@192.168.43.1:/sdcard

else
   echo "[!] input wrong."
fi

