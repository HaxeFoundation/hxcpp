#include <hxcpp.h>
#include "Cppia.h"
#include "CppiaStream.h"

namespace hx
{



class CppiaEnumBase : public EnumBase_obj
{
public:
   #if (HXCPP_API_LEVEL<330)
   CppiaClassInfo *classInfo; 
   #endif

   CppiaEnumBase(CppiaClassInfo *inInfo) { classInfo = inInfo; }

   ::hx::ObjectPtr<hx::Class_obj > __GetClass() const;
	::String GetEnumName( ) const;
	::String __ToString() const;
};




struct CppiaEnumConstructor
{
   struct Arg
   {
      Arg(int inNameId, int inTypeId) : nameId(inNameId), typeId(inTypeId) { }
      int nameId;
      int typeId;
   };
 
   std::vector<Arg> args;
   CppiaClassInfo   *classInfo;
   int              nameId;
   Dynamic          value;
   int              index;
   String           name;

   CppiaEnumConstructor(CppiaModule &inModule, CppiaStream &inStream, CppiaClassInfo *inClassInfo)
   {
      classInfo = inClassInfo;
      nameId = inStream.getInt();
      index = -1;
      name = String();
      int argCount = inStream.getInt();
      for(int a=0;a<argCount;a++)
      {
         int nameId = inStream.getInt();
         int typeId = inStream.getInt();
         args.push_back( Arg(nameId,typeId) );
      }
         
   }
   hx::Object *create( Array<Dynamic> inArgs )
   {
      bool ok = inArgs.mPtr ? (inArgs->length==args.size()) : args.size()==0;
      if (!ok)
         throw Dynamic(HX_CSTRING("Bad enum arg count"));
      if (args.size()==0)
         return value.mPtr;
      #if (HXCPP_API_LEVEL >= 330)
      EnumBase_obj *result = new ((int)args.size()*sizeof(cpp::Variant)) CppiaEnumBase(classInfo);
      result->_hx_setIdentity(name, index, args.size());
      for(int i=0;i<args.size();i++)
         result->_hx_init( i, inArgs[i] );
      #else
      EnumBase_obj *result = new CppiaEnumBase(classInfo);
      result->__Set(name, index, inArgs);
      #endif
      return result;
   }

   #ifdef CPPIA_JIT
   static EnumBase_obj * SLJIT_CALL createEnumBase( CppiaEnumConstructor *enumCtor)
   {
      EnumBase_obj *result = new ((int)enumCtor->args.size()*sizeof(cpp::Variant)) CppiaEnumBase(enumCtor->classInfo);
      result->_hx_setIdentity(enumCtor->name, enumCtor->index, enumCtor->args.size());
      return result;
   }

   cpp::Variant::Type getVariantType(ExprType inType)
   {
      switch(inType)
      {
         case etVoid: return cpp::Variant::typeObject;
         case etNull: return cpp::Variant::typeObject;
         case etObject: return cpp::Variant::typeObject;
         case etString: return cpp::Variant::typeString;
         case etFloat: return cpp::Variant::typeDouble;
         case etInt: return cpp::Variant::typeInt;
      }
      return cpp::Variant::typeObject;
   }

   void genCode(CppiaCompiler *compiler, Expressions &inArgs, const JitVal &inDest, ExprType destType)
   {
      bool ok = args.size() && args.size()==inArgs.size();
      if (!ok)
         printf("Bad enum arg count\n");

      #if (HXCPP_API_LEVEL >= 330)
      compiler->callNative( (void *)createEnumBase, (void *)this );
      JitTemp result(compiler,jtPointer);
      compiler->move(result, sJitReturnReg);

      int offset = sizeof(EnumBase_obj);
      for(int i=0;i<args.size();i++)
      {
         ExprType type = inArgs[i]->getType();
         JitTemp val(compiler,getJitType(type));
         inArgs[i]->genCode(compiler, val, inArgs[i]->getType());
         compiler->move( sJitReturnReg.as(jtPointer), result );
         if (type==etString)
         {
            compiler->move( sJitReturnReg.star(jtInt,offset + offsetof(cpp::Variant,valStringLen)), val.as(jtInt) );
            compiler->move( sJitReturnReg.star(jtPointer,offset), val.as(jtPointer) + StringOffset::Ptr);
         }
         else
         {
            compiler->move( sJitReturnReg.star(type,offset), val );
         }
         compiler->move( sJitReturnReg.star(jtInt,offset + offsetof(cpp::Variant,type)), (int)getVariantType(type) );

         offset += sizeof(cpp::Variant);
      }

      if (destType==etObject)
         compiler->move(inDest, sJitReturnReg.as(jtPointer));
      #endif
   }
   #endif

};







class EnumConstructorClosure : public hx::Object
{
public:
   CppiaEnumConstructor &constructor;

   EnumConstructorClosure(CppiaEnumConstructor &c) : constructor(c)
   {
   }

   Dynamic __Run(const Array<Dynamic> &inArgs)
   {
      return constructor.create(inArgs);
   }

   int __GetType() const { return vtFunction; }
   int __ArgCount() const { return constructor.args.size(); }

   String toString() { return HX_CSTRING("#function(") + constructor.name + HX_CSTRING(")"); }
};



hx::Object *createEnumClosure(CppiaEnumConstructor &inContructor)
{
   return new EnumConstructorClosure(inContructor);
}


#ifdef CPPIA_JIT
void SLJIT_CALL createEnum(hx::Class_obj *inClass, String *inName, int inArgs)
{
   StackContext *ctx = StackContext::getCurrent();

   // ctx.pointer points to end-of-args
   unsigned char *oldPointer = ctx->pointer;
   hx::Object **base = ((hx::Object **)(ctx->frame) );
   Array<Dynamic> args = Array_obj<Dynamic>::__new(inArgs);

   TRY_NATIVE
      for(int i=0;i<inArgs;i++)
         args[i] = Dynamic(base[i]);
      base[0] = inClass->ConstructEnum(*inName, args).mPtr;
  
   CATCH_NATIVE
   ctx->pointer = oldPointer;
}
#endif


struct EnumField : public CppiaDynamicExpr
{
   int                  enumId;
   int                  fieldId;
   CppiaEnumConstructor *value;
   Expressions          args;

   // Mark class?
   String               enumName;
   hx::Class            enumClass;
   Dynamic              enumValue;

   EnumField(CppiaStream &stream,bool inWithArgs)
   {
      enumId = stream.getInt();
      fieldId = stream.getInt();
      value= 0;
      if (inWithArgs)
      {
         int argCount = stream.getInt();
         for(int a=0;a<argCount;a++)
            args.push_back( createCppiaExpr(stream) );
      }
   }

   const char *getName() { return "EnumField"; }
   CppiaExpr *link(CppiaModule &inModule)
   {
      TypeData *type = inModule.types[enumId];
      if (type->cppiaClass)
      {
         if (!type->cppiaClass->isEnum)
            throw "Field of non-enum";
         value = type->cppiaClass->findEnum(fieldId);
      }
      else
      {
         enumClass = hx::Class_obj::Resolve(type->name);
         if (!enumClass.mPtr)
         {
            printf("Could not find enum %s\n", type->name.out_str() );
            throw "Bad enum";
         }
         enumName = inModule.strings[fieldId];
         inModule.markable.push_back(this);
      }

      LinkExpressions(args,inModule);
      return this;
   }

