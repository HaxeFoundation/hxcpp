#include <hxcpp.h>
#include "Cppia.h"
#include "CppiaStream.h"
#include <stdlib.h>


namespace hx
{


static int sScriptId = 0;


const char **sgNativeNameSlots = 0;
int sgNativeNameSlotCount = 0;

Array<Dynamic> gAllCppiaModules;

std::vector<hx::Resource> scriptResources;



// --- CppiaModule ----

CppiaModule::CppiaModule()
{
   main = 0;
   layout = 0;
   creatingClass = 0;
   creatingFunction = 0;
   scriptId = ++sScriptId;
   strings = Array_obj<String>::__new(0,0);
   if (sgNativeNameSlotCount>0)
      for(int i=2;i<sgNativeNameSlotCount;i++)
         interfaceSlots[sgNativeNameSlots[i]] = i;

}

void CppiaModule::setDebug(CppiaExpr *outExpr, int inFileId, int inLine)
{
   #ifdef HXCPP_DEBUGGER
   if (inFileId)
      allFileIds.insert(inFileId);
   #endif
   outExpr->className = creatingClass;
   outExpr->functionName = creatingFunction;
   //outExpr->filename = cStrings[inFileId].c_str();
   outExpr->filename = strings[inFileId].utf8_str();
   outExpr->line = inLine;
}


// --- CppiaModule -------------------------

CppiaModule::~CppiaModule()
{
   delete main;
   for(int i=0;i<classes.size();i++)
      delete classes[i];
}

void CppiaModule::link()
{
   DBGLOG("Resolve registered - super\n");
   HaxeNativeClass::link();
   
   DBGLOG("Resolve typeIds\n");
   for(int t=0;t<types.size();t++)
      types[t]->link(*this);

   DBGLOG("Resolve inherited atributes\n");
   for(int i=0;i<classes.size();i++)
   {
      classes[i]->linkTypes();
   }

   for(int i=0;i<classes.size();i++)
   {
      linkingClass = classes[i];
      classes[i]->link();
   }
   linkingClass = 0;

   if (main)
      main = (ScriptCallable *)main->link(*this);
}

#ifdef CPPIA_JIT
void CppiaModule::compile()
{
   for(int i=0;i<classes.size();i++)
      classes[i]->compile();

   if (main)
      main->compile();
}
#endif

void addScriptableClass(String inName);
void addScriptableFile(String inName);

void CppiaModule::registerDebugger()
{
   #ifdef HXCPP_DEBUGGER
   for(int i=0;i<classes.size();i++)
   {
      String scriptable(classes[i]->name.c_str());
      addScriptableClass( scriptable.makePermanent().utf8_str() );
   }

   for(hx::UnorderedSet<int>::const_iterator i = allFileIds.begin(); i!=allFileIds.end(); ++i)
      addScriptableFile(strings[*i]);

   #endif
}

CppiaClassInfo *CppiaModule::findClass( ::String inName)
{

   std::string stdName(inName.utf8_str());
   for(int i=0;i<classes.size();i++)
      if (classes[i]->name == stdName)
         return classes[i];
   return 0;
}


void CppiaModule::mark(hx::MarkContext *__inCtx)
{
   DBGLOG(" --- MARK --- \n");
   HX_MARK_MEMBER(strings);
   for(int i=0;i<types.size();i++)
   {
      if (types[i]) /* May be partially constructed */
         types[i]->mark(__inCtx);
   }
   for(int i=0;i<markable.size();i++)
   {
      markable[i]->mark(__inCtx);
   }
   for(int i=0;i<classes.size();i++)
      if (classes[i])
      {
         classes[i]->mark(__inCtx);
      }
}

#ifdef HXCPP_VISIT_ALLOCS
void CppiaModule::visit(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(strings);
   for(int i=0;i<types.size();i++)
      types[i]->visit(__inCtx);
   for(int i=0;i<markable.size();i++)
      markable[i]->visit(__inCtx);
   for(int i=0;i<classes.size();i++)
      if (classes[i])
         classes[i]->visit(__inCtx);
}
#endif


void cppiaClassInit(CppiaClassInfo *inClass, CppiaCtx *ctx, int inPhase);



void CppiaModule::boot(CppiaCtx *ctx)
{
   // boot (statics)
   for(int i=0;i<classes.size();i++)
      cppiaClassInit(classes[i],ctx,0);
   // run __init__
   for(int i=0;i<classes.size();i++)
      cppiaClassInit(classes[i],ctx,1);
}

int CppiaModule::getInterfaceSlot(const std::string &inName)
{
   InterfaceSlots::iterator it = interfaceSlots.find(inName);
   if (it==interfaceSlots.end())
   {
      #if (HXCPP_API_LEVEL >= 330)
      int result = interfaceSlots.size()+1;
      #else
      int result = interfaceSlots.size()+2;
      #endif
      interfaceSlots[inName] = result;
      return result;
   }
   return it->second;
}


int CppiaModule::findInterfaceSlot(const std::string &inName)
{
   InterfaceSlots::iterator it = interfaceSlots.find(inName);
   if (it==interfaceSlots.end())
      return -1;
   return it->second;
}




void CppiaModule::where(CppiaExpr *e)
{
   if (linkingClass && layout)
      CPPIALOG(" in %s::%p\n", linkingClass->name.c_str(), layout);
   CPPIALOG("   %s at %s:%d %s\n", e->getName(), e->filename, e->line, e->functionName);
}



void ScriptableRegisterNameSlots(const char *inNames[], int inLength)
{
   sgNativeNameSlots = inNames;
   sgNativeNameSlotCount = inLength;
}


// --- CppiaConst -------------------------------------------

CppiaConst::CppiaConst() : type(cNull), ival(0), dval(0) { }

void CppiaConst::fromStream(CppiaStream &stream)
{
   std::string tok = stream.getToken();
   if (tok[0]=='i')
   {
      type = cInt;

      dval = ival = stream.getInt();
   }
   else if (tok=="true")
   {
      type = cInt;
      dval = ival = 1;
   }
   else if (tok=="false")
   {
      type = cInt;
      dval = ival = 0;
   }
   else if (tok[0]=='f')
   {
      type = cFloat;
      int strIndex = stream.getInt();
      String val = stream.module->strings[strIndex];
      dval = atof(val.out_str());
      ival = dval;
   }
   else if (tok[0]=='s')
   {
      type = cString;
      ival = stream.getInt();
   }
   else if (tok=="NULL")
      type = cNull;
   else if (tok=="THIS")
      type = cThis;
   else if (tok=="SUPER")
      type = cSuper;
   else
      throw "unknown const value";
}




// --- CppiaLoadedModule -------------------------------------------





// Cppia Object - manage
class CppiaObject : public hx::CppiaLoadedModule_obj
{
public:
   CppiaModule *cppia;
   bool        booted;

