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

#if (HXCPP_API_LEVEL >= 330)
template<typename T>
inline T interface_check(T inObj,int interfaceId)
{
   Dynamic d(inObj);
   if ( !d.mPtr || !d->_hx_getInterface(interfaceId))
      hx::BadCast();
   return inObj;
}
#endif


#if (HXCPP_API_LEVEL < 330)
class HXCPP_EXTERN_CLASS_ATTRIBUTES Interface : public hx::Object
{
public:
   // The following functions make use of : hx::Object *__GetRealObject();

	void __Mark(hx::MarkContext *__inCtx);
   hx::Object *__ToInterface(const hx::type_info &);
	int __GetType() const;
	void *__GetHandle() const;
	hx::FieldRef __FieldRef(const ::String &);
	::String __ToString() const;
	int __ToInt() const;
	double __ToDouble() const;
	const char * __CStr() const;
	::String toString();
	bool __HasField(const ::String &);
   hx::Val __Field(const ::String &, hx::PropertyAccess inCallProp);
	Dynamic __IField(int);
   hx::Val __SetField(const ::String &,const hx::Val &, hx::PropertyAccess inCallProp);
	void __SetThis(Dynamic);
	void __GetFields(Array< ::String> &);
	hx::Class __GetClass() const;
	int __Compare(const hx::Object *) const;

   /* No need for enum options - not in interfaces */
   /* No need for array options - not in interfaces */
   /* No need for function options - not in interfaces */
};
#endif

}


#endif

