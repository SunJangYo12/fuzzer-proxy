#!/usr/bin/env python3

import frida
import sys
import os
import time
import datetime
from ctypes import *

pwd = None

def on_message(message, data):
    if message['type'] == 'send':
        if message['payload']['type'] == 'prepare':
           print("[*] prepare successfull")

        if message['payload']['type'] == 'log_message':
           print(message['payload']['log'])

        elif message['payload']['type'] == 'fuzz_result':
           xdate = datetime.datetime.now()
           xlog = message['payload']['log']
           xiteration = message['payload']['iteration']
           xstage = message['payload']['stage']

           print("[*] PY Crash log: ", xlog)

           f = open("examples/.crash/data.txt", "a")
           f.write("""
*********** {} **********
Log: {}
iteration: {}
stage: {}
\n\n
                   """.format(xdate, xlog, xiteration, xstage))
           f.close()

    elif message['type'] == 'error':
        print(message['stack'])

def exiting():
    print("Exiting!")

def check_afl(script, target):
    if target == 'a':
       apwd = script.exports_sync.getpwd()
       script.exports_sync.shell("cd {}; pidof afl-proxy > .pid-aflproxy".format(apwd))
       is_afl = script.exports_sync.readtext("{}/.pid-aflproxy".format(apwd)).split("\n");
    else:
       script.exports_sync.shell("pidof afl-proxy > .pid-aflproxy")
       is_afl = script.exports_sync.readtext(".pid-aflproxy").split("\n");


    if is_afl[0] == "=" or is_afl[0] == "":
       print("[!] afl-fuzz not running, please run afl-proxy first\n")
       exit(0)
    else:
       print("[+] afl-fuzz is running")


def inject_module(script, target):
    triggerAddr = None
    while True:
       if target == "a":
          triggerAddr = script.exports_sync.injectmodule("android", None);
       elif target == "l":
          pathmodule = "{}/ccode/afl_proxy/module/libmodule.so".format(os.getcwd())
          triggerAddr = script.exports_sync.injectmodule("linux", pathmodule);
       else:
          print("[!] PY error target inject module")
          exit(1)

       if triggerAddr == "0x0":
          print("[+] PY inject libmodule.so")
          time.sleep(1)
       else:
          print("[+] PY inject libmodule.so successfully")
          print("[*] PY libmodule.so trigger addr: {}".format(triggerAddr))
          break
    if triggerAddr == None:
       print("[!] error inject module")
       exit()
    return triggerAddr


def main():
    print("\n\t=====================")
    print("\t Fuzzer proxy v1.0.4")
    print("\t=====================\n")
    target = input(">> Select target? Linux/Android (l/a): ")

    if target == "a":
       #ahost = input(">> Android host: ")
       #device = frida.get_device_manager().add_remote_device(ahost)
       device = frida.get_device_manager().add_remote_device("192.168.43.1")
    elif target == "l":
       device = frida.get_local_device() #local linux
    else:
       print("[!] PY target not found")
       exit(1)

    pid_raw = input(">> Chose pid? (1234): ")

    pid = int(pid_raw)
    session = device.attach(pid)

    with open('js/harness.js', 'r') as file:
        data = file.read()
        script = session.create_script(data)

    script.on("message", on_message)
    script.load()

    print("[+] PY inject harness.js successfully")

    pwd = script.exports_sync.getpwd()

    if pwd == None:
       print("[!] can't getting pwd in injectedlib")
    else:
       print("[+] PY dataDir: {}".format(pwd))

    pshell = input("\n>> Continue/shell/reverse_shell_java/copy_fuzzer(c/s/sj/cf): ")
    if pshell == "cf":
        print("[+] PY set permission. {}".format(pwd))
        script.exports_sync.shell("chmod 777 {}".format(pwd))

        fuzzlocate = input(">> fuzzer locate (/sdcard/android_copy): ")
        script.exports_sync.shell("cp -R {} {}".format(fuzzlocate, pwd))
        print("[+] PY fuzzer and library copied.")
        exit(0)

    if pshell == "sj":
        rsip = input(">> listen ip: ")
        rsport = input(">> listen port: ")
        script.exports_sync.reshelljava(rsip, rsport)
        exit(0)

    if pshell == "s":
        shell_cmd = input(">> Command shell (ls): ")
        script.exports_sync.shell(shell_cmd)
        exit(0)

    triggerAddr = inject_module(script, target)

    pshell = input("\n>> Continue/reverse_shell(c/rs): ")
    if pshell == "rs":
        rsip = input(">> listen ip: ")
        rsport = input(">> listen port: ")
        rsbin = input(">> locate bin(/bin/sh): ")
        print(script.exports_sync.reshell(rsip, rsport, rsbin))

        exit(0)


    if target == "a":
        pmethod = input("\n>> Select method fuzzing? library/attacking/view (lib/libview/pid): ")

        if pmethod == "pid":
            #ipid = input(">> Chose pid target: ")
            print("[+] PY start example targetPid...")
            script.exports_sync.shell("cd {}; pidof tesPid > .pid-tesPid".format(pwd))
            example_pid = script.exports_sync.readtext("{}/.pid-tesPid".format(pwd)).split("\n");

            print("[+] tesPid running in pid: {}".format(example_pid[0]))


            #print("[+] PY setup afl-proxy...")
            #script.exports_sync.shell("cd {}; /system/bin/sh android_copy/examples/pidandroid/run-afl.sh".format(pwd))
            #check_afl(script, target)

            comm_id = script.exports_sync.prepare(triggerAddr, "android", None);
            exit()


        elif pmethod == "lib" or pmethod == "libview":
            if pmethod == "lib":
               script.exports_sync.shell("cd {}; /system/bin/sh android_copy/examples/libandroid/run.sh".format(pwd))

            count = 0;
            while True:
                result = script.exports_sync.readtext("/sdcard/zz_out/default/fuzzer_stats").split("\n")
                print("[{}]".format(count))
                print("{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}\n".format(result[8], result[11], result[10], result[19], result[21], result[22], result[25], result[23], result[4]))
                time.sleep(1)
                count += 1

    elif target == "l":
        check_afl(script, target)
        comm_id = script.exports_sync.prepare(triggerAddr, "linux", os.getcwd());
        if comm_id == 0:
            print("[!] error prepare fuzzing")
            exit()


    while target == "l":
        try:
           status = os.kill(pid, 0) #cek kehidupan target
           try:
              script.exports_sync.fuzzinmodule(triggerAddr, comm_id);
           except:
              print("[*] PY error post JS")
        except:
           time.sleep(2)
           print("program target not running, crash?")


    session.on('detached', exiting)


if __name__ == "__main__":
    main()