   hx::Object *getValue()
   {
      if (value)
         return value->value.mPtr;
      if (!enumValue.mPtr)
      {
         enumValue = enumClass->ConstructEnum(enumName,null());
      }
      return enumValue.mPtr;
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      int s = args.size();
      if (s==0)
         return getValue();

      Array<Dynamic> dynArgs = Array_obj<Dynamic>::__new(s,s);
      for(int a=0;a<s;a++)
      {
         dynArgs[a] = args[a]->runObject(ctx);
         BCR_CHECK;
      }

      return value ? value->create(dynArgs) : enumClass->ConstructEnum(enumName,dynArgs).mPtr;
   }

   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(enumName);
      HX_MARK_MEMBER(enumClass);
      HX_MARK_MEMBER(enumValue);
   }
#ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(enumName);
      HX_VISIT_MEMBER(enumClass);
      HX_VISIT_MEMBER(enumValue);
   }
#endif


   #ifdef CPPIA_JIT
   void genCode(CppiaCompiler *compiler, const JitVal &inDest, ExprType destType)
   {
      int s = args.size();
      if (s==0)
      {
          // TODO GC pin
         if (destType==etObject)
            compiler->move(inDest, (void *)getValue());
      }
      else if (value)
      {
         value->genCode(compiler, args, inDest, destType);
      }
      else
      {
         {
         AutoFramePos frame(compiler);

         for(int a=0;a<s;a++)
         {
            args[a]->genCode(compiler, JitFramePos(compiler->getCurrentFrameSize(),etObject), etObject);
            compiler->addFrame(etObject);
         }

         }

         compiler->add(sJitCtxFrame, sJitFrame, compiler->getCurrentFrameSize());

         compiler->callNative((void *)createEnum,(void *)enumClass.mPtr,(void *)&enumName,(int)args.size() );

         if (destType!=etVoid && destType!=etNull)
            compiler->convertResult( etObject, inDest, destType );
      }
   }
   #endif

};



CppiaExpr *createEnumField(CppiaStream &stream,bool inWithArgs)
{
   return new EnumField(stream, inWithArgs);
}




int getScriptId(hx::Class inClass);

void  linkCppiaClass(hx::Class_obj *inClass, CppiaModule &cppia, String inName);


// Class Info...




CppiaClassInfo::CppiaClassInfo(CppiaModule &inCppia) : cppia(inCppia)
{
   isLinked = false;
   haxeBase = 0;
   extraData = 0;
   containsPointers = false;
   classSize = 0;
   newFunc = 0;
   initExpr = 0;
   enumMeta = 0;
   isInterface = false;
   interfaceSlotSize = 0;
   superType = 0;
   typeId = 0;
   vtable = 0;
   type = 0;
   mClass.mPtr = 0;
   dynamicMapOffset = 0;
   haxeBaseVTable = 0;
}

/*
class CppiaClass *getCppiaClass()
{
   return (class CppiaClass *)mClass.mPtr;
}
*/

hx::Object *CppiaClassInfo::createInstance(CppiaCtx *ctx,Expressions &inArgs, bool inCallNew)
{
   hx::Object *obj = haxeBase->factory(vtable,extraData);

   createDynamicFunctions(obj);

   if (newFunc && inCallNew)
      runFunExpr(ctx, newFunc->funExpr, obj, inArgs );

   return obj;
}

hx::Object *CppiaClassInfo::createInstance(CppiaCtx *ctx,Array<Dynamic> &inArgs)
{
   hx::Object *obj = haxeBase->factory(vtable,extraData);

   createDynamicFunctions(obj);

   if (newFunc)
      runFunExprDynamicVoid(ctx, newFunc->funExpr, obj, inArgs );

   return obj;
}


void *CppiaClassInfo::getHaxeBaseVTable()
{
   if (!haxeBase || haxeBaseVTable)
      return haxeBaseVTable;

   hx::Object *temp = haxeBase->factory(vtable,extraData);
   haxeBaseVTable = *(void **)temp;
   if (!containsPointers && ( ((unsigned int *)temp)[-1] & IMMIX_ALLOC_IS_CONTAINER) )
      containsPointers = true;

   return haxeBaseVTable;
}

int CppiaClassInfo::getScriptVTableOffset()
{
   return haxeBase ? (int)(haxeBase->mDataOffset - sizeof(void *)) : sizeof(hx::Object);
}


bool CppiaClassInfo::isNativeProperty(const String &inString)
{
   return nativeProperties.find(inString) != nativeProperties.end();
}


int CppiaClassInfo::__GetType() { return isEnum ? vtEnum : vtClass; }

int CppiaClassInfo::getEnumIndex(String inName)
{
   for(int i=0;i<enumConstructors.size();i++)
   {
     if (enumConstructors[i]->name==inName)
        return i;
   }

   throw Dynamic(HX_CSTRING("Bad enum index"));
   return 0;
}

bool CppiaClassInfo::implementsInterface(CppiaClassInfo *inInterface)
{
   for(int i=0;i<implements.size();i++)
      if (implements[i] == inInterface->typeId)
         return true;
   return false;
}


ScriptCallable *CppiaClassInfo::findFunction(bool inStatic,int inId)
{
   Functions &funcs = inStatic ? staticFunctions : memberFunctions;
   for(int i=0;i<funcs.size();i++)
   {
      if (funcs[i]->nameId == inId)
         return funcs[i]->funExpr;
   }
   return 0;
}


CppiaFunction *CppiaClassInfo::findVTableFunction(int inId)
{
   for(int i=0;i<memberFunctions.size();i++)
   {
      if (memberFunctions[i]->nameId == inId)
         return memberFunctions[i];
   }
   return 0;
}

ScriptCallable *CppiaClassInfo::findInterfaceFunction(const std::string &inName)
{
   Functions &funcs = memberFunctions;
   for(int i=0;i<funcs.size();i++)
   {
      if ( cppia.strings[funcs[i]->nameId].utf8_str() == inName)
         return funcs[i]->funExpr;
   }
   return 0;
}

ScriptCallable *CppiaClassInfo::findFunction(bool inStatic, const String &inName)
{
   Functions &funcs = inStatic ? staticFunctions : memberFunctions;
   for(int i=0;i<funcs.size();i++)
   {
      if ( cppia.strings[funcs[i]->nameId] == inName)
         return funcs[i]->funExpr;
   }
   return 0;
}

inline ScriptCallable *CppiaClassInfo::findFunction(FunctionMap &inMap,const String &inName)
{
   FunctionMap::iterator it = inMap.find(inName.utf8_str());
   if (it!=inMap.end())
      return it->second;
   return 0;

}

bool CppiaClassInfo::getField(hx::Object *inThis, String inName, hx::PropertyAccess  inCallProp, Dynamic &outValue)
{
   if (inCallProp==paccDynamic)
      inCallProp = isNativeProperty(inName) ? paccAlways : paccNever;

   if (inCallProp)
   {
      ScriptCallable *getter = findMemberGetter(inName);
      if (getter)
      {
         Array<Dynamic> empty;
         outValue.mPtr = runFunExprDynamic(CppiaCtx::getCurrent(),getter,inThis, empty);
         return true;
      }
   }

   ScriptCallable *closure = findFunction(false,inName);
   if (closure)
   {
      outValue.mPtr = createMemberClosure(inThis,closure);
      return true;
   }

   // Look for dynamic function (variable)
   for(int i=0;i<dynamicFunctions.size();i++)
   {
      if (cppia.strings[ dynamicFunctions[i]->nameId  ]==inName)
      {
         CppiaVar *d = dynamicFunctions[i];

         outValue = d->getValue(inThis);

         return true;
      }
   }

   for(int i=0;i<memberVars.size();i++)
   {
      CppiaVar &var = *memberVars[i];
      if (var.name==inName)
      {
         outValue = var.getValue(inThis);
         return true;
      }
   }

   hx::FieldMap *map = dynamicMapOffset ? (hx::FieldMap *)( (char *)inThis + dynamicMapOffset ) :
                        inThis->__GetFieldMap();
   if (map)
   {
      if (hx::FieldMapGet(map,inName,outValue))
         return true;
   }

   //printf("Get field not found (%s) %s\n", inThis->toString().out_str(),inName.out_str());
   return false;
}

