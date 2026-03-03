
## How to use 

To activate the tracy integration, compile your app with:

```
-D HXCPP_TRACY
-D HXCPP_TELEMETRY
-D HXCPP_STACK_TRACE
-D HXCPP_STACK_LINE
```

and use the following call in your mainloop:

```
cpp.vm.tracy.TracyProfiler.frameMark();
```

Then start either Tracy's UI-App or cmdline server listening on localhost and start your hxcpp-made binary.


## Some notes about the integration

### Haxe-Code
We integrate Tracy into hxcpp as a Telemetry option which utilizes `hx::StackPosition` and offer a set of static functions to set zones and other tracy functionality. Through this all your haxe-code will be captured in a profiler-session.

There are however native parts of hxcpp that wont be visible by default in Tracy (bc there are no ZoneScopes). 

> Note: Exceptions are in a few spots in the GC-Code, so GC becomes visible for us.

> Note: Hxcpp's native calls will become visible if you use the option to capture callstacks.

> Note: We capture source-locations and their filepaths. By default these are relative to your project and thus the sourcecode preview / browsing in Tracy wont work since it expects absolute paths. To solve this you can use `-D absolute-path` in your builds.

### externs
The same is true about externs you might be using in your project. If you want to make these visible, you need to `@:include('hx/TelemetryTracy.h')` and you gain access to Tracy's C-Macros that you can use in your extern's c/cpp-code. Please refer to the official Tracy documentation: https://github.com/wolfpld/tracy/releases/latest/download/tracy.pdf

### externs with static/dynamic libs
Another special case are static or dynamic libs your externs might be using. For these you will have to make more changes that are beyond the scope of this doc here, please refer to Tracy's official documentation over here: https://github.com/wolfpld/tracy/releases/latest/download/tracy.pdf

## Optional Features

### Memory Profiling

The following define adds tracking (de-)allocations of hxcpp's small & large object heap.

```
-D HXCPP_TRACY_MEMORY
```

### Capture Callstacks

By default we only track zones. If you wanna inspect the actual callstack per zone, you should use the following define:

```
-D HXCPP_TRACY_INCLUDE_CALLSTACKS
```

> Note: This will inflate the telemetry data A LOT and cost more performance. Please be aware. 


### On Demand Profiling

By default this integration will start sampling & collecting telemetry with the start of your application. You can change this behavior by the following define and your app will only generate telemetry if the Tracy Profiler app is open and reachable. 

```
-D HXCPP_TRACY_ON_DEMAND
```

### Short-lived Application Support

In cases where you dont have a mainloop or a very short-lived application you can use the following define to let your application stay around to complete sending telemetry data it has collected.  

```
-D HXCPP_TRACY_NO_EXIT
```