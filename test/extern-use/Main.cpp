#include <stdio.h>

#include <hx/Native.h>
#include <api/HaxeObject.h>
#include <api/HaxeApi.h>

static hx::Ref<api::HaxeObject *> sHaxeRef;

int main(int argc, char **argv)
{
   const char *err = hx::Init();
   if (err)
      printf("Could not initialize library: %s\n", err);
   else
   {
      hx::NativeAttach autoAttach;

      api::HaxeObject *obj = api::HaxeApi::createBase();
      obj->setName("child");
      sHaxeRef = obj;
      api::HaxeObject *child = obj->createChild();
      child->setName("Child");
      printf("Got %s\n", child->getName().c_str() );
   }
   return 0;
}