bool CppiaClassInfo::setField(hx::Object *inThis, String inName, Dynamic inValue, hx::PropertyAccess  inCallProp, Dynamic &outValue)
{
   if (inCallProp==paccDynamic)
      inCallProp = isNativeProperty(inName) ? paccAlways : paccNever;

   if (inCallProp)
   {
      //printf("Set field %s %s = %s\n", inThis->toString().out_str(), inName.out_str(), inValue->toString().out_str());
      ScriptCallable *setter = findMemberSetter(inName);
      if (setter)
      {
         Array<Dynamic> args = Array_obj<Dynamic>::__new(1,1);
         args[0] = inValue;
         outValue.mPtr = runFunExprDynamic(CppiaCtx::getCurrent(),setter,inThis, args);
         return true;
      }
   }


   // Look for dynamic function (variable)
   for(int i=0;i<dynamicFunctions.size();i++)
   {
      if (cppia.strings[ dynamicFunctions[i]->nameId  ]==inName)
      {
         CppiaVar *d = dynamicFunctions[i];
         outValue = d->setValue(inThis,inValue);
         return true;
      }
   }

   for(int i=0;i<memberVars.size();i++)
   {
      CppiaVar &var = *memberVars[i];
      if (var.name==inName)
      {
         outValue = var.setValue(inThis, inValue);
         return true;
      }
   }

   hx::FieldMap *map = dynamicMapOffset ? (hx::FieldMap *)( (char *)inThis + dynamicMapOffset ) :
                        inThis->__GetFieldMap();
   if (map)
   {
      #ifdef HXCPP_GC_GENERATIONAL
      FieldMapSet(inThis,map, inName, inValue);
      #else
      FieldMapSet(map, inName, inValue);
      #endif
      outValue = inValue;
      return true;
   }

   // Fall though to haxe base
   //printf("Set field not found (%s) %s map=%p o=%d\n", inThis->toString().out_str(),inName.out_str(), map, dynamicMapOffset);

   return false;
}


int CppiaClassInfo::findFunctionSlot(int inName)
{
   for(int i=0;i<memberFunctions.size();i++)
      if (memberFunctions[i]->nameId==inName)
         return memberFunctions[i]->vtableSlot;
   return -1;
}

ExprType CppiaClassInfo::findFunctionType(CppiaModule &inModule, int inName)
{
   for(int i=0;i<memberFunctions.size();i++)
      if (memberFunctions[i]->nameId==inName)
         return inModule.types[ memberFunctions[i]->returnType ]->expressionType;
   return etVoid;
}


CppiaVar *CppiaClassInfo::findVar(bool inStatic,int inId)
{
   std::vector<CppiaVar *> &vars = inStatic ? staticVars : memberVars;
   for(int i=0;i<vars.size();i++)
   {
      if (vars[i]->nameId == inId)
         return vars[i];
   }

   std::vector<CppiaVar *> &dvars = inStatic ? staticDynamicFunctions : dynamicFunctions;
   for(int i=0;i<dvars.size();i++)
   {
      if (dvars[i]->nameId == inId)
         return dvars[i];
   }

   if (superType && superType->cppiaClass)
      return superType->cppiaClass->findVar(inStatic,inId);

   return 0;
}

void CppiaClassInfo::dumpVars(const char *inMessage, std::vector<CppiaVar *> &vars)
{
   printf(" %s:\n", inMessage);
   for(int i=0;i<vars.size();i++)
      printf("   %d] %s (%d)\n", i, cppia.strings[ vars[i]->nameId ].out_str(), vars[i]->nameId );
}

void CppiaClassInfo::dumpFunctions(const char *inMessage, std::vector<CppiaFunction *> &funcs)
{
   printf(" %s:\n", inMessage);
   for(int i=0;i<funcs.size();i++)
      printf("   %d] %s (%d)\n", i, cppia.strings[ funcs[i]->nameId ].out_str(), funcs[i]->nameId);
}


void CppiaClassInfo::dump()
{
   printf("Class %s\n", name.c_str());
   dumpFunctions("Member functions",memberFunctions);
   dumpFunctions("Static functions",staticFunctions);
   dumpVars("Member vars",memberVars);
   dumpVars("Member dyns",dynamicFunctions);
   dumpVars("Static vars",staticVars);
   dumpVars("Static dyns",staticDynamicFunctions);
}

CppiaEnumConstructor *CppiaClassInfo::findEnum(int inFieldId)
{
   for(int i=0;i<enumConstructors.size();i++)
      if (enumConstructors[i]->nameId==inFieldId)
         return enumConstructors[i];
   return 0;
}

void CppiaClassInfo::mark(hx::MarkContext *__inCtx)
{
   HX_MARK_MEMBER(mClass);
   for(int i=0;i<enumConstructors.size();i++)
   {
      HX_MARK_MEMBER(enumConstructors[i]->value);
      HX_MARK_MEMBER(enumConstructors[i]->name);
   }
   for(int i=0;i<staticVars.size();i++)
      staticVars[i]->mark(__inCtx);
   for(int i=0;i<staticDynamicFunctions.size();i++)
      staticDynamicFunctions[i]->mark(__inCtx);
}
#ifdef HXCPP_VISIT_ALLOCS
void CppiaClassInfo::visit(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(mClass);
   for(int i=0;i<enumConstructors.size();i++)
   {
      HX_VISIT_MEMBER(enumConstructors[i]->value);
      HX_VISIT_MEMBER(enumConstructors[i]->name);
   }
   for(int i=0;i<staticVars.size();i++)
      staticVars[i]->visit(__inCtx);
   for(int i=0;i<staticDynamicFunctions.size();i++)
      staticDynamicFunctions[i]->visit(__inCtx);
}
#endif



