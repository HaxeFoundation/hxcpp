## Some notes about the integration

At the moment there is support for Zonemarkers with Haxe's sourcemapping, Messages & Plots. What is missing is Memory tracking, though @Aidan63 has the large object heap + stacktraces working and is testing an idea on how to make all allocations visible.

### Haxe-Code
We hook Tracy into hxcpp's `hx::StackContext` and this way it spreads over all generated cpp code catching & measuring all the callstacks. 

There are however native parts of hxcpp that wont be visible by default in Tracy (bc there are no ZoneScopes).
> Note: There is a exception in the GC-Code, so it becomes visible for us.

### externs
The same is true about externs you might be using in your project. If you want to make these visible, you need to add Tracy's C-Macros yourself in the extern's c/cpp-code. You should be able to include Tracy's client headers or if it's cpp-files you can just `#include <hxcpp.h>`, which you need for precompiled headers anyway. 

### externs with static/dynamic libs
Another special case are static or dynamic libs your externs might be using. For these you will have to make more changes that are beyond the scope of this doc here, please refer to Tracy's official documentation over here: https://github.com/wolfpld/tracy/releases/latest/download/tracy.pdf


# How to use 

To activate the tracy integration, compile your app with:

```
-D HXCPP_TRACY
-D HXCPP_STACK_TRACE
-D HXCPP_STACK_LINE
```

and use the following call in your mainloop:

```
#if defined(HXCPP_TRACY)
  untyped __cpp__('FrameMark');
#endif
```

> Note: The Tracy-Headers are included with `hxcpp.h` and the untyped Tracy C Macros should be available everywhere in your code.
> Alternatively you can use this simple extern class in your project: https://gist.github.com/dazKind/b36475c0846491aefdfb12c5f831daba


Then start Tracy listening on localhost and start your hxcpp-made exe.