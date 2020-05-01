#ifndef HX_OBJC_HELPERS_INCLUDED
#define HX_OBJC_HELPERS_INCLUDED



@interface NSHaxeWrapperClass : NSObject {
  @public hx::Object *haxeObject;
}
- (id)init:(hx::Object *) inHaxe;
- (void)dealloc;
@end


NSDictionary<NSString *, id> *_hx_obj_to_nsdictionary(Dynamic d);

id _hx_value_to_objc(Dynamic d);


Dynamic _hx_nsdictionary_to_obj(NSDictionary<NSString *, id> *inDictionary);

Dynamic _hx_objc_to_dynamic(id inValue);

Array<unsigned char> _hx_objc_to_bytes(id value);


namespace hx
{


// 0 args
struct TObjcBlockVoidVoid
{
   typedef void (^t)();

   static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^ {
         wrapper->haxeObject->__run();
      };
      return wrap;
   }
};


template<typename Ret>
struct TObjcBlockRetVoid
{
   typedef Ret (^t)();

   static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^() {
         return (Ret) wrapper->haxeObject->__run();
      };
      return wrap;
   }
};


// 1 arg
template<typename Arg0>
struct TObjcBlockVoidArgs1
{
   typedef void (^t)(Arg0 a);

   static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0) {
         wrapper->haxeObject->__run(a0);
      };
      return wrap;
   }
};

template<typename Ret, typename Arg0>
struct TObjcBlockRetArgs1
{
   typedef Ret (^t)(Arg0 a);
   inline static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0) {
         return (Ret) wrapper->haxeObject->__run(a0);
      } ;
   }
};



// 2 arg
template<typename Arg0, typename Arg1>
struct TObjcBlockVoidArgs2
{
   typedef void (^t)(Arg0 a0, Arg1 a1);

   static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0, Arg1 a1) {
         wrapper->haxeObject->__run(a0,a1);
      };
      return wrap;
   }
};

template<typename Ret, typename Arg0, typename Arg1>
struct TObjcBlockRetArgs2
{
   typedef Ret (^t)(Arg0 a0, Arg1 a1);
   inline static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0, Arg1 a1) {
         return (Ret) wrapper->haxeObject->__run(a0,a1);
      } ;
   }
};


// 3 arg
template<typename Arg0, typename Arg1, typename Arg2>
struct TObjcBlockVoidArgs3
{
   typedef void (^t)(Arg0 a0, Arg1 a1, Arg2 a2);

   static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0, Arg1 a1, Arg2 a2) {
         wrapper->haxeObject->__run(a0,a1,a2);
      };
      return wrap;
   }
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2>
struct TObjcBlockRetArgs3
{
   typedef Ret (^t)(Arg0 a0, Arg1 a1, Arg2 a2);
   inline static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0, Arg1 a1, Arg2 a2) {
         return (Ret) wrapper->haxeObject->__run(a0,a1,a2);
      } ;
   }
};


// 4 arg
template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
struct TObjcBlockVoidArgs4
{
   typedef void (^t)(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3);

   static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3) {
         wrapper->haxeObject->__run(a0,a1,a2,a3);
      };
      return wrap;
   }
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
struct TObjcBlockRetArgs4
{
   typedef Ret (^t)(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3);
   inline static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3) {
         return (Ret) wrapper->haxeObject->__run(a0,a1,a2,a3);
      } ;
   }
};


// 5 arg
template<typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
struct TObjcBlockVoidArgs5
{
   typedef void (^t)(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4);

   static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) {
         wrapper->haxeObject->__run(a0,a1,a2,a3,a4);
      };
      return wrap;
   }
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
struct TObjcBlockRetArgs5
{
   typedef Ret (^t)(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4);
   inline static t create(Dynamic func)
   {
      NSHaxeWrapperClass *wrapper = [[NSHaxeWrapperClass alloc] init:func.mPtr];
      t wrap = ^(Arg0 a0, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) {
         return (Ret) wrapper->haxeObject->__run(a0,a1,a2,a3,a4);
      } ;
   }
};








}

#endif