bool CppiaClassInfo::load(CppiaStream &inStream)
{
   CppiaOp op = inStream.getOp();
   isInterface = isEnum = false;

   if (op==IaClass)
      isInterface = false;
   else if (op==IaInterface)
      isInterface = true;
   else if (op==IaEnum)
      isEnum = true;
   else
   {
      DBGLOG("Invalid class field op %d\n", op);
      throw "Bad class type";
   }

   std::string tok;

    typeId = inStream.getInt();
    mClass.mPtr = createCppiaClass(this);

    superId = isEnum ? 0 : inStream.getInt();
    int implementCount = isEnum ? 0 : inStream.getInt();
    implements.resize(implementCount);
    for(int i=0;i<implementCount;i++)
       implements[i] = inStream.getInt();

    name = cppia.typeStr(typeId);
    DBGLOG("Class %s %s\n", name.c_str(), isEnum ? "enum" : isInterface ? "interface" : "class" );

    bool isNew = //(resolved == null() || !IsCppiaClass(resolved) ) &&
                (!HaxeNativeClass::findClass(name) && !HaxeNativeInterface::findInterface(name) );

    if (isNew && isEnum)
    {
       hx::Class cls =  hx::Class_obj::Resolve(String(name.c_str()));
       if (cls.mPtr && getScriptId(cls)==0)
       {
          DBGLOG("Found existing enum %s - ignore\n", name.c_str());
          isNew = false;
       }
    }

    if (isNew)
    {
       cppia.types[typeId]->cppiaClass = this;
    }
    else
    {
       DBGLOG("Already has registered %s - ignore\n",name.c_str());
    }

    inStream.module->creatingClass = name.c_str();

    int fields = inStream.getInt();
    for(int f=0;f<fields;f++)
    {
       if (isEnum)
       {
          CppiaEnumConstructor *enumConstructor = new CppiaEnumConstructor(cppia,inStream,this);
          enumConstructors.push_back( enumConstructor );
       }
       else
       {
          tok = inStream.getToken();
          if (tok=="FUNCTION")
          {
             bool isStatic = inStream.getStatic();
             bool isDynamic = inStream.getInt();
             CppiaFunction *func = new CppiaFunction(&cppia,isStatic,isDynamic);
             func->load(inStream,!isInterface);
             if (isDynamic)
             {
                if (isStatic)
                   staticDynamicFunctions.push_back( new CppiaVar(func) );
                else
                   dynamicFunctions.push_back( new CppiaVar(func) );
             }
             else
             {
                if (isStatic)
                   staticFunctions.push_back(func);
                else
                {
                   memberFunctions.push_back(func);
                }
             }
          }
          else if (tok=="VAR")
          {
             bool isStatic = inStream.getStatic();
             CppiaVar *var = new CppiaVar(isStatic);
             if (isStatic)
                staticVars.push_back(var);
             else
                memberVars.push_back(var);
             var->load(inStream);
          }
          else if (tok=="IMPLDYNAMIC")
          {
             // Fill in later
             dynamicMapOffset = -1;
          }
          else if (tok=="INLINE")
          {
             // OK
          }
          else
             throw "unknown field type";
       }
    }
    if (isEnum)
    {
       if (inStream.getBool())
          enumMeta = createCppiaExpr(inStream);
    }
    inStream.module->creatingClass = 0;
    return isNew;
}

#if (HXCPP_API_LEVEL<330)
void **CppiaClassInfo::createInterfaceVTable(int inTypeId)
{
   std::vector<CppiaExpr *> vtable;

   HaxeNativeInterface *interface = cppia.types[inTypeId]->interfaceBase;
   // Native-defined interface...
   if (interface)
   {
      vtable.push_back( findInterfaceFunction("toString") );
      ScriptNamedFunction *functions = interface->functions;
      if (functions != 0) {
         for(ScriptNamedFunction *f = functions; f->name; f++)
            if (strcmp(f->name,"toString"))
               vtable.push_back( findInterfaceFunction(f->name) );
      }
         
   }

   CppiaClassInfo *cls = cppia.types[inTypeId]->cppiaClass;
   if (!cls && !interface)
      throw "vtable for unknown class";

   if (cls && !cls->isInterface)
      throw "vtable for non-interface";

   if (cls)
   {
      for(int i=0;i<cls->memberFunctions.size();i++)
      {
         CppiaFunction *func = cls->memberFunctions[i];
         vtable.push_back( findFunction(false,func->nameId) );
      }
   }

   void **result = new void *[ vtable.size() + 1];
   result[0] = this;
   result++;
   memcpy(result, &vtable[0], sizeof(void *)*vtable.size());
   return result;
}
#endif

hx::Class *CppiaClassInfo::getSuperClass()
{
   DBGLOG("getSuperClass %s %d\n", name.c_str(), superId);
   if (!superId)
      return 0;

   TypeData *superType = cppia.types[ superId ];
   if (!superType)
      throw "Unknown super type!";
   if (superType->cppiaClass)
      return &superType->cppiaClass->mClass;
   if (!superType->haxeClass.mPtr)
   {
      printf("Invalid super class '%s' for '%s'.\n", superType->name.c_str(), name.c_str());
      throw "Missing super class";
   }

   return &superType->haxeClass;
}

void CppiaClassInfo::addMemberFunction(Functions &ioCombined, CppiaFunction *inNewFunc)
{
   for(int j=0;j<ioCombined.size();j++)
      if (ioCombined[j]->name==inNewFunc->name)
      {
         ioCombined[j] = inNewFunc;
         return;
      }
   ioCombined.push_back(inNewFunc);
}