   CppiaObject(CppiaModule *inModule)
   {
      cppia = inModule;
      GCSetFinalizer( this, onRelease );
   }
   static void onRelease(hx::Object *inObj)
   {
      delete ((CppiaObject *)inObj)->cppia;
   }
   void __Mark(hx::MarkContext *ctx) { cppia->mark(ctx); }
#ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *ctx) { cppia->visit(ctx); }
#endif


   void boot()
   {
      if (booted)
         return;

      booted = true;
      try
      {
         DBGLOG("--- Boot --------------------------------------------\n");
         CppiaCtx *ctx = CppiaCtx::getCurrent();
         cppia->boot(ctx);
      }
      catch(const char *errorString)
      {
         String error(errorString);
         hx::Throw(error);
      }
   }


   void run()
   {
      if (!booted)
         boot();
      if (cppia->main)
      {
         try
         {
            //__hxcpp_enable(false);
            DBGLOG("--- Run --------------------------------------------\n");
            CppiaCtx *ctx = CppiaCtx::getCurrent();
            ctx->runVoid(cppia->main);
            if (ctx->exception)
            {
               hx::Object *e = ctx->exception;
               ctx->exception = 0;
               hx::Throw( Dynamic(e) );
            }
         }
         catch(const char *errorString)
         {
            String error(errorString);
            hx::Throw(error);
         }
      }
   }

