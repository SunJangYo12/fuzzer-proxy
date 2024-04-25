export AFL_SKIP_CPUFREQ=1
export LD_LIBRARY_PATH=.
export AFL_FUZZER_STATS_UPDATE_INTERVAL=3
export AFL_NO_UI=1


chmod 777 android_copy/afl-fuzz
chmod 777 android_copy/examples/libandroid/harness

mkdir -p /sdcard/zz_out
mkdir -p "android_copy/examples/libandroid/.tmp/in"
echo "AAAAA" > "android_copy/examples/libandroid/.tmp/in/tes"

cd android_copy/examples/libandroid
../../afl-fuzz -O -G 256 -i .tmp/in -o /sdcard/zz_out ./harness > /dev/null &