void CppiaClassInfo::linkTypes()
{
   if (isLinked)
      return;
   isLinked = true;

   type = cppia.types[typeId];
   mClass->mName = type->name;
   superType = superId ? cppia.types[ superId ] : 0;
   CppiaClassInfo  *cppiaSuper = superType ? superType->cppiaClass : 0;
   // Link super first
   if (superType && superType->cppiaClass)
      superType->cppiaClass->linkTypes();

   // implemented interfaces before main class
   for(int i=0;i<implements.size();i++)
      if (cppia.types[ implements[i] ]->cppiaClass)
         cppia.types[ implements[i] ]->cppiaClass->linkTypes();


   // Add super interfaces ...
   TypeData *extraInterfaces = cppia.types[ superId ];
   while(extraInterfaces)
   {
      if (extraInterfaces->cppiaClass)
      {
         CppiaClassInfo &parent = *extraInterfaces->cppiaClass;
         std::vector<int> &impl = parent.implements;
         for(int i=0;i<impl.size();i++)
         {
            bool found = false;
            for(int j=0;j<implements.size() && !found; j++)
               found = implements[j] == impl[i];
            if (!found)
               implements.push_back(impl[i]);
         }
         extraInterfaces = cppia.types[ parent.superId ];
      }
      else
      {
         #if (HXCPP_API_LEVEL >= 330)
         HaxeNativeInterface *native = HaxeNativeInterface::findInterface( extraInterfaces->name.utf8_str() );
         if (native)
         {
            String hashName = extraInterfaces->name.split(HX_CSTRING(".")).mPtr->join(HX_CSTRING("::"));
            interfaceScriptTables[hashName.hash()] = native->scriptTable;
            ScriptNamedFunction *functions = native->functions;
            for(ScriptNamedFunction *f = functions; f && f->name; f++)
            {
               nativeInterfaceFunctions.push_back(f);
               int slot = cppia.getInterfaceSlot(f->name);
               if (slot<0)
                  printf("Interface slot '%s' not found\n",f->name);
               if (slot>interfaceSlotSize)
                  interfaceSlotSize = slot;
            }
         }
         #endif
         break;
      }
   }


   DBGLOG(" Linking class '%s' ", type->name.out_str());
   if (!superType)
   {
      DBGLOG("script base\n");
   }
   else if (cppiaSuper)
   {
      DBGLOG("extends script '%s'\n", superType->name.out_str());
   }
   else
   {
      DBGLOG("extends haxe '%s'\n", superType->name.out_str());
   }

   // Link class before we combine the function list...
   linkCppiaClass(mClass.mPtr,cppia,type->name);


   haxeBase = type->haxeBase;
   if (!haxeBase && !isInterface)
      throw "No base defined for non-interface";

   classSize = haxeBase ? haxeBase->mDataOffset : 0;

   // Combine member vars ...
   if (cppiaSuper)
   {
      classSize = cppiaSuper->classSize;

      containsPointers = cppiaSuper->containsPointers;

      if (cppiaSuper->dynamicMapOffset!=0)
         dynamicMapOffset = cppiaSuper->dynamicMapOffset;

      std::vector<CppiaVar *> combinedVars(cppiaSuper->memberVars );
      for(int i=0;i<memberVars.size();i++)
      {
         for(int j=0;j<combinedVars.size();j++)
            if (combinedVars[j]->nameId==memberVars[i]->nameId)
               printf("Warning duplicate member var %s\n", cppia.strings[memberVars[i]->nameId].out_str());
         combinedVars.push_back(memberVars[i]);
      }
      memberVars.swap(combinedVars);

      std::vector<CppiaVar *> combinedDynamics(cppiaSuper->dynamicFunctions );
      for(int i=0;i<dynamicFunctions.size();i++)
      {
         bool found = false;
         for(int j=0;j<combinedDynamics.size() && !found;j++)
            if (dynamicFunctions[i]->nameId == combinedDynamics[j]->nameId)
            {
               // Overwrite
               dynamicFunctions[i]->offset = combinedDynamics[j]->offset;
               combinedDynamics[j] = dynamicFunctions[i];
               found = true;
            }
         if (!found)
            combinedDynamics.push_back(dynamicFunctions[i]);
      }
      dynamicFunctions.swap(combinedDynamics);

      // Combine member functions ...
      Functions combinedFunctions(cppiaSuper->memberFunctions );
      if (isInterface)
      {
         // 'implements' interfaces are like extra super-classes for interfaces
         // For non-interface classes, these function will show up in the members anyhow
         for(int i=0;i<implements.size();i++)
         {
            CppiaClassInfo  *cppiaInterface = cppia.types[implements[i]]->cppiaClass;
            if (cppiaInterface)
            {
               Functions &intfFuncs = cppiaInterface->memberFunctions;
               for(int j=0;j<intfFuncs.size();j++)
                  addMemberFunction(combinedFunctions, intfFuncs[j]);
            }
         }
      }


      for(int i=0;i<memberFunctions.size();i++)
         addMemberFunction(combinedFunctions, memberFunctions[i]);

      memberFunctions.swap(combinedFunctions);
   }


   // Non-interface classes will have haxeBase
   if (haxeBase)
   {
      // Calculate table offsets...
      DBGLOG("  base haxe size %s = %d\n", haxeBase->name.c_str(), classSize);
      for(int i=0;i<memberVars.size();i++)
      {
         if (memberVars[i]->offset)
         {
            DBGLOG("   super var %s @ %d\n", cppia.identStr(memberVars[i]->nameId), memberVars[i]->offset);
         }
         else
         {
            DBGLOG("   link var %s @ %d\n", cppia.identStr(memberVars[i]->nameId), classSize);
            memberVars[i]->linkVarTypes(cppia,classSize);
            containsPointers = containsPointers || memberVars[i]->hasPointer();
         }
      }
      for(int i=0;i<dynamicFunctions.size();i++)
      {
         containsPointers = true;
         if (dynamicFunctions[i]->offset)
         {
            DBGLOG("   super dynamic function %s @ %d\n", cppia.identStr(dynamicFunctions[i]->nameId), dynamicFunctions[i]->offset);
         }
         else
         {
            DBGLOG("   link dynamic function %s @ %d\n", cppia.identStr(dynamicFunctions[i]->nameId), classSize);
            dynamicFunctions[i]->linkVarTypes(cppia,classSize);
         }
      }

      if (dynamicMapOffset==-1)
      {
         dynamicMapOffset = classSize;
         classSize += sizeof( hx::FieldMap * );
         containsPointers = true;
      }

      extraData = classSize - haxeBase->mDataOffset;

      if (!containsPointers)
         getHaxeBaseVTable();
   }


   DBGLOG("  script member vars size = %d\n", extraData);
 
   for(int i=0;i<staticVars.size();i++)
   {
      DBGLOG("   link static var %s\n", cppia.identStr(staticVars[i]->nameId));
      staticVars[i]->linkVarTypes(cppia);
   }

   for(int i=0;i<staticDynamicFunctions.size();i++)
   {
      DBGLOG("   link dynamic static var %s\n", cppia.identStr(staticDynamicFunctions[i]->nameId));
      staticDynamicFunctions[i]->linkVarTypes(cppia);
   }


   // Combine vtable positions...
   DBGLOG("  format haxe callable vtable (%d)....\n", (int)memberFunctions.size());
   std::vector<std::string> table;
   if (haxeBase)
      haxeBase->addVtableEntries(table);
   for(int i=0;i<table.size();i++)
      DBGLOG("   base table[%d] = %s\n", i, table[i].c_str() );


   int vtableSlot = table.size();
   for(int i=0;i<memberFunctions.size();i++)
   {
      int idx = -1;
      for(int j=0;j<table.size();j++)
         if (table[j] == memberFunctions[i]->name)
         {
            idx = j;
            break;
         }
      if (idx<0)
      {
         if (isInterface)
         {
            idx = cppia.getInterfaceSlot(memberFunctions[i]->name);
            if (idx==-1)
               throw "Missing function in interface";
            if (idx>interfaceSlotSize)
               interfaceSlotSize = idx;
            idx = -idx;
            DBGLOG("Using interface vtable[%d] = %s\n", idx, memberFunctions[i]->name.c_str() );
         }
         else
         {
            idx = vtableSlot++;
            DBGLOG("   cppia slot [%d] = %s\n", idx, memberFunctions[i]->name.c_str() );
         }
      }
      else
         DBGLOG("   override slot [%d] = %s\n", idx, memberFunctions[i]->name.c_str() );
      memberFunctions[i]->setVTableSlot(idx);
   }


   // Create interface vtables...
   for(int i=0;i<implements.size();i++)
   {
      int id = implements[i];
      #if (HXCPP_API_LEVEL < 330)
      void **vtable = createInterfaceVTable(id);
      #endif

      while(id > 0)
      {
         TypeData *interface = cppia.types[id];
         CppiaClassInfo  *cppiaInterface = interface->cppiaClass;
         #if (HXCPP_API_LEVEL >= 330)
         HaxeNativeInterface *native = HaxeNativeInterface::findInterface( interface->name.utf8_str() );
         if (native)
         {
            String hashName = interface->name.split(HX_CSTRING(".")).mPtr->join(HX_CSTRING("::"));
            interfaceScriptTables[hashName.hash()] = native->scriptTable;

            ScriptNamedFunction *functions = native->functions;
            if (functions != 0)
            {
               for(ScriptNamedFunction *f = functions; f->name; f++)
               {
                  nativeInterfaceFunctions.push_back(f);
                  int slot = cppia.getInterfaceSlot(f->name);
                  if (slot<0)
                     printf("Interface slot '%s' not found\n",f->name);
                  if (slot>interfaceSlotSize)
                     interfaceSlotSize = slot;
               }
            }
         }
         #else
         interfaceVTables[ interface->name.utf8_str() ] = vtable;
         #endif

         if (!cppiaInterface)
            break;

         Functions &intfFuncs = cppiaInterface->memberFunctions;
         for(int f=0;f<intfFuncs.size();f++)
         {
            int slot = cppia.getInterfaceSlot(intfFuncs[f]->name);
            if (slot<0)
               printf("Interface slot '%s' not found\n",intfFuncs[f]->name.c_str());
            if (slot>interfaceSlotSize)
               interfaceSlotSize = slot;
         }
         id =  cppiaInterface->superId;
      }
   }

   if (interfaceSlotSize)
      interfaceSlotSize++;

   vtable = new void*[vtableSlot + 2 + interfaceSlotSize];
   memset(vtable, 0, sizeof(void *)*(vtableSlot+2+interfaceSlotSize));
   vtable += interfaceSlotSize;
   *vtable++ = this;

   DBGLOG("  vtable size %d -> %p\n", vtableSlot, vtable);

   // Extract special function ...
   for(int i=0;i<staticFunctions.size(); )
   {
      if (staticFunctions[i]->name == "new")
      {
         newFunc = staticFunctions[i];
         staticFunctions.erase( staticFunctions.begin() + i);
      }
      else if (staticFunctions[i]->name == "__init__")
      {
         initExpr = staticFunctions[i]->funExpr;
         staticFunctions.erase( staticFunctions.begin() + i);
      }
      else
         i++;

   }

   for(int i=0;i<enumConstructors.size();i++)
   {
      CppiaEnumConstructor &e = *enumConstructors[i];
      e.name = cppia.strings[e.nameId];
      if (e.args.size()==0)
      {
         EnumBase base = new CppiaEnumBase(this);
         e.value = base;
         #if (HXCPP_API_LEVEL>=330)
         base->_hx_setIdentity(cppia.strings[e.nameId],i,0);
         #else
         base->__Set( cppia.strings[e.nameId],i,null() );
         #endif
      }
      else
      {
         e.index = i;
         e.value = createEnumClosure(e);
      }
   }

   if (!newFunc && cppiaSuper && cppiaSuper->newFunc)
   {
      //throw "No chaining constructor";
      newFunc = cppiaSuper->newFunc;
   }

   DBGLOG("  this constructor %p\n", newFunc);
}