   ::hx::Class resolveClass( ::String inName)
   {
      CppiaClassInfo *info = cppia->findClass(inName);
      if (info)
         return info->getClass();
      return null();
   }

};




CppiaLoadedModule LoadCppia(const unsigned char *inData, int inDataLength)
{
   if (!gAllCppiaModules.mPtr)
   {
      gAllCppiaModules = Array_obj<Dynamic>::__new();
      GCAddRoot( (hx::Object **)&gAllCppiaModules.mPtr );
   }

   CppiaModule   *cppiaPtr = new CppiaModule();
   CppiaLoadedModule loadedModule = new CppiaObject(cppiaPtr);
   gAllCppiaModules->push(loadedModule);


   CppiaModule   &cppia = *cppiaPtr;
   CppiaStream stream(cppiaPtr,inData, inDataLength);

   String error;
   try
   {
      std::string tok = stream.getAsciiToken();
      if (tok!="CPPIA" && tok!="CPPIB")
         throw "Bad magic";

      stream.setBinary(tok=="CPPIB");

      int stringCount = stream.getAsciiInt();
      for(int s=0;s<stringCount;s++)
         cppia.strings[s] = stream.readString();

      int typeCount = stream.getAsciiInt();
      cppia.types.resize(typeCount);
      DBGLOG("Type count : %d\n", typeCount);
      for(int t=0;t<typeCount;t++)
         cppia.types[t] = new TypeData(stream.readString());

      int classCount = stream.getAsciiInt();
      DBGLOG("Class count : %d\n", classCount);

      if (stream.binary)
      {
         int newLine = stream.getByte();
         if (newLine!='\n')
            throw "Missing new-line after class count";
      }

      cppia.classes.reserve(classCount);
      for(int c=0;c<classCount;c++)
      {
         CppiaClassInfo *info = new CppiaClassInfo(cppia);
         if (info->load(stream))
            cppia.classes.push_back(info);
      }

      tok = stream.getToken();
      if (tok=="MAIN")
      {
         DBGLOG("Main...\n");
         cppia.main = new ScriptCallable(createCppiaExpr(stream));
         cppia.main->className = "cppia";
         cppia.main->functionName = "__cppia_main";
      }
      else if (tok!="NOMAIN")
         throw "no main specified";

      tok = stream.getToken();
      if (tok=="RESOURCES")
      {
         int count = stream.getInt( );
         scriptResources.resize(count+1);
         for(int r=0;r<count;r++)
         {
            tok = stream.getToken();
            if (tok!="RESO")
               throw "no reso tag";

            scriptResources[r].mName = cppia.strings[stream.getInt()];
            scriptResources[r].mDataLength = stream.getInt();
         }
         if (!stream.binary)
            stream.skipChar();

         for(int r=0;r<count;r++)
         {
            int len = scriptResources[r].mDataLength;
            unsigned char *buffer = (unsigned char *)malloc(len+5);
            *(unsigned int *)buffer = HX_GC_CONST_ALLOC_BIT;
            buffer[len+5-1] = '\0';
            stream.readBytes(buffer+4, len);
            #ifdef HX_SMART_STRINGS_1
            unsigned char *p = (unsigned char *)buffer+4;
            unsigned char *end = p + len;
            while(!hasBig && p<end)
               if (*p++>127)
               {
                  *(unsigned int *)buffer |= HX_GC_STRING_CHAR16_T;
                  break;
               }
            #endif
            scriptResources[r].mData = buffer + 4;
         }
         scriptResources[count].mDataLength = 0;
         scriptResources[count].mData = 0;
         scriptResources[count].mName = String();
         
         RegisterResources(&scriptResources[0]);
      }
      else
         throw "no resources tag";


   }
   catch(const char *errorString)
   {
      error = HX_CSTRING("Error reading file ") + String(errorString) + 
                HX_CSTRING(", line ") + String(stream.line) + HX_CSTRING(", char ") + 
                   String(stream.pos);
   }

   if (!error.raw_ptr())
      try
      {
         DBGLOG("Link...\n");
         cppia.link();
      }
      catch(const char *errorString)
      {
         error = String(errorString);
      }

   if (gEnableJit)
   {
      #ifdef CPPIA_JIT
      if (!error.raw_ptr())
         try
         {
            DBGLOG("Compile...\n");
            cppia.compile();
         }
         catch(const char *errorString)
         {
            error = String(errorString);
         }
      #endif
   }

   if (error.raw_ptr())
      hx::Throw(error);

   cppia.registerDebugger();

   return loadedModule;
}





Array<String > gAllClasses;
Array<String > gAllFiles;

void addScriptableClass(String inName)
{
   #ifdef HXCPP_DEBUGGER
   if (!gAllClasses.mPtr)
   {
      gAllClasses = Array_obj<Dynamic>::__new();
      GCAddRoot( (hx::Object **)&gAllClasses.mPtr );
   }
   if (gAllClasses->indexOf(inName)<0)
      gAllClasses->push(inName);
   #endif
}


void addScriptableFile(String inName)
{
   #ifdef HXCPP_DEBUGGER
   if (!gAllFiles.mPtr)
   {
      gAllFiles = Array_obj<Dynamic>::__new();
      GCAddRoot( (hx::Object **)&gAllFiles.mPtr );
   }
   if (gAllFiles->indexOf(inName)<0)
      gAllFiles->push(inName);
   #endif
}








} // end namespace hx




