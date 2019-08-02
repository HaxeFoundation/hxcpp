#include <hxcpp.h>
#include "Cppia.h"
#include "CppiaStream.h"

namespace hx
{

static int sTypeSize[] = { 0, 0, sizeof(hx::Object *), sizeof(String), sizeof(Float), sizeof(int) };

inline void AlignOffset(ExprType type, int &ioOffset)
{
   #ifdef HXCPP_ALIGN_FLOAT
   if (type==etFloat && (ioOffset & 0x7) )
      ioOffset = (ioOffset + 7) & ~7;
   #endif
}


FieldStorage fieldStorageFromType(TypeData *inType)
{
   if (!inType)
      return fsObject;

   switch(inType->expressionType)
   {
      case etInt:
         if (inType->name==HX_CSTRING("Bool"))
            return fsBool;
         return fsInt;
      case etFloat: return fsFloat;
      case etString: return fsString;
      case etObject: return fsObject;
      default:
           ;
   }
   return fsUnknown;
}


// --- CppiaVar ----

CppiaVar::CppiaVar(bool inIsStatic) : isStatic(inIsStatic)
{
   clear();
}

CppiaVar::CppiaVar(CppiaFunction *inDynamicFunction)
{
   clear();
   isStatic = inDynamicFunction->isStatic;
   dynamicFunction = inDynamicFunction;
   nameId = dynamicFunction->nameId;
   exprType = etObject;
   storeType = fsObject;
}

void CppiaVar::clear()
{
   type = 0;
   nameId = 0;
   typeId = 0;
   offset = 0;
   type = 0;
   objVal.mPtr = 0;
   intVal = 0;
   stringVal = String();
   valPointer = 0;
   storeType = fsUnknown;
   dynamicFunction = 0;
   isVirtual = false;
   init = 0;
}

void CppiaVar::load(CppiaStream &stream)
{
   readAccess = getAccess(stream);
   writeAccess = getAccess(stream);
   isVirtual = stream.getBool();
   nameId = stream.getInt();
   typeId = stream.getInt();
   if (stream.getInt())
      init = createCppiaExpr(stream);
}


// Static...
void CppiaVar::linkVarTypes(CppiaModule &cppia)
{
   if (dynamicFunction)
   {
      nameId = dynamicFunction->nameId;
   }
   type = cppia.types[typeId];
   name = cppia.strings[nameId];
   exprType = typeId==0 ? etObject : type->expressionType;

   if (!isVirtual)
   {
      storeType = typeId==0 ? fsObject : fieldStorageFromType(type);

      switch(storeType)
      {
         case fsBool: valPointer = &boolVal;  break;
         case fsByte: valPointer = &byteVal;  break;
         case fsInt: valPointer = &intVal;  break;
         case fsFloat: valPointer = &floatVal; break;
         case fsString: valPointer = &stringVal; break;
         case fsObject: valPointer = &objVal; break;
         default:
            valPointer = 0;
            throw "Unkown store type";
      }
   }
}

bool CppiaVar::hasPointer()
{
   return storeType==fsString || storeType==fsObject;
}


Dynamic CppiaVar::getStaticValue()
{
   switch(storeType)
   {
      case fsByte: return *(unsigned char *)(valPointer);
      case fsBool: return *(bool *)(valPointer);
      case fsInt: return *(int *)(valPointer);
      case fsFloat: return *(Float *)(valPointer);
      case fsString: return *(String *)(valPointer);
      case fsObject: return *(hx::Object **)(valPointer);
      case fsUnknown:
         break;
   }
   return null();
}


Dynamic CppiaVar::setStaticValue(Dynamic inValue)
{
   switch(storeType)
   {
      case fsByte: *(unsigned char *)(valPointer) = inValue; return inValue;
      case fsBool: *(bool *)(valPointer) = inValue; return inValue;
      case fsInt: *(int *)(valPointer) = inValue; return inValue;
      case fsFloat: *(Float *)(valPointer) = inValue; return inValue;
      case fsString: *(String *)(valPointer) = inValue; return inValue;
      case fsObject: *(hx::Object **)(valPointer) = inValue.mPtr; return inValue;
      case fsUnknown:
         break;
   }
   return null();
}




CppiaExpr *CppiaVar::createAccess(CppiaExpr *inSrc)
{
   if (valPointer)
      return createStaticAccess(inSrc, storeType, valPointer);
   if (isVirtual)
      throw "Direct access to extern field";
   //throw "Unlinked static variable";
   return 0;
}

void CppiaVar::linkVarTypes(CppiaModule &cppia, int &ioOffset)
{
   DBGLOG("linkVarTypes %p\n",dynamicFunction);
   if (dynamicFunction)
   {
      //dynamicFunction->linkTypes(cppia);
      nameId = dynamicFunction->nameId;
   }
   type = cppia.types[typeId];
   if (!isVirtual)
   {
      exprType = typeId==0 ? etObject : type->expressionType;
      AlignOffset(exprType, ioOffset);
      offset = ioOffset;
      
      switch(exprType)
      {
         case etInt: ioOffset += sizeof(int); storeType=fsInt; break;
         case etFloat: ioOffset += sizeof(Float);storeType=fsFloat;  break;
         case etString: ioOffset += sizeof(String);storeType=fsString;  break;
         case etObject: ioOffset += sizeof(hx::Object *);storeType=fsObject;  break;
         case etVoid:
         case etNull:
            break;
      }
   }
}

void CppiaVar::createDynamic(hx::Object *inBase)
{
   *(hx::Object **)((char *)inBase+offset) = createMemberClosure(inBase,(ScriptCallable*)dynamicFunction->funExpr);
}


Dynamic CppiaVar::getValue(hx::Object *inThis)
{
   char *base = ((char *)inThis) + offset;
 
   switch(storeType)
   {
      case fsByte: return *(unsigned char *)(base);
      case fsInt: return *(int *)(base);
      case fsBool: return *(bool *)(base);
      case fsFloat: return *(Float *)(base);
      case fsString: return *(String *)(base);
      case fsObject: return *(hx::Object **)(base);
      case fsUnknown:
         break;
   }
   return null();
}

Dynamic CppiaVar::setValue(hx::Object *inThis, Dynamic inValue)
{
   char *base = ((char *)inThis) + offset;
 
   switch(storeType)
   {
      case fsByte: *(unsigned char *)(base) = inValue; return inValue;
      case fsInt: *(int *)(base) = inValue; return inValue;
      case fsBool: *(bool *)(base) = inValue; return inValue;
      case fsFloat: *(Float *)(base) = inValue; return inValue;
      case fsString: *(String *)(base) = inValue; return inValue;
      case fsObject:
            switch(type->arrayType)
            {
               case arrNotArray:
                  *(hx::Object **)(base) = inValue.mPtr;
                  break;
               case arrBool:
                  *(Array<bool> *)(base) = inValue;
                  break;
               case arrInt:
                  *(Array<int> *)(base) = inValue;
                  break;
               case arrFloat:
                  *(Array<Float> *)(base) = inValue;
                  break;
               case arrFloat32:
                  *(Array<float> *)(base) = inValue;
                  break;
               case arrUnsignedChar:
                  *(Array<unsigned char> *)(base) = inValue;
                  break;
               case arrString:
                  *(Array<String> *)(base) = inValue;
                  break;
               case arrAny:
                  #if (HXCPP_API_LEVEL>=330)
                  (*(cpp::VirtualArray *)base).setDynamic(inValue);
                  break;
                  #else
                  // Fallthrough
                  #endif
               case arrObject:
                  *(Array<Dynamic> *)(base) = inValue;
                  break;
            }
            return inValue;
      case fsUnknown:
         break;
   }
   return null();
}


void CppiaVar::link(CppiaModule &inModule)
{
   if (dynamicFunction)
      dynamicFunction->link();
   type = inModule.types[typeId];
   exprType = typeId==0 ? etObject : type->expressionType;
   name = inModule.strings[nameId];

   if (init)
      init = init->link(inModule);
}

CppiaVar::Access CppiaVar::getAccess(CppiaStream &stream)
{
   std::string tok = stream.getToken();
   if (tok.size()!=1)
      throw "bad var access length";
   switch(tok[0])
   {
      case 'N': return accNormal;
      case 'n': return accNo;
      case 'R': return accResolve;
      case 'C': return accCall;
      case 'V': return accCallNative;
   }
   throw "bad access code";
   return accNormal;
}

void CppiaVar::runInit(CppiaCtx *ctx)
{
   if (isStatic)
   {
      if (dynamicFunction)
         objVal = createMemberClosure(0,(ScriptCallable*)dynamicFunction->funExpr);
      else if (init)
         switch(storeType)
         {
            case fsBool: boolVal = init->runInt(ctx); break;
            case fsByte: byteVal = init->runInt(ctx); break;
            case fsInt: intVal = init->runInt(ctx); break;
            case fsFloat: floatVal = init->runFloat(ctx); break;
            case fsString: stringVal = init->runString(ctx); break;
            case fsObject: objVal = init->runObject(ctx); break;
            case fsUnknown: ; // ?
         }
   }
}

void CppiaVar::mark(hx::MarkContext *__inCtx)
{
   HX_MARK_MEMBER(stringVal);
   HX_MARK_MEMBER(objVal);
   HX_MARK_MEMBER(name);
}
void CppiaVar::visit(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(stringVal);
   HX_VISIT_MEMBER(objVal);
   HX_VISIT_MEMBER(name);
}



// --- CppiaStackVar ----


std::map<int,int> sVarIdNameMap;


int getStackVarNameId(int inVarId)
{
   return sVarIdNameMap[inVarId];
}



CppiaStackVar::CppiaStackVar()
{
   nameId = 0;
   id = 0;
   capture = false;
   typeId = 0;
   defaultStackPos = -1;
   stackPos = 0;
   fromStackPos = 0;
   capturePos = 0;
   expressionType = etNull;
   argType = expressionType;
   storeType = fsUnknown;
   type = 0;
   module = 0;
}

CppiaStackVar::CppiaStackVar(CppiaStackVar *inVar,int &ioSize, int &ioCaptureSize)
{
   nameId = inVar->nameId;
   id = inVar->id;
   sVarIdNameMap[id] = nameId;
   capture = inVar->capture;
   typeId = inVar->typeId;
   type = inVar->type;
   expressionType = inVar->expressionType;
   argType = expressionType;
   defaultStackPos = -1;

   fromStackPos = inVar->stackPos;
   storeType = inVar->storeType;
   stackPos = ioSize;
   capturePos = ioCaptureSize;
   ioSize += sTypeSize[expressionType];
   ioCaptureSize += sTypeSize[expressionType];
   module = inVar->module;
}


void CppiaStackVar::fromStream(CppiaStream &stream)
{
   nameId = stream.getInt();
   id = stream.getInt();
   capture = stream.getBool();
   typeId = stream.getInt();
}

void CppiaStackVar::setInFrame(unsigned char *inFrame,Dynamic inValue)
{
   unsigned char *ptr = inFrame + stackPos;
   switch(storeType)
   {
      case fsByte:
         *(int *)(ptr) = (unsigned char)inValue;
         break;
      case fsBool:
         *(int *)(ptr) = (bool)inValue;
         break;
      case fsInt:
         *(int *)(ptr) = inValue;
         break;
      case fsFloat:
         SetFloatAligned(ptr,inValue);
         break;
      case fsString:
         *(String *)(ptr) = inValue;
         break;
      case fsObject:
         *(hx::Object **)(ptr) = inValue.mPtr;
         break;
      case fsUnknown:
         break;
   }
}

void CppiaStackVar::set(CppiaCtx *inCtx,Dynamic inValue)
{
   setInFrame( inCtx->frame, inValue );
}


Dynamic CppiaStackVar::getInFrame(const unsigned char *inFrame)
{
   const unsigned char *ptr = inFrame + stackPos;
   switch(storeType)
   {
      case fsByte: return (unsigned char) *(int *)ptr;
      case fsBool: return (bool) *(int *)ptr;
      case fsInt: return *(int *)ptr;
      case fsFloat: return *(Float *)ptr;
      case fsString: return *(String *)ptr;
      case fsObject: return  *(hx::Object **)ptr;
      case fsUnknown:
         break;
   }
   return null();
}


void CppiaStackVar::markClosure(char *inBase, hx::MarkContext *__inCtx)
{
   switch(storeType)
   {
      case fsString:
         HX_MARK_MEMBER(*(String *)(inBase + capturePos));
         break;
      case fsObject:
         HX_MARK_MEMBER(*(hx::Object **)(inBase + capturePos));
         break;
      default: ;
   }
}

void CppiaStackVar::visitClosure(char *inBase, hx::VisitContext *__inCtx)
{
   switch(storeType)
   {
      case fsString:
         HX_VISIT_MEMBER(*(String *)(inBase + capturePos));
         break;
      case fsObject:
         HX_VISIT_MEMBER(*(hx::Object **)(inBase + capturePos));
         break;
      default: ;
   }
}

void CppiaStackVar::link(CppiaModule &inModule, bool hasDefault)
{
   expressionType = inModule.types[typeId]->expressionType;
   argType = hasDefault ? ( expressionType==etString ? etString : etObject) : expressionType;
   storeType = typeId==0 ? fsObject : fieldStorageFromType(inModule.types[typeId]);
   if (inModule.layout)
   {
      inModule.layout->varMap[id] = this;
      stackPos = inModule.layout->size;
      inModule.layout->size += sTypeSize[argType];
   }
   else
   {
      stackPos = -1;
      storeType = fsUnknown;
   }
   type = inModule.types[typeId];
   defaultStackPos =stackPos;
   module = &inModule;
}


void CppiaStackVar::linkDefault()
{
   if (expressionType==etFloat && sizeof(double)>sizeof(hx::Object *) )
   {
      stackPos = module->layout->size;
      module->layout->size += sTypeSize[expressionType];
   }
}

void CppiaStackVar::setDefault(CppiaCtx *inCxt, const CppiaConst &inDefault)
{
   if (argType==etString)
   {
      if (inDefault.type == CppiaConst::cString)
      {
         String *s = (String *)(inCxt->frame + defaultStackPos);
         if (!s->raw_ptr())
            *s = module->strings[ inDefault.ival ];
      }
   }
   else
   {
      hx::Object *src = *(hx::Object **)(inCxt->frame + defaultStackPos);

      if (src)
      {
         setInFrame(inCxt->frame, src);
      }
      else
      {
         unsigned char *ptr = inCxt->frame + stackPos;
         switch(storeType)
         {
            case fsByte: *(int *)ptr = (unsigned char)inDefault.ival; break;
            case fsBool: *(int *)ptr = (bool)inDefault.ival; break;
            case fsInt: *(int *)ptr =
                 inDefault.type==CppiaConst::cInt ? inDefault.ival :
                                                    inDefault.dval;
                 break;
            case fsFloat: *(Float *)ptr =
                 inDefault.type==CppiaConst::cInt ? inDefault.ival :
                                                    inDefault.dval;
               break;
            case fsObject:
               {
                  Dynamic &d = *(Dynamic *)ptr;
                  if (inDefault.type==CppiaConst::cInt)
                     d = inDefault.ival;
                  else if (inDefault.type==CppiaConst::cFloat)
                     d = inDefault.dval;
                  else if (inDefault.type==CppiaConst::cString)
                     d =  module->strings[ inDefault.ival ];
               }
               break;
            case fsString:
            case fsUnknown:
               break; // handled above, or not needed
 
         }
      }
   }
}


#ifdef CPPIA_JIT

static int SLJIT_CALL objToInt(hx::Object *inVal)
{
   return inVal->__ToInt();
}
static void SLJIT_CALL objToFloat(hx::Object *inVal,double *outFloat)
{
   *outFloat =  inVal->__ToDouble();
}
static hx::Object *SLJIT_CALL intToObject(int inVal)
{
   return Dynamic(inVal).mPtr;
}
static hx::Object *SLJIT_CALL floatToObject(double * inVal)
{
   return Dynamic(*inVal).mPtr;
}
static hx::Object *SLJIT_CALL stringToObject(String * inVal)
{
   return Dynamic(*inVal).mPtr;
}




void CppiaStackVar::genDefault(CppiaCompiler *compiler, const CppiaConst &inDefault)
{
   if (argType==etString)
   {
      if (inDefault.type == CppiaConst::cString)
      {
         JumpId notNull = compiler->compare(cmpP_NOT_ZERO, JitFramePos(defaultStackPos+StringOffset::Ptr).as(jtPointer),(void *)0);
         String val = module->strings[ inDefault.ival ];
         compiler->move(JitFramePos(defaultStackPos).as(jtInt), (int)val.length);
         compiler->move(JitFramePos(defaultStackPos+StringOffset::Ptr).as(jtPointer), (void *)val.raw_ptr());
         compiler->comeFrom(notNull);
      }
   }
   else
   {
      JitFramePos srcPos(defaultStackPos);
      JitFramePos destPos(stackPos);
      compiler->move( sJitTemp0, srcPos.as(jtPointer) );
      JumpId notNull = compiler->compare(cmpP_NOT_ZERO, sJitTemp0, (void *)0);

      switch(storeType)
      {
         case fsByte:
            compiler->move( destPos.as(jtByte), JitVal((int)(unsigned char)inDefault.ival));
            break;

         case fsBool: 
            compiler->move( destPos.as(jtByte), JitVal((int)(bool)inDefault.ival));
            break;

         case fsInt:
            compiler->move( destPos.as(jtInt), inDefault.ival);
             break;
         case fsFloat:
            compiler->move( sJitTemp0, (void *)&inDefault.dval);
            compiler->move( destPos.as(jtFloat), sJitTemp0.star(jtFloat));
            break;
         case fsObject:
            if (inDefault.type==CppiaConst::cInt)
               compiler->callNative( (void *)intToObject, inDefault.ival );
            else if (inDefault.type==CppiaConst::cFloat)
               compiler->callNative( (void *)floatToObject, (void *)&inDefault.dval );
            else if (inDefault.type==CppiaConst::cString)
               compiler->callNative( (void *)stringToObject, (void *)&module->strings[ inDefault.ival ] );
            else
               break;
            compiler->move(destPos.as(jtPointer), sJitReturnReg.as(jtPointer));
            break;
         case fsString: // handled
         case fsUnknown: // ?
            break;
      }

      JumpId defaultDone = compiler->jump();
      compiler->comeFrom(notNull);

      switch(storeType)
      {
         case fsByte:
         case fsBool: 
            //Fallthough...
            //compiler->callNative( (void *)objToInt, sJitTemp0.as(jtPointer) );
            //compiler->move( destPos.as(jtByte), sJitReturnReg.as(jtByte));
            //compiler->move( destPos.as(jtInt), sJitReturnReg.as(jtInt));
            //break;

         case fsInt:
            compiler->callNative( (void *)objToInt, sJitTemp0.as(jtPointer) );
            compiler->move( destPos.as(jtInt), sJitReturnReg.as(jtInt));
             break;
         case fsFloat:
            compiler->callNative( (void *)objToFloat, sJitTemp0.as(jtPointer), destPos.as(jtFloat)) ;
            break;
         case fsObject:
            // Should be same pos
            //compiler->move(destPos.as(jtPointer), sJitTemp0.as(jtPointer));
            break;
         case fsString: // handled
         case fsUnknown: // ?
            break;
      }

      compiler->comeFrom(defaultDone);
   }

}
#endif



} // end namespace hx