#ifdef CPPIA_JIT
void CppiaClassInfo::compile()
{
   for(int i=0;i<staticFunctions.size();i++)
   {
      DBGLOG(" Compile %s::%s\n", name.c_str(), staticFunctions[i]->name.c_str() );
      staticFunctions[i]->compile();
   }
   if (newFunc)
      newFunc->compile();

   // Functions
   for(int i=0;i<memberFunctions.size();i++)
   {
      DBGLOG(" Compile member %s::%s\n", name.c_str(), memberFunctions[i]->name.c_str() );
      memberFunctions[i]->compile();
   }
}
#endif

void CppiaClassInfo::link()
{
   int newPos = -1;
   for(int i=0;i<staticFunctions.size();i++)
   {
      DBGLOG(" Link %s::%s\n", name.c_str(), staticFunctions[i]->name.c_str() );
      staticFunctions[i]->link();
   }
   if (newFunc)
      newFunc->link();

   if (initExpr)
      initExpr = initExpr->link(cppia);

   // Functions
   for(int i=0;i<memberFunctions.size();i++)
   {
      DBGLOG(" Link member %s::%s\n", name.c_str(), memberFunctions[i]->name.c_str() );
      memberFunctions[i]->link();
   }

   for(int i=0;i<staticDynamicFunctions.size();i++)
      staticDynamicFunctions[i]->link(cppia);

   for(int i=0;i<dynamicFunctions.size();i++)
      dynamicFunctions[i]->link(cppia);

   for(int i=0;i<memberFunctions.size();i++)
   {
      vtable[ memberFunctions[i]->vtableSlot ] = memberFunctions[i]->funExpr;
      if (interfaceSlotSize)
      {
         int interfaceSlot = cppia.findInterfaceSlot( memberFunctions[i]->name );
         DBGLOG("Set %s : %s to %d @%p\n", name.c_str(), memberFunctions[i]->name.c_str(), interfaceSlot, vtable);
         if (interfaceSlot>0 && interfaceSlot<interfaceSlotSize)
            vtable[ -interfaceSlot ] = memberFunctions[i]->funExpr;
      }
   }

   // Find interface functions that are not implemented in client, but rely on host fallback...
   #if (HXCPP_API_LEVEL >= 330)
   if (!isInterface)
   {
      std::vector<ScriptNamedFunction *> &nativeInterfaceFuncs = type->cppiaClass->nativeInterfaceFunctions;
      for(int i=0;i<nativeInterfaceFuncs.size();i++)
      {
         int interfaceSlot = cppia.findInterfaceSlot( nativeInterfaceFuncs[i]->name);
         if (!vtable[-interfaceSlot])
            vtable[-interfaceSlot] = new ScriptCallable(cppia,nativeInterfaceFuncs[i]);
      }
   }
   #endif

   // Vars ...
   if (!isInterface)
   {
      for(int i=0;i<memberVars.size();i++)
      {
         CppiaVar &var = *memberVars[i];
         var.link(cppia);
         if (var.readAccess == CppiaVar::accCall || var.readAccess==CppiaVar::accCallNative)
         {
            ScriptCallable *getter = findFunction(false,HX_CSTRING("get_") + var.name);
            if (!getter)
            {
               dump();
               throw Dynamic(HX_CSTRING("Could not find getter for ") + var.name);
            }
            DBGLOG("  found getter for %s.%s\n", name.c_str(), var.name.out_str());
            memberGetters[var.name.utf8_str()] = getter;
         }
         if (var.writeAccess == CppiaVar::accCall || var.writeAccess==CppiaVar::accCallNative)
         {
            ScriptCallable *setter = findFunction(false,HX_CSTRING("set_") + var.name);
            if (!setter)
               throw Dynamic(HX_CSTRING("Could not find setter for ") + var.name);
            DBGLOG("  found setter for %s.%s\n", name.c_str(), var.name.out_str());
            memberSetters[var.name.utf8_str()] = setter;
         }

         if (var.readAccess == CppiaVar::accCallNative || var.writeAccess == CppiaVar::accCallNative)
            nativeProperties.insert( var.name );
      }
   }

   for(int i=0;i<staticVars.size();i++)
   {
      CppiaVar &var = *staticVars[i];
      var.link(cppia);
      if (var.readAccess == CppiaVar::accCall || var.readAccess == CppiaVar::accCallNative)
      {
         ScriptCallable *getter = findFunction(true,HX_CSTRING("get_") + var.name);
         if (!getter)
            throw Dynamic(HX_CSTRING("Could not find getter for ") + var.name);
         DBGLOG("  found getter for %s.%s\n", name.c_str(), var.name.out_str());
         staticGetters[var.name.utf8_str()] = getter;
      }
      if (var.writeAccess == CppiaVar::accCall || var.writeAccess == CppiaVar::accCallNative)
      {
         ScriptCallable *setter = findFunction(true,HX_CSTRING("set_") + var.name);
         if (!setter)
            throw Dynamic(HX_CSTRING("Could not find setter for ") + var.name);
         DBGLOG("  found setter for %s.%s\n", name.c_str(), var.name.out_str());
         staticSetters[var.name.utf8_str()] = setter;
      }

      if (var.readAccess == CppiaVar::accCallNative || var.writeAccess == CppiaVar::accCallNative)
         nativeProperties.insert( var.name );
   }

   if (enumMeta)
      enumMeta = enumMeta->link(cppia);

   //printf("Found haxeBase %s = %p / %d\n", cppia.types[typeId]->name.out_str(), haxeBase, dataSize );
}