#ifdef HXCPP_STACK_SCRIPTABLE

void __hxcpp_dbg_getScriptableVariables(hx::StackFrame *inFrame, ::Array<Dynamic> outNames)
{
   hx::ScriptCallable *callable = inFrame->position->scriptCallable;
   if (callable)
      callable->getScriptableVariables((unsigned char *)inFrame, outNames);
}

bool __hxcpp_dbg_getScriptableValue(hx::StackFrame *inFrame, String inName, ::Dynamic &outValue)
{
   hx::ScriptCallable *callable = inFrame->position->scriptCallable;
   if (callable)
      return callable->getScriptableValue((unsigned char *)inFrame, inName, outValue);
   return false;
}


bool __hxcpp_dbg_setScriptableValue(hx::StackFrame *inFrame, String inName, ::Dynamic inValue)
{
   hx::ScriptCallable *callable = inFrame->position->scriptCallable;
   if (callable)
      return callable->setScriptableValue((unsigned char *)inFrame, inName, inValue);
   return false;
}

#endif





#ifdef HXCPP_DEBUGGER
void __hxcpp_dbg_getScriptableFiles( Array< ::String> ioPaths )
{
   Array<String> merge = hx::gAllFiles;
   if (merge.mPtr)
      for(int i=0;i< merge->length; i++)
      {
         if (ioPaths->indexOf( merge[i] ) < 0)
            ioPaths->push( merge[i] );
      }
}

void __hxcpp_dbg_getScriptableFilesFullPath( Array< ::String> ioPaths )
{
   __hxcpp_dbg_getScriptableFiles( ioPaths );
}

void __hxcpp_dbg_getScriptableClasses( Array< ::String> ioClasses )
{
   Array<String> merge = hx::gAllClasses;
   if (merge.mPtr)
      for(int i=0;i< merge->length; i++)
      {
         if (ioClasses->indexOf( merge[i] ) < 0)
            ioClasses->push( merge[i] );
      }
}

#endif



::hx::CppiaLoadedModule __scriptable_cppia_from_string(String inCode)
{
   const unsigned char *code = (const unsigned char *)inCode.raw_ptr();
   int length = inCode.length;
   #ifdef HX_SMART_STRINGS
   if (inCode.isUTF16Encoded())
   {
      code = (const unsigned char *)inCode.utf8_str();
      length = strlen( (const char *)code );
   }
   #endif

   return hx::LoadCppia(code,length);
}


::hx::CppiaLoadedModule __scriptable_cppia_from_data(Array<unsigned char> inCode)
{
   return hx::LoadCppia( (unsigned char *)inCode->GetBase() ,inCode->length);
}


void __scriptable_load_cppia(String inCode)
{
   const unsigned char *code = (const unsigned char *)inCode.raw_ptr();
   int length = inCode.length;
   #ifdef HX_SMART_STRINGS
   if (inCode.isUTF16Encoded())
   {
      code = (const unsigned char *)inCode.utf8_str();
      length = strlen( (const char *)code );
   }
   #endif

   ::hx::CppiaLoadedModule module = hx::LoadCppia(code,length);

   module->boot();
   module->run();
}

