#include <hxcpp.h>
#include <hx/Scriptable.h>
#include "Cppia.h"


namespace hx
{


typedef std::map<std::string, HaxeNativeClass *> ScriptRegisteredMap;
static ScriptRegisteredMap *sScriptRegistered = 0;

typedef std::map<std::string, HaxeNativeInterface *> HaxeNativeIntefaceMap;
static HaxeNativeIntefaceMap *sScriptRegisteredInterface = 0;






// -- HaxeNativeClass ---

HaxeNativeClass::HaxeNativeClass(const std::string &inName, int inDataOffset, ScriptNamedFunction *inFunctions, hx::ScriptableClassFactory inFactory, ScriptFunction inConstruct)
{
   name = inName;
   mDataOffset = inDataOffset;
   functions = inFunctions;
   factory = inFactory;
   construct = inConstruct;
   haxeSuper = 0;
}

void HaxeNativeClass::addVtableEntries( std::vector<std::string> &outVtable)
{
   if (haxeSuper)
      haxeSuper->addVtableEntries(outVtable);

   if (functions)
      for(ScriptNamedFunction *func = functions; func->name; func++)
         outVtable.push_back( func->name );
}

void HaxeNativeClass::dump()
{
   printf("HaxeNativeClass %s\n", name.c_str());

   if (functions)
      for(ScriptNamedFunction *f=functions;f->name;f++)
         printf("  func %s\n", f->name );
   if (haxeSuper)
   {
      printf("super:\n");
      haxeSuper->dump();
   }
}

ScriptFunction HaxeNativeClass::findFunction(const std::string &inName)
{
   if (functions)
      for(ScriptNamedFunction *f=functions;f->name;f++)
         if (inName == f->name)
            return *f;
   if (haxeSuper)
      return haxeSuper->findFunction(inName);

   return ScriptFunction(0,0);
}


HaxeNativeClass *HaxeNativeClass::findClass(const std::string &inName)
{
   return sScriptRegistered ? (*sScriptRegistered)[inName] : 0;
}

HaxeNativeClass *HaxeNativeClass::hxObject()
{
   return sScriptRegistered ? (*sScriptRegistered)["hx.Object"] : 0;
}

void HaxeNativeClass::link()
{
   HaxeNativeClass *hxObj = hxObject();
   for(ScriptRegisteredMap::iterator i = sScriptRegistered->begin(); i!=sScriptRegistered->end();++i)
   {
      if (!i->second)
         continue;

      DBGLOG("== %s ==\n", i->first.c_str());
      if (i->second == hxObj)
      {
         DBGLOG(" super =0\n");
         continue;
      }
      hx::Class cls = hx::Class_obj::Resolve( String(i->first.c_str() ) );
      if (cls.mPtr)
      {
         hx::Class superClass = cls->GetSuper();
         if (superClass.mPtr)
         {
            HaxeNativeClass *superRef = (*sScriptRegistered)[superClass.mPtr->mName.__s];
            if (superRef)
            {
               DBGLOG("registered %s\n",superClass.mPtr->mName.__s);
               i->second->haxeSuper = superRef;
            }
         }
      }

      DBGLOG("haxe super = %p\n", i->second->haxeSuper);
      if (!i->second->haxeSuper)
      {
         DBGLOG("using hx.Object\n");
         i->second->haxeSuper = hxObj;
      }
   }

}


// -- HaxeNativeInterface ---


HaxeNativeInterface::HaxeNativeInterface(const std::string &inName, ScriptNamedFunction *inFunctions,hx::ScriptableInterfaceFactory inFactory,const hx::type_info *inType)
{
   functions = inFunctions;
   factory = inFactory;
   name = inName;
   mType = inType;
}

ScriptFunction HaxeNativeInterface::findFunction(const std::string &inName)
{
   if (functions)
      for(ScriptNamedFunction *f=functions;f->name;f++)
         if (inName == f->name)
            return *f;
   //if (haxeSuper)
   //   return haxeSuper->findFunction(inName);

   return ScriptFunction(0,0);
}



void ScriptableRegisterClass( String inName, int inDataOffset, ScriptNamedFunction *inFunctions, hx::ScriptableClassFactory inFactory, hx::ScriptFunction inConstruct)
{
   DBGLOG("ScriptableRegisterClass %s\n", inName.__s);
   if (!sScriptRegistered)
      sScriptRegistered = new ScriptRegisteredMap();
   HaxeNativeClass *registered = new HaxeNativeClass(inName.__s,inDataOffset, inFunctions,inFactory, inConstruct);
   (*sScriptRegistered)[inName.__s] = registered;
   //printf("Registering %s -> %p\n",inName.__s,(*sScriptRegistered)[inName.__s]);
}


void ScriptableRegisterInterface( String inName,
                                  ScriptNamedFunction *inFunctions,
                                  const hx::type_info *inType,
                                  hx::ScriptableInterfaceFactory inFactory )
{
   DBGLOG("ScriptableInterfaceFactory %s\n",inName.__s);
   if (!sScriptRegisteredInterface)
      sScriptRegisteredInterface = new HaxeNativeIntefaceMap();
   HaxeNativeInterface *registered = new HaxeNativeInterface(inName.__s, inFunctions, inFactory,inType);
   (*sScriptRegisteredInterface)[inName.__s] = registered;
}


HaxeNativeInterface *HaxeNativeInterface::findInterface(const std::string &inName)
{
   return sScriptRegisteredInterface ? (*sScriptRegisteredInterface)[inName] : 0;
}




} // end namespace hx