void CppiaClassInfo::init(CppiaCtx *ctx, int inPhase)
{
   unsigned char *pointer = ctx->pointer;
   ctx->push( (hx::Object *) 0 ); // this
   AutoStack save(ctx,pointer);
 
   if (inPhase==0)
   {
      for(int i=0;i<staticVars.size();i++)
      {
         staticVars[i]->runInit(ctx);
         if (staticVars[i]->name==HX_CSTRING("__meta__"))
         {
            // Todo - delete/clean value
            mClass->__meta__ = staticVars[i]->objVal;
            staticVars.erase( staticVars.begin() + i );
            i--;
         }
      }
      if (enumMeta)
         mClass->__meta__ = enumMeta->runObject(ctx);

      for(int i=0;i<staticDynamicFunctions.size();i++)
         staticDynamicFunctions[i]->runInit(ctx);
   }
   else if (inPhase==1)
   {
      if (initExpr)
         initExpr->runVoid(ctx);
   }
}


Dynamic CppiaClassInfo::getStaticValue(const String &inName,hx::PropertyAccess  inCallProp)
{
   if (inCallProp==paccDynamic)
      inCallProp = isNativeProperty(inName) ? paccAlways : paccNever;
   if (inCallProp)
   {
      ScriptCallable *getter = findStaticGetter(inName);
      if (getter)
      {
         Array<Dynamic> empty;
         return runFunExprDynamic(CppiaCtx::getCurrent(),getter,0, empty);
      }
   }

   CppiaExpr *closure = findFunction(true,inName);
   if (closure)
      return createMemberClosure(0,(ScriptCallable*)closure);

   // Look for dynamic function (variable)
   for(int i=0;i<staticDynamicFunctions.size();i++)
   {
      if (cppia.strings[ staticDynamicFunctions[i]->nameId  ]==inName)
      {
         CppiaVar *d = staticDynamicFunctions[i];

         return d->getStaticValue();
      }
   }

   for(int i=0;i<staticVars.size();i++)
   {
      CppiaVar &var = *staticVars[i];
      if (var.name==inName)
         return var.getStaticValue();
   }

   printf("Get static field not found (%s) %s\n", name.c_str(),inName.out_str());
   return null();
}


bool CppiaClassInfo::hasStaticValue(const String &inName)
{
   if (findStaticGetter(inName))
      return true;

   CppiaExpr *closure = findFunction(true,inName);
   if (closure)
      return true;

   // Look for dynamic function (variable)
   for(int i=0;i<staticDynamicFunctions.size();i++)
      if (cppia.strings[ staticDynamicFunctions[i]->nameId  ]==inName)
         return true;

   for(int i=0;i<staticVars.size();i++)
   {
      CppiaVar &var = *staticVars[i];
      if (var.name==inName)
         return true;
   }

   return false;
}


Dynamic CppiaClassInfo::setStaticValue(const String &inName,const Dynamic &inValue ,hx::PropertyAccess  inCallProp)
{
   if (inCallProp==paccDynamic)
      inCallProp = isNativeProperty(inName) ? paccAlways : paccNever;
   if (inCallProp)
   {
      ScriptCallable *setter = findStaticSetter(inName);
      if (setter)
      {
         Array<Dynamic> args = Array_obj<Dynamic>::__new(1,1);
         args[0] = inValue;
         return runFunExprDynamic(CppiaCtx::getCurrent(),setter,0, args);
      }
   }


   // Look for dynamic function (variable)
   for(int i=0;i<staticDynamicFunctions.size();i++)
   {
      if (cppia.strings[ staticDynamicFunctions[i]->nameId  ]==inName)
      {
         CppiaVar *d = staticDynamicFunctions[i];
         return d->setStaticValue(inValue);
      }
   }

   for(int i=0;i<staticVars.size();i++)
   {
      CppiaVar &var = *staticVars[i];
      if (var.name==inName)
      {
         return var.setStaticValue(inValue);
      }
   }

   printf("Set static field not found (%s) %s\n", name.c_str(), inName.out_str());
   return null();

}

void CppiaClassInfo::GetInstanceFields(hx::Object *inObject, Array<String> &ioFields)
{
   for(int i=0;i<memberVars.size();i++)
   {
      CppiaVar &var = *memberVars[i];
      ioFields->push(var.name);
   }

   if (inObject)
   {
      hx::FieldMap *map = inObject->__GetFieldMap();
      if (map)
         FieldMapAppendFields(map, ioFields);
   }
}

Array<String> CppiaClassInfo::GetClassFields()
{
   Array<String> result = Array_obj<String>::__new();

   if (isEnum)
   {
      for(int i=0;i<enumConstructors.size();i++)
         result->push(enumConstructors[i]->name);
   }
   else
   {

      for(int i=0;i<staticVars.size();i++)
      {
         CppiaVar &var = *staticVars[i];
         if (var.isVirtual)
            continue;
         result->push(var.name);
      }

      for(int i=0;i<staticDynamicFunctions.size();i++)
      {
         CppiaVar &var = *staticDynamicFunctions[i];
         if (var.isVirtual)
            continue;
         result->push(var.name);
      }

      for(int i=0;i<staticFunctions.size();i++)
      {
         CppiaFunction &func = *staticFunctions[i];
         result->push(func.getName());
      }
   }

   return result;
}


void CppiaClassInfo::markInstance(hx::Object *inThis, hx::MarkContext *__inCtx)
{
   if (dynamicMapOffset)
      HX_MARK_MEMBER(getFieldMap(inThis));

   for(int i=0;i<dynamicFunctions.size();i++)
      dynamicFunctions[i]->mark(inThis, __inCtx);
   // Only script members?
   for(int i=0;i<memberVars.size();i++)
      memberVars[i]->mark(inThis, __inCtx);
}

#ifdef HXCPP_VISIT_ALLOCS
void CppiaClassInfo::visitInstance(hx::Object *inThis, hx::VisitContext *__inCtx)
{
   if (dynamicMapOffset)
      HX_VISIT_MEMBER(getFieldMap(inThis));

   for(int i=0;i<dynamicFunctions.size();i++)
      dynamicFunctions[i]->visit(inThis, __inCtx);
   for(int i=0;i<memberVars.size();i++)
      memberVars[i]->visit(inThis, __inCtx);
}
#endif




// --- CppiaClass --------






class CppiaClass : public hx::Class_obj
{
public:
   CppiaClassInfo *info;

   CppiaClass(CppiaClassInfo *inInfo)
   {
      info = inInfo;
   }

   Array<String> GetClassFields()
   {
      return info->GetClassFields();
   }

   Array<String> GetInstanceFields()
   {
      Array<String> members = mSuper ? (*mSuper)->GetInstanceFields() : Array_obj<String>::__new(0,0);

      for(int i=0;i<info->memberFunctions.size();i++)
      {
         CppiaFunction *func = info->memberFunctions[i];
         String name = func->getName();
         if (members->Find(name)<0)
            members->push(name);
      }

      CppiaModule &module = info->cppia;

      for(int i=0;i<info->memberVars.size();i++)
      {
         CppiaVar *var = info->memberVars[i];
         if (var->isVirtual)
            continue;
         String name = module.strings[var->nameId];
         if (members->Find(name)<0)
            members->push( name );
      }

      for(int i=0;i<info->dynamicFunctions.size();i++)
      {
         CppiaVar *var = info->dynamicFunctions[i];
         if (var->isVirtual)
            continue;
         String name = module.strings[var->nameId];
         if (members->Find(name)<0)
            members->push(name);
      }

      return members;
   }

