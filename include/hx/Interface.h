#ifndef HX_INTERFACE_H
#define HX_INTERFACE_H

namespace hx
{

HXCPP_EXTERN_CLASS_ATTRIBUTES void InvalidInterface();

template<typename T>
inline T interface_cast(void *ptr)
{
   #if defined(HXCPP_GC_CHECK_POINTER) || defined(HXCPP_DEBUG)
   if (!ptr) hx::InvalidInterface();
   #endif
   return static_cast<T>(ptr);
}

template<typename T>
inline T interface_check(T inObj,int interfaceId)
{
   Dynamic d(inObj);
   if ( !d.mPtr || !d->_hx_getInterface(interfaceId))
      hx::BadCast();
   return inObj;
}

}


#endif

