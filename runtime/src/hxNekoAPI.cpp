#include <neko.h>

#include <map>
#include <vector>
#include <gc.h>


vkind k_int32 = vtAbstractBase;
vkind k_hash = (int)vtAbstractBase + 1;
static int sgKinds = (int)vtAbstractBase + 2;


int hxcpp_alloc_kind()
{
   return ++sgKinds;
}

void hxcpp_alloc_field( value obj, field f, value v )
{
   obj->__SetField(__hxcpp_field_from_id(f),v);
}



void hxcpp_finalizer(void * obj, void * client_data)
{
   finalizer f = (finalizer)client_data;
   if (f)
      f( (hxObject *)obj );
}


void hxcpp_val_gc( value v, finalizer f )
{
   hxObject *ptr = v.GetPtr();
   if (ptr)
   {
      GC_register_finalizer(ptr,hxcpp_finalizer,(void *)f,0,0);
   }
}



void hxcpp_fail(const char *inMsg,const char *inFile, int inLine)
{
   fprintf(stderr,"Terminal error %s, File %s, line %d\n", inMsg,inFile,inLine);
   exit(1);
}