   void linkClass(CppiaModule &inModule,String inName)
   {
      mName = inName;
      mSuper = info->getSuperClass();
      DBGLOG("LINK %p ########################### %s -> %p\n", this, mName.out_str(), mSuper );
      //mStatics = Array_obj<String>::__new(0,0);

      bool overwrite = false;
      hx::Class old = hx::Class_obj::Resolve(inName);
      // Overwrite cppia classes
      if (old.mPtr && dynamic_cast<CppiaClass *>( old.mPtr ) )
         overwrite = true;
      registerScriptable(overwrite);
   }


   Dynamic ConstructEmpty()
   {
      if (info->isEnum)
         return Dynamic();

      Expressions none;
      return info->createInstance(CppiaCtx::getCurrent(),none,false);
   } 

   Dynamic ConstructArgs(hx::DynamicArray inArgs)
   {
      return info->createInstance(CppiaCtx::getCurrent(),inArgs);
   }

   bool __IsEnum() { return info->isEnum; }

   Dynamic ConstructEnum(String inName,hx::DynamicArray inArgs)
   {
      if (!info->isEnum)
         return Dynamic();

      int index = info->getEnumIndex(inName);
	   return info->enumConstructors[index]->create(inArgs);
   }

   bool VCanCast(hx::Object *inPtr)
   {
      if (!inPtr)
         return false;

      hx::Class c = inPtr->__GetClass();
      if (!c.mPtr)
         return false;

      if (info->isInterface)
      {
         // Can only be this cppia interface if it is a cppia class...
         CppiaClass *cppiaClass = dynamic_cast<CppiaClass *>(c.mPtr);
         if (!cppiaClass)
            return false;
         return cppiaClass->info->implementsInterface(info);
      }


      hx::Class_obj *classPtr = c.mPtr;
      while(classPtr)
      {
         if (classPtr==this)
            return true;
         if (info->isEnum)
            return false;
         classPtr = classPtr->GetSuper().mPtr;
      }

      return false;
   }


   hx::Val __Field(const String &inName,hx::PropertyAccess inCallProp)
   {
      if (inName==HX_CSTRING("__meta__"))
         return __meta__;
      return info->getStaticValue(inName,inCallProp);
   }

   hx::Val __SetField(const String &inName,const hx::Val &inValue ,hx::PropertyAccess inCallProp)
   {
      return info->setStaticValue(inName,inValue,inCallProp);
   }

   bool __HasField(const String &inName)
   {
      if (inName==HX_CSTRING("__meta__"))
         return __meta__!=null();

      return info->hasStaticValue(inName);
   }

};


hx::Class_obj *createCppiaClass(CppiaClassInfo *inInfo) { return new CppiaClass(inInfo); }









// --- Enum Base ---
::hx::ObjectPtr<hx::Class_obj > CppiaEnumBase::__GetClass() const
{
   return classInfo->getClass();
}

::String CppiaEnumBase::GetEnumName( ) const
{
   return classInfo->getClass()->mName;
}

::String CppiaEnumBase::__ToString() const
{
   #if (HXCPP_API_LEVEL>=330)
   return classInfo->getClass()->mName + HX_CSTRING(".") + _hx_tag;
   #else
   return classInfo->getClass()->mName + HX_CSTRING(".") + tag;
   #endif
}









void cppiaClassInit(CppiaClassInfo *inClass, CppiaCtx *ctx, int inPhase)
{
   inClass->init(ctx,inPhase);
}


void cppiaClassMark(CppiaClassInfo *inClass,hx::MarkContext *__inCtx)
{
   inClass->mark(__inCtx);
}
#ifdef HXCPP_VISIT_ALLOCS
void cppiaClassVisit(CppiaClassInfo *inClass,hx::VisitContext *__inCtx)
{
   inClass->visit(__inCtx);
}
#endif



void  linkCppiaClass(hx::Class_obj *inClass, CppiaModule &cppia, String inName)
{
   ((CppiaClass *)inClass)->linkClass(cppia,inName);
}


int getScriptId(hx::Class inClass)
{
   hx::Class_obj *ptr = inClass.mPtr;
   if (!ptr)
      return 0;
   CppiaClass *cls = dynamic_cast<CppiaClass *>(ptr);
   if (!cls)
      return 0;
   return cls->info->cppia.scriptId;
}





::String ScriptableToString(void *inClass)
{
   CppiaClassInfo *info = (CppiaClassInfo *)inClass;
   return info->getClass()->toString();
}

int ScriptableGetType(void *inClass)
{
   CppiaClassInfo *info = (CppiaClassInfo *)inClass;
   return info->__GetType();
}


hx::Class ScriptableGetClass(void *inClass)
{
   CppiaClassInfo *info = (CppiaClassInfo *)inClass;
   return info->getClass();
}


// Called by haxe generated code ...
void ScriptableMark(void *inClass, hx::Object *inThis, hx::MarkContext *__inCtx)
{
   ((CppiaClassInfo *)inClass)->markInstance(inThis, __inCtx);
}

#ifdef HXCPP_VISIT_ALLOCS
void ScriptableVisit(void *inClass, hx::Object *inThis, hx::VisitContext *__inCtx)
{
   ((CppiaClassInfo *)inClass)->visitInstance(inThis, __inCtx);
}
#endif

bool ScriptableField(hx::Object *inObj, const ::String &inName,hx::PropertyAccess  inCallProp,Dynamic &outResult)
{
   void **vtable = inObj->__GetScriptVTable();
   return ((CppiaClassInfo *)vtable[-1])->getField(inObj,inName,inCallProp,outResult);
}

bool ScriptableField(hx::Object *inObj, int inName,hx::PropertyAccess  inCallProp,Float &outResult)
{
   void **vtable = inObj->__GetScriptVTable();
   Dynamic result;
   if ( ((CppiaClassInfo *)vtable[-1])->getField(inObj,__hxcpp_field_from_id(inName),inCallProp,result) )
   {
      //CPPIA_CHECK(result);
      if (result.mPtr)
         outResult = result->__ToDouble();
      else
         outResult = 0;
      return true;
   }
   return false;
}

bool ScriptableField(hx::Object *inObj, int inName,hx::PropertyAccess  inCallProp,Dynamic &outResult)
{
   void **vtable = inObj->__GetScriptVTable();
   return ((CppiaClassInfo *)vtable[-1])->getField(inObj,__hxcpp_field_from_id(inName),inCallProp,outResult);
}

void ScriptableGetFields(hx::Object *inObject, Array< ::String> &outFields)
{
   void **vtable = inObject->__GetScriptVTable();
   return ((CppiaClassInfo *)vtable[-1])->GetInstanceFields(inObject,outFields);
}

bool ScriptableSetField(hx::Object *inObj, const ::String &inName, Dynamic inValue,hx::PropertyAccess  inCallProp, Dynamic &outResult)
{
   void **vtable = inObj->__GetScriptVTable();
   return ((CppiaClassInfo *)vtable[-1])->setField(inObj,inName,inValue,inCallProp,outResult);
}

#if (HXCPP_API_LEVEL >= 330)
void *hx::Object::_hx_getInterface(int inId)
{
   void **vtable = __GetScriptVTable();
   if (!vtable)
      return 0;
   CppiaClassInfo *info = ((CppiaClassInfo *)vtable[-1]);
   void *result = info->interfaceScriptTables[inId];
   return info->interfaceScriptTables[inId];
}
#endif



} // end namespace hx

