Threads And Stacks
-------------------


### Conservative, co-operation
Hxcpp uses conservative stop-the-world GC, where the threads need to co-operate.
 - Threads must not change GC pointers in the collection phase
 - The thread stacks/registers must be scanned for GC pointers
 - Threads must not block without letting the GC system know not to wait for them, otherwise GC blocks until end of block 
   + call hx::GCEnterBlocking() / gc_enter_blocking() / (cpp.vm.Gc.enterGCFreeZone() from Haxe) before potentially blocking system call (fs,network, etc)
   + call hx::GCExitBlocking() / gc_exit_blocking() / (cpp.vm.Gc.exitGCFreeZone() from Haxe) before making more GC calls
   + Might need to pre-allocate buffers
   + Don't forget the exit blocking on error condition

### Foreign Threads
When you create a thread from haxe, it starts attached.  Before a non-haxe created thread can interact with hxcpp, some care must be taken, since GC allocations are done using a GC context per thread, and all threads must respect the stopped world.
  - Foreign threads must be attached-detached
     - SetTopOfStack(int * inTop,bool inPush)
     - *inTop* = pointer to top of stack to attach, or '0' to remove stack
     - *inPush* = usually true.  recursive attachment/detachment
  - Must not change things when the world is stopped
  - Must define their stack range for scanning
  - If you are attached, you may need to enter/exit gc free zone
  - Must release context when done, if no more calls are going to be made
  - Make sure local variables are covered in stack
    - compiler may reorder, so be careful
  - Read documentation because some things, eg audio callbacks, happen on other threads
  - You can use other techniques, eg
     - create a haxe thread, which blocks waiting for signal
     - foreign thread generates request and signals haxe thread
     - haxe thread performs job and generates data then signals foreign thread
     - foreign picks up data and carries on




### Top of Stack

 - To understand how to handle threads, you need a mental picture of the c++ stack
 - The stack usually goes "down".  That is, if the first stack location is 10000, the next one will be 9999 etc.
 - Historical, but consistent.  Except for emscripten which goes up - but still use same terminology/picture, just change the less-thans to greater-thans in code.

Say the system starts each program stack at 10000, the stack might look like this, with local variables and arguments pushed on the stack:

```
 10000 
 -----------------------------------------------
 9996  startup temp variable
 9992  startup temp variable
       -- main function --
 9988  main return address    - order and details of this are ABI specific
 9984  char ** argv
 9980  int     argc
```

Hxcpp then runs it main code, which starts with the macro HX_TOP_OF_STACK, which expands to something like:
```
   int t0 = 99;
   hx::SetTopOfStack(&t0,false);
   ...
   __boot_all();
   __hxcpp_main();

       -- main function --
 9988  main return address      order and details of this are ABI specific
 9984  char ** argv
 9980  int     argc
 9976  int     t0
       -- hx::SetTopOfStack --

    records '9976' as top of stack for this thread
```

Later, many generated functions deep, `__hxcpp_main` generates an allocation call which
triggers a collection

```
 ...
 8100  Array<Bullet>   bullets
       -- alloc Enemy --
 ...
       -- Call collect --

 8050 int   bottomOfStackTemp
      MarkConservative(&bottomOfStackTemp, 9976) -> scans stack from 8050 -> 9976
      MarkConservative(Capture registers)

```

Enter/exit use similar technique, where the registers are captured and the bottomOfStack is 'locked-in' when the "enter gc free zone" call is made.
```
 8100  Array<Bullet>   bullets
       -- EnterGCFreeZone --
 8088 int   bottomOfStackTemp
      thread->setBottomOfStack(&bottomOfStackTemp)
      thread->captureRegisters()
      return
      * any changes here will not affect GC
```

Now, when another thread does a collection, the gc-free thread can be scanned from 8088 to 9976, regardless of any stuff happening lower dowsn the stack.


### Not Called From Main 

Top of stack can be tricky to get right when a gui framework does not really have a "main".


```
 10000 
 -----------------------------------------------
 9996  startup temp variable
 9992  startup temp variable
       -- main function --
       setupWindows(onReadyCallback)......
          ...
 8000  
       -- onReadyCallback --
 7976  int     t0
       SetTopOfStack(&t0,false) -> 7966
       __hxcpp_main();
          setOnFrameCallack(haxeOnFrame)
          return;
```

Later, the haxeOnFrame callback is trigger, but not "below" `__hxcpp_main`

```
 9800  -- haxeOnFrame ---
     // Top of stack will be below bottom of stack.

```

Solutions:
  - Make sure you get in at top of main
    + may scan too much?
  - Ratchet up top-of-stack in callbacks, inForce = false
    + gc_set_top_of_stack(void * inTopOfStack,bool inForce);
  - Detach main thread after hxcpp_main and reattach each callback
    + android solution because render callbacks happen on different threads
    + gc_set_top_of_stack(&base,true); // attach
    + gc_set_top_of_stack(0,true); // detach



### Debugging.
  - in debug mode, hxcpp will check for calls from unattached threads
  - hxcpp can log conservative ranges.  With a native debugger you can check the address of
     your local variables and ensure they are included.
  - hxcpp will scan native objects on the stack, but will not follow non-haxe pointers to other objects, so additional GC roots may be required.


