#ifndef HX_NATIVE_INCLUDED_H
#define HX_NATIVE_INCLUDED_H
/*
 *
 This file is in the public domain, and can be freely distributed.

*/

#ifndef HXCPP_H
#define HXCPP_H
typedef double Float;
typedef int Int;
typedef bool Bool;
typedef void Void;

#ifndef HXCPP_CLASS_ATTRIBUTES
#define HXCPP_CLASS_ATTRIBUTES
#endif

#endif

namespace hx
{
   class Object;

   typedef Object NativeInterface;

   /*
   class HXCPP_CLASS_ATTRIBUTES NativeInterface
   {
      virtual hx::Object *__GetRealObject() = 0;
      void _hx_addRef();
      void _hx_decRef();
   };
   */

   template<typename T>
   class Ref
   {
   };

   #define HX_NATIVE_IMPLEMENTATION hx::Object *__GetRealObject() { return 0; }
}

#endif

