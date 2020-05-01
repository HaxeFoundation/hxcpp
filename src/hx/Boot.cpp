#include <hxcpp.h>
#include <hxMath.h>

#ifdef HX_WINRT
#include<Roapi.h>
#endif
namespace hx
{

void Boot()
{
   //__hxcpp_enable(false);
   #ifdef HX_WINRT
   HRESULT hr = ::RoInitialize(  RO_INIT_MULTITHREADED );
   #endif

	#ifdef GPH
	 setvbuf( stdout , 0 , _IONBF , 0 );
	 setvbuf( stderr , 0 , _IONBF , 0 );
	#endif

   __hxcpp_stdlibs_boot();
   Object::__boot();
	Dynamic::__boot();
	hx::Class_obj::__boot();
	String::__boot();
	Anon_obj::__boot();
	ArrayBase::__boot();
	EnumBase_obj::__boot();
   Math_obj::__boot();
}

}


