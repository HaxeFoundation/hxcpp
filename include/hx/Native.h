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



#define HX_DECLARE_NATIVE0(klass) \
	class klass;
#define HX_DECLARE_NATIVE1(ns1,klass) namespace ns1 { HX_DECLARE_NATIVE0(klass) }
#define HX_DECLARE_NATIVE2(ns2,ns1,klass) namespace ns2 { HX_DECLARE_NATIVE1(ns1,klass) }
#define HX_DECLARE_NATIVE3(ns3,ns2,ns1,klass) namespace ns3 { HX_DECLARE_NATIVE2(ns2,ns1,klass) }
#define HX_DECLARE_NATIVE4(ns4,ns3,ns2,ns1,klass) namespace ns4 { HX_DECLARE_NATIVE3(ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE5(ns5,ns4,ns3,ns2,ns1,klass) namespace ns5 { HX_DECLARE_NATIVE4(ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE6(ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns6 { HX_DECLARE_NATIVE5(ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns7 { HX_DECLARE_NATIVE6(ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns8 { HX_DECLARE_NATIVE7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_NATIVE9(ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns9 { HX_DECLARE_NATIVE8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }





namespace hx
{
   class Object;

   class HXCPP_CLASS_ATTRIBUTES NativeInterface
   {
      virtual hx::Object *__GetRealObject() = 0;
      void _hx_addRef();
      void _hx_decRef();
   };

   template<typename T>
   class Ref
   {
   };

   #define HX_NATIVE_IMPLEMENTATION hx::Object *__GetRealObject() { return 0; }
}

#endif

