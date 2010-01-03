#include <hxcpp.h>
#include <hxMath.h>

namespace hx
{

void Boot()
{
	hx::GCInit();
   //__hxcpp_enable(false);

   Object::__boot();
	Dynamic::__boot();
	Class_obj::__boot();
	String::__boot();
	Anon_obj::__boot();
	ArrayBase::__boot();
	EnumBase_obj::__boot();
   Math_obj::__boot();
}

}


