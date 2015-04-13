#ifndef HX_INTERFACE_H
#define HX_INTERFACE_H

namespace hx
{

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
	Dynamic __Field(const ::String &, hx::PropertyAccess inCallProp);
	Dynamic __IField(int);
	Dynamic __SetField(const ::String &,const Dynamic &, hx::PropertyAccess inCallProp);
	void __SetThis(Dynamic);
	void __GetFields(Array< ::String> &);
	hx::Class __GetClass() const;
	int __Compare(const hx::Object *) const;

   /* No need for enum options - not in interfaces */
   /* No need for array options - not in interfaces */
   /* No need for function options - not in interfaces */
};

}

#endif

