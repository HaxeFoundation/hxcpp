#include <stdio.h>

#include <hx/Native.h>
#include <api/HaxeObject.h>
#include <api/HaxeApi.h>


struct MyStruct
{
   hx::Ref<api::HaxeObject *> haxeRef;
};

extern void __hxcpp_collect(bool inMajor);

int main(int argc, char **argv)
{
   MyStruct *myStruct = new MyStruct();


   const char *err = hx::Init();
   if (err)
   {
      printf("Could not initialize library: %s\n", err);
      return -1;
   }
   else
   {
      hx::NativeAttach autoAttach;

      api::HaxeObject *obj = api::HaxeApi::createBase();
      obj->setName("child");

      myStruct->haxeRef = obj;
      obj->setName("Name");

      api::HaxeObject *child = obj->createChild();
   }

   {
      hx::NativeAttach autoAttach;
      __hxcpp_collect(true);
   }

   {
      hx::NativeAttach autoAttach;
      if (myStruct->haxeRef->getName()!="Name")
      {
         printf("Could not get value back (%s)\n", myStruct->haxeRef->getName().c_str() );
         return -1;
      }
   }

   {
      hx::NativeAttach autoAttach0;
      hx::NativeAttach autoAttach1;
      hx::NativeAttach autoAttach2;
      hx::NativeAttach autoAttach3;
      if (hx::GcGetThreadAttachedCount()!=4)
      {
         printf("Bad attach count\n");
         return -1;
      }
   }
   if (hx::GcGetThreadAttachedCount()!=0)
   {
      printf("Bad clear attach count\n");
      return -1;
   }

   printf("all good\n");
   return 0;
}

