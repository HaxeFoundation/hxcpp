#include <stdio.h>

#include <hx/Native.h>
#include <api/HaxeObject.h>
#include <api/HaxeApi.h>


struct MyStruct
{
   hx::Ref<api::HaxeObject *> haxeRef;
};

extern void __hxcpp_collect(bool inMajor);

bool checkAttachNone(const char *where)
{
   if (hx::GcGetThreadAttachedCount()!=0)
   {
      printf("Bad attach count - something attached: %s\n",where);
      return false;
   }
   return true;
}

int main(int argc, char **argv)
{
   MyStruct *myStruct = new MyStruct();


   if (!checkAttachNone("before creation"))
      return -1;

   {
      hx::NativeAttach autoAttach;
      const char *err = hx::Init(false);
      if (err)
      {
         printf("Could not initialize library: %s\n", err);
         return -1;
      }
   }


   if (!checkAttachNone("after init"))
      return -1;

   {
   hx::NativeAttach autoAttach;

   api::HaxeObject *obj = api::HaxeApi::createBase();
   obj->setName("child");

   myStruct->haxeRef = obj;
   obj->setName("Name");

   api::HaxeObject *child = obj->createChild();
   }


   if (!checkAttachNone("after interaction"))
      return -1;

   {
      hx::NativeAttach autoAttach;
      __hxcpp_collect(true);
   }

   if (!checkAttachNone("after collect"))
      return -1;

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

   if (!checkAttachNone("after clear"))
      return -1;

   printf("all good\n");
   return 0;
}

