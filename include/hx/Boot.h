#ifndef HX_BOOT_H
#define HX_BOOT_H

// Properly construct all the classes defined in the haxe code
void __boot_all();

namespace hx
{

// Initializer the hxcpp runtime system
HXCPP_EXTERN_CLASS_ATTRIBUTES void Boot();

}

#endif
