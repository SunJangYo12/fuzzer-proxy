export AFL_SKIP_CPUFREQ=1
export LD_LIBRARY_PATH=../../
export AFL_FUZZER_STATS_UPDATE_INTERVAL=3
export AFL_NO_UI=1


chmod 777 android_copy/afl-fuzz
chmod 777 android_copy/examples/pidandroid/afl-proxy

mkdir -p /sdcard/zz_out
mkdir -p "android_copy/examples/pidandroid/.tmp/in"
echo "AAAAA" > "android_copy/examples/pidandroid/.tmp/in/tes"

cd android_copy/examples/pidandroid
../../afl-fuzz -i .tmp/in -o /sdcard/zz_out ./afl-proxy > /dev/null &

