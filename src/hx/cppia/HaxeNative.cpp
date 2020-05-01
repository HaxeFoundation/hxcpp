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

HaxeNativeClass::HaxeNativeClass(const std::string &inName, int inDataOffset, ScriptNamedFunction *inFunctions, hx::ScriptableClassFactory inFactory, ScriptNamedFunction inConstruct)
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
         if (!func->isStatic)
            outVtable.push_back( func->name );
}

void HaxeNativeClass::dump()
{
   printf("HaxeNativeClass %s\n", name.c_str());

   if (functions)
      for(ScriptNamedFunction *f=functions;f->name;f++)
         printf("  %s func %s\n", f->isStatic ? "static" : "virtual", f->name );
   if (haxeSuper)
   {
      printf("super:\n");
      haxeSuper->dump();
   }
}

ScriptNamedFunction HaxeNativeClass::findFunction(const std::string &inName)
{
   if (functions)
      for(ScriptNamedFunction *f=functions;f->name;f++)
         if (inName == f->name && !f->isStatic)
            return *f;
   if (haxeSuper)
      return haxeSuper->findFunction(inName);

   return ScriptNamedFunction(0,0,0,0,0);
}


ScriptNamedFunction HaxeNativeClass::findStaticFunction(String inName)
{
   if (functions)
      for(ScriptNamedFunction *f=functions;f->name;f++)
         if ( !strcmp(inName.utf8_str(),f->name) && f->isStatic)
            return *f;

   return ScriptNamedFunction(0,0,0,0,0);
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
            HaxeNativeClass *superRef = (*sScriptRegistered)[superClass.mPtr->mName.utf8_str()];
            if (superRef)
            {
               DBGLOG("registered %s\n",superClass.mPtr->mName.utf8_str());
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
   DBGLOG("ScriptableRegisterClass %s\n", inName.out_str());
   if (!sScriptRegistered)
      sScriptRegistered = new ScriptRegisteredMap();
   HaxeNativeClass *registered = new HaxeNativeClass(inName.utf8_str(),inDataOffset, inFunctions,inFactory, inConstruct);
   (*sScriptRegistered)[inName.utf8_str()] = registered;
   //printf("Registering %s -> %p\n",inName.out_str(),(*sScriptRegistered)[inName.utf8_str()]);
}


#if (HXCPP_API_LEVEL >= 330)

HaxeNativeInterface::HaxeNativeInterface(const std::string &inName,
                                         ScriptNamedFunction *inFunctions,
                                         void *inScriptTable )
{
   name = inName;
   functions = inFunctions;
   scriptTable = inScriptTable;
}

void ScriptableRegisterInterface( String inName,
                                  ScriptNamedFunction *inFunctions,
                                  void *inScriptTable)
{
   DBGLOG("ScriptableInterfaceFactory %s\n",inName.out_str());
   if (!sScriptRegisteredInterface)
      sScriptRegisteredInterface = new HaxeNativeIntefaceMap();

   HaxeNativeInterface *registered = new HaxeNativeInterface(inName.utf8_str(), inFunctions, inScriptTable);
   (*sScriptRegisteredInterface)[inName.utf8_str()] = registered;
}

#else


HaxeNativeInterface::HaxeNativeInterface(const std::string &inName,
                                         ScriptNamedFunction *inFunctions,
                                         hx::ScriptableInterfaceFactory inFactory,
                                         const hx::type_info *inType)
{
   functions = inFunctions;
   factory = inFactory;
   name = inName;
   mType = inType;
}

void ScriptableRegisterInterface( String inName,
                                  ScriptNamedFunction *inFunctions,
                                  const hx::type_info *inType,
                                  hx::ScriptableInterfaceFactory inFactory )
{
   DBGLOG("ScriptableInterfaceFactory %s\n",inName.out_str());
   if (!sScriptRegisteredInterface)
      sScriptRegisteredInterface = new HaxeNativeIntefaceMap();

   HaxeNativeInterface *registered = new HaxeNativeInterface(inName.utf8_str(), inFunctions, inFactory,inType);
   (*sScriptRegisteredInterface)[inName.utf8_str()] = registered;
}

#endif



HaxeNativeInterface *HaxeNativeInterface::findInterface(const std::string &inName)
{
   return sScriptRegisteredInterface ? (*sScriptRegisteredInterface)[inName] : 0;
}




} // end namespace hx
