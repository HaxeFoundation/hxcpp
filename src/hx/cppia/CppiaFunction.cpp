   #include <hxcpp.h>
#include "Cppia.h"
#include "CppiaStream.h"

namespace hx
{


static int sTypeSize[] = { 0, 0, sizeof(hx::Object *), sizeof(String), sizeof(Float), sizeof(int) };

// --- ScriptCallable ----------------------------------------------------

String sInvalidArgCount = HX_CSTRING("Invalid arguement count");


#ifdef CPPIA_JIT
void SLJIT_CALL argToInt(CppiaCtx *ctx) { ctx->pushInt( (* (hx::Object **)(ctx->pointer))->__ToInt() ); }
void SLJIT_CALL argToDouble(CppiaCtx *ctx) { ctx->pushFloat( (* (hx::Object **)(ctx->pointer))->__ToDouble() ); }
void SLJIT_CALL argToString(CppiaCtx *ctx) { ctx->pushString( (* (hx::Object **)(ctx->pointer))->__ToString() ); }
#endif



ScriptCallable::ScriptCallable(CppiaStream &stream)
{
   body = 0;
   stackSize = 0;
   data = 0;
   returnTypeId = stream.getInt();
   returnType = etVoid;
   argCount = stream.getInt();
   args.resize(argCount);
   hasDefault.resize(argCount);
   initVals.resize(argCount);
   captureSize = 0;
   #ifdef CPPIA_JIT
   compiled = 0;
   #endif
   for(int a=0;a<argCount;a++)
   {
      args[a].fromStream(stream);
      bool init = stream.getBool();
      hasDefault[a] = init;
      if (init)
         initVals[a].fromStream(stream);
   }
   body = createCppiaExpr(stream);
}

ScriptCallable::ScriptCallable(CppiaExpr *inBody) : CppiaDynamicExpr(inBody)
{
   returnTypeId = 0;
   returnType = etVoid;
   argCount = 0;
   stackSize = 0;
   captureSize = 0;
   body = inBody;
   #ifdef CPPIA_JIT
   compiled = 0;
   #endif
}

ScriptCallable::~ScriptCallable()
{
   #ifdef CPPIA_JIT
   if (compiled)
      CppiaCompiler::freeCompiled(compiled);
   #endif
}

CppiaExpr *ScriptCallable::link(CppiaModule &inModule)
{
   StackLayout *oldLayout = inModule.layout;
   StackLayout layout(oldLayout);
   inModule.layout = &layout;
   data = &inModule;

   returnType = inModule.types[ returnTypeId ]->expressionType;
   layout.returnType = returnType;

   for(int a=0;a<args.size();a++)
      args[a].link(inModule);

   body = body->link(inModule);

   captureVars.swap(layout.captureVars);
   captureSize = layout.captureSize;

   #ifdef HXCPP_STACK_SCRIPTABLE
   std::swap(varMap,layout.varMap);
   #endif

   stackSize = layout.size;
   inModule.layout = oldLayout;

   position.className = className;
   position.functionName = functionName;
   position.fileName = filename;
   position.fullName = filename;

   #ifdef HXCPP_DEBUGGER
   position.fileHash = Hash(0,filename);
   int hash = Hash(0,className);
   hash = Hash(hash,".");
   position.classFuncHash = Hash(hash,functionName);
   #endif

   return this;
}

#ifdef HXCPP_STACK_SCRIPTABLE
void ScriptCallable::getScriptableVariables(unsigned char *inFrame, Array<Dynamic> outNames)
{
   hx::Object *thizz = *(hx::Object **)inFrame;
   if (thizz)
      outNames->push(HX_CSTRING("this"));

   for(CppiaStackVarMap::iterator i=varMap.begin();i!=varMap.end();++i)
   {
      CppiaStackVar *var = i->second;
      outNames->push(var->module->strings[ var->nameId] );
   }
}


bool ScriptCallable::getScriptableValue(unsigned char *inFrame, String inName, ::Dynamic &outValue)
{
   if (inName.length==4 && !strcmp(inName.__s,"this"))
   {
      outValue = *(hx::Object **)inFrame;
      return true;
   }


   for(CppiaStackVarMap::iterator i=varMap.begin();i!=varMap.end();++i)
   {
      CppiaStackVar *var = i->second;
      if ( var->module->strings[ var->nameId]==inName)
      {
         outValue = var->getInFrame(inFrame);
         return true;
      }
   }

   return false;
}


bool ScriptCallable::setScriptableValue(unsigned char *inFrame, String inName, ::Dynamic inValue)
{
   for(CppiaStackVarMap::iterator i=varMap.begin();i!=varMap.end();++i)
   {
      CppiaStackVar *var = i->second;
      if ( var->module->strings[ var->nameId]==inName)
      {
         var->setInFrame(inFrame,inValue);
         return true;
      }
   }

   return false;
}


#endif


int ScriptCallable::Hash(int value, const char *inString)
{
   if (inString)
      while(*inString)
         value = value*223 + *inString++;
   return value;
}


#ifdef CPPIA_JIT


int SLJIT_CALL objectToInt(hx::Object *obj) { return obj->__ToInt(); }
void SLJIT_CALL frameToDouble(CppiaCtx *inCtx) { inCtx->returnFloat( inCtx->getObject() ); }
//void objectToInt(CppiaCtx *inCtx) { inCtx->returnInt( inCtx->getObject() ); }
void SLJIT_CALL objectToDouble(CppiaCtx *inCtx) { inCtx->returnFloat( inCtx->getObject() ); }
void SLJIT_CALL objectToDoublePointer(CppiaCtx *inCtx)
{
   *(double *)inCtx->pointer = (*(hx::Object **)inCtx->pointer)->__ToDouble();
}
void SLJIT_CALL objectToStringPointer(CppiaCtx *inCtx)
{
   *(String *)inCtx->pointer = (*(hx::Object **)inCtx->pointer)->toString();
}
void SLJIT_CALL objectToString(CppiaCtx *inCtx) { inCtx->returnString( inCtx->getObject() ); }
void SLJIT_CALL stringToObject(CppiaCtx *inCtx) { inCtx->returnObject( inCtx->getString() ); }
void SLJIT_CALL intToObject(CppiaCtx *inCtx) { inCtx->returnObject( inCtx->getInt() ); }
void SLJIT_CALL doubleToObject(CppiaCtx *inCtx) { inCtx->returnObject( inCtx->getFloat() ); }


void ScriptCallable::genPushDefault(CppiaCompiler *compiler, int inArg, bool pushNullToo)
{
   int framePos = compiler->getCurrentFrameSize();
   JitFramePos target(framePos);
   CppiaStackVar &var = args[inArg];
   switch(var.expressionType)
   {
       case etInt:
          compiler->move( target, JitVal(initVals[inArg].ival) );
          break;
       case etFloat:
          // TODO - dpointer, not dval
          compiler->move( target, JitVal(&initVals[inArg].dval));
          break;
       case etString:
          {
             const String &str = data->strings[ initVals[inArg].ival ];
             compiler->move(target, JitVal(str.length) );
             compiler->move(target+sizeof(int), JitVal((void *)str.__s) );
          }
          break;
       default:
          {
             // TODO : GC on CppiaConst Dynamixc...
             Dynamic val;
             switch(initVals[inArg].type)
             {
                 case CppiaConst::cInt: val = initVals[inArg].ival; break;
                 case CppiaConst::cFloat: val = initVals[inArg].dval; break;
                 case CppiaConst::cString: val = data->strings[ initVals[inArg].ival ]; break;
                 default: ;
             }
             if (val.mPtr || pushNullToo)
             {
                compiler->move( target, JitVal((void *)val.mPtr) );
             }
          }
    }
}






void ScriptCallable::genArgs(CppiaCompiler *compiler, CppiaExpr *inThis, Expressions &inArgs, const JitVal &inThisVal)
{
   int inCount = inArgs.size();
   bool badCount = argCount<inCount;

   for(int i=inCount;i<argCount && !badCount;i++)
      if (!hasDefault[i])
         badCount = true;

   if (badCount)
   {
      printf("Arg count mismatch %d!=%d ?\n", argCount, (int)inArgs.size());
      printf(" %s at %s:%d %s\n", getName(), filename, line, functionName);
      CPPIA_CHECK(0);
      throw Dynamic(HX_CSTRING("Arg count error"));
      //return;
   }


   int framePos = compiler->getCurrentFrameSize();
   // Push this ...
   if (inThis)
   {
      inThis->genCode(compiler, JitFramePos(framePos), etObject);
   }
   else if (inThisVal.valid())
   {
      compiler->move( JitFramePos(framePos), inThisVal );
   }
   else
   {
      compiler->move( JitFramePos(framePos), JitVal( (void *)0 ));
   }
   compiler->addFrame(etObject);


   for(int a=0;a<argCount;a++)
   {
      CppiaStackVar &var = args[a];
      // TODO capture
      if (hasDefault[a])
      {
         if (a>=inCount)
         {
            genPushDefault(compiler,a,true);
         }
         else
         {
             // Gen Object onto frame ...
             int framePos = compiler->getCurrentFrameSize();

             if (var.expressionType==etInt)
             {
                inArgs[a]->genCode(compiler, sJitArg0, etObject );

                // Check for null
                JumpId notNull = compiler->notNull(sJitArg0);

                // Is  null ...
                genPushDefault(compiler,a,false);

                JumpId doneArg = compiler->jump();

                compiler->comeFrom(notNull);

                // sJitArg0 holds object
                compiler->callNative( objectToInt, sJitArg0);
                compiler->move( JitFramePos(framePos), sJitReturnReg );

                compiler->comeFrom(doneArg);
             }
             else if (var.expressionType==etObject)
             {
                inArgs[a]->genCode(compiler, JitFramePos(framePos), etObject );
             }
             else if  (var.expressionType==etString)
             {
                inArgs[a]->genCode(compiler, JitFramePos(framePos), etString );

                JumpId notNull = compiler->compare(cmpP_NOT_EQUAL, JitFramePos(framePos + sizeof(int)), (void *)0 );

                // Is  null ...
                genPushDefault(compiler,a,false);

                compiler->comeFrom(notNull);
             }
             else // Float
             {
                inArgs[a]->genCode(compiler, sJitTemp0, etObject );

                // Check for null
                JumpId notNull = compiler->compare(cmpP_NOT_EQUAL, sJitTemp0, (void *)0);

                // Is  zero ...
                genPushDefault(compiler,a,false);
                JumpId doneArg = compiler->jump();

                compiler->comeFrom(notNull);

                compiler->move( JitFramePos(framePos), sJitTemp0 );
                compiler->add( sJitTemp0, sJitFrame, framePos );
                compiler->callNative( (void *)objectToDouble, sJitTemp0 );

                compiler->comeFrom(doneArg);
             }
          }
      }
      else
      {
         int framePos = compiler->getCurrentFrameSize();
         inArgs[a]->genCode(compiler, JitFramePos(framePos).as( getJitType(var.type->expressionType) ), var.type->expressionType);
      }
      compiler->addFrame(var.expressionType);
   }
}
#endif


void ScriptCallable::pushArgs(CppiaCtx *ctx, hx::Object *inThis, Expressions &inArgs)
{
   BCR_VCHECK;
   int inCount = inArgs.size();
   bool badCount = argCount<inCount;

   for(int i=inCount;i<argCount && !badCount;i++)
      if (!hasDefault[i])
         badCount = true;

   if (badCount)
   {
      printf("Arg count mismatch %d!=%d ?\n", argCount, (int)inArgs.size());
      printf(" %s at %s:%d %s\n", getName(), filename, line, functionName);
      CPPIA_CHECK(0);
      throw Dynamic(HX_CSTRING("Arg count error"));
      //return;
   }


   ctx->push( inThis );

   for(int a=0;a<argCount;a++)
   {
      CppiaStackVar &var = args[a];
      // TODO capture
      if (hasDefault[a])
      {
         bool makeNull = a>=inCount;

         if (var.expressionType == etString)
         {
            String val;
            if (!makeNull)
               val = inArgs[a]->runString(ctx);
            if (val==null() && initVals[a].type==CppiaConst::cString)
               val = data->strings[ initVals[a].ival ];

            BCR_VCHECK;
            ctx->pushString(val);
            continue;
         }

         hx::Object *obj = makeNull ? 0 : inArgs[a]->runObject(ctx);
         BCR_VCHECK;
         switch(var.expressionType)
         {
            case etInt:
               ctx->pushInt( obj ? obj->__ToInt() : initVals[a].ival );
               break;
            case etFloat:
               ctx->pushFloat( (Float)(obj ? obj->__ToDouble() : initVals[a].dval) );
               break;
            case etString: // Handled above
               break;
            default:
               if (obj)
                  ctx->pushObject(obj);
               else
               {
                  switch(initVals[a].type)
                  {
                     case CppiaConst::cInt:
                        ctx->pushObject( Dynamic(initVals[a].ival).mPtr );
                        break;
                     case CppiaConst::cFloat:
                        ctx->pushObject( Dynamic(initVals[a].dval).mPtr );
                        break;
                     case CppiaConst::cString:
                        ctx->pushObject( Dynamic(data->strings[ initVals[a].ival ]).mPtr );
                        break;
                     default:
                        ctx->pushObject(null());
                  }

               }
         }
      }
      else
      {
         switch(var.expressionType)
         {
            case etInt:
               ctx->pushInt(inArgs[a]->runInt(ctx));
               break;
            case etFloat:
               ctx->pushFloat(inArgs[a]->runFloat(ctx));
               break;
            case etString:
               ctx->pushString(inArgs[a]->runString(ctx));
               break;
            default:
               ctx->pushObject(inArgs[a]->runObject(ctx));
         }
         BCR_VCHECK;
      }
   }
}



void ScriptCallable::pushArgsDynamic(CppiaCtx *ctx, hx::Object *inThis, Array<Dynamic> &inArgs)
{
   BCR_VCHECK;

   int inCount = inArgs==null() ? 0 : inArgs->length;
   /*
   bool badCount = argCount<inCount;
   for(int i=inCount;i<argCount && !badCount;i++)
      if (!hasDefault[i])
         badCount = true;

   if (badCount)
   {
      printf("Dynamic Arg count mismatch %d!=%d ?\n", argCount, inCount);
      printf(" %s at %s:%d %s\n", getName(), filename, line, functionName);
      CPPIA_CHECK(0);
      throw Dynamic(HX_CSTRING("Arg count error"));
      //return;
   }
   */


   ctx->push( inThis );

   for(int a=0;a<argCount;a++)
   {
      CppiaStackVar &var = args[a];
      // TODO capture
      if (hasDefault[a])
      {
         hx::Object *obj = a<inCount ?inArgs[a].mPtr : 0;
         BCR_VCHECK;
         switch(var.expressionType)
         {
            case etInt:
               ctx->pushInt( obj ? obj->__ToInt() : initVals[a].ival );
               break;
            case etFloat:
               ctx->pushFloat( (Float)(obj ? obj->__ToDouble() : initVals[a].dval) );
               break;
            case etString:
               ctx->pushString( obj ? obj->toString() :
                           initVals[a].type == CppiaConst::cNull ? String() :
                             data->strings[ initVals[a].ival ] );
               break;

            default:
               if (obj)
                  ctx->pushObject(obj);
               else
               {
                  switch(initVals[a].type)
                  {
                     case CppiaConst::cInt:
                        ctx->pushObject( Dynamic(initVals[a].ival).mPtr );
                        break;
                     case CppiaConst::cFloat:
                        ctx->pushObject( Dynamic(initVals[a].dval).mPtr );
                        break;
                     case CppiaConst::cString:
                        ctx->pushObject( Dynamic(data->strings[ initVals[a].ival ]).mPtr );
                        break;
                     default:
                        ctx->pushObject(null());
                  }

               }
         }
      }
      else if (a<inCount)
      {
         switch(var.expressionType)
         {
            case etInt:
               ctx->pushInt(inArgs[a]);
               break;
            case etFloat:
               ctx->pushFloat(inArgs[a]);
               break;
            case etString:
               ctx->pushString(inArgs[a]);
               break;
            default:
               ctx->pushObject(inArgs[a].mPtr);
         }
         BCR_VCHECK;
      }
      else
      {
         // Push cpp defaults...
         switch(var.expressionType)
         {
            case etInt:
               ctx->pushInt(0);
               break;
            case etFloat:
               ctx->pushFloat(0.0);
               break;
            case etString:
               ctx->pushString(String());
               break;
            default:
               ctx->pushObject(0);
         }
 
      }
   }
}


// Return the closure
hx::Object *ScriptCallable::runObject(CppiaCtx *ctx)
{
   return createClosure(ctx,this);
}

const char *ScriptCallable::getName() { return "ScriptCallable"; }
String ScriptCallable::runString(CppiaCtx *ctx) { return HX_CSTRING("#function"); }

void ScriptCallable::runVoid(CppiaCtx *ctx) { }

// Run the actual function
void ScriptCallable::runFunction(CppiaCtx *ctx)
{
   CPPIA_STACK_FRAME(this);

   #ifdef CPPIA_JIT
   if (compiled)
   {
      //printf("Running compiled code...\n");
      compiled(ctx);
      //printf("Done.\n");
   }
   else
   #endif
   {
      if (stackSize)
      {
         memset(ctx->pointer, 0 , stackSize );
         ctx->pointer += stackSize;
      }
      CPPIA_STACK_LINE(this);
      body->runVoid(ctx);
   }
}

void ScriptCallable::addStackVarsSpace(CppiaCtx *ctx)
{
   if (stackSize)
   {
      memset(ctx->pointer, 0 , stackSize );
      ctx->pointer += stackSize;
   }
}


bool ScriptCallable::pushDefault(CppiaCtx *ctx,int arg)
{
   if (!hasDefault[arg])
      return false;

   switch(args[arg].expressionType)
   {
      case etInt:
         if (initVals[arg].type==CppiaConst::cFloat)
            ctx->pushInt( initVals[arg].dval );
         else
            ctx->pushInt( initVals[arg].ival );
         break;
      case etFloat:
         if (initVals[arg].type==CppiaConst::cFloat)
            ctx->pushFloat( initVals[arg].dval );
         else
            ctx->pushFloat( initVals[arg].ival );
         break;
      case etString:
         ctx->pushString( initVals[arg].type == CppiaConst::cNull ? String() : data->strings[initVals[arg].ival] );
         break;
      default:
         switch(initVals[arg].type)
         {
            case CppiaConst::cInt:
               ctx->pushObject( Dynamic(initVals[arg].ival).mPtr );
               break;
            case CppiaConst::cFloat:
               ctx->pushObject( Dynamic(initVals[arg].dval).mPtr );
               break;
            case CppiaConst::cString:
               ctx->pushObject( Dynamic(data->strings[ initVals[arg].ival ]).mPtr );
               break;
            default:
               ctx->pushObject(null());
         }

   }
   return true;
}

void ScriptCallable::addExtraDefaults(CppiaCtx *ctx,int inHave)
{
   if (inHave>argCount)
   {
      return;
      //throw sInvalidArgCount;
   }

   for(int a=inHave;a<argCount;a++)
   {
      CppiaStackVar &var = args[a];
      if (!pushDefault(ctx,a))
      {
         return;
         throw sInvalidArgCount;
      }
   }
}


#ifdef CPPIA_JIT
void ScriptCallable::compile()
{
   if (!compiled && body)
   {
      CppiaCompiler *compiler = CppiaCompiler::create(stackSize);

      // First pass calculates size...
      body->genCode(compiler);

      compiler->beginGeneration(1);

      // Second pass does the job
      body->genCode(compiler);

      compiled = compiler->finishGeneration();

      delete compiler;
   }
}
#endif

// --- CppiaClosure ----



class CppiaClosure : public hx::Object
{
public:
   inline void *operator new( size_t inSize, int inExtraDataSize )
     { return hx::InternalNew(inSize + inExtraDataSize,true); }
   inline void operator delete(void *,int) {}

   ScriptCallable *function;

   CppiaClosure(CppiaCtx *ctx, ScriptCallable *inFunction)
   {
      function = inFunction;

      unsigned char *base = ((unsigned char *)this) + sizeof(CppiaClosure);

      *(hx::Object **)base = ctx->getThis(false);

      for(int i=0;i<function->captureVars.size();i++)
      {
         CppiaStackVar *var = function->captureVars[i];
         int size = sTypeSize[var->expressionType];
         memcpy( base+var->capturePos, ctx->frame + var->fromStackPos, size );
      }
   }

   hx::Object **getThis() const
   {
      unsigned char *base = ((unsigned char *)this) + sizeof(CppiaClosure);
      return (hx::Object **)base;
   }

   // Create member closure...
   CppiaClosure(hx::Object *inThis, ScriptCallable *inFunction)
   {
      function = inFunction;
      *getThis() = inThis;
   }


   Dynamic doRun(CppiaCtx *ctx, int inHaveArgs)
   {
      function->addExtraDefaults(ctx,inHaveArgs);
      function->addStackVarsSpace(ctx);

      unsigned char *base = ((unsigned char *)this) + sizeof(CppiaClosure);
      *(hx::Object **)ctx->frame =  *(hx::Object **)base;

      for(int i=0;i<function->captureVars.size();i++)
      {
         CppiaStackVar *var = function->captureVars[i];
         int size = sTypeSize[var->expressionType];
         memcpy( ctx->frame+var->stackPos, base + var->capturePos, size);
      }

      switch(function->returnType)
      {
         case etFloat:
            return ctx->runFloat( function );
         case etInt:
            return ctx->runInt( function );
         case etString:
            return ctx->runString( function );
         case etObject:
            return ctx->runObject( function );
         default: break;
      }
      ctx->runVoid( function );
      return null();
   }

   void pushArgDynamic(CppiaCtx *ctx, int a, Dynamic inValue)
   {
      // Developer has used dynamic to call a closure with wrong # parameters
      if (a>=function->args.size())
         return;

      if (!inValue.mPtr && function->pushDefault(ctx,a) )
          return;

      switch(function->args[a].expressionType)
      {
         case etInt:
            ctx->pushInt(inValue->__ToInt());
            return;
         case etFloat:
            ctx->pushFloat(inValue->__ToDouble());
            break;
         case etString:
            ctx->pushString(inValue.mPtr ? inValue->toString() : String());
            break;
         default:
            {
               hx::Object *value = inValue.mPtr;
               if (value)
               {
                  ArrayType want = function->args[a].type->arrayType;
                  if (want!=arrNotArray)
                     value = DynamicToArrayType(value, want);
               }
               ctx->pushObject(value);
            }
      }
   }



   Dynamic __Run(const Array<Dynamic> &inArgs)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();

      AutoStack a(ctx);
      ctx->pointer += sizeof(hx::Object *);

      int haveArgs = !inArgs.mPtr ? 0 : inArgs->length;
      if (haveArgs>function->argCount)
         throw sInvalidArgCount;

      for(int a=0; a<haveArgs; a++)
         pushArgDynamic(ctx,a,inArgs[a]);

      return doRun(ctx,haveArgs);
   }

   Dynamic __run()
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack a(ctx);
      ctx->pointer += sizeof(hx::Object *);
      return doRun(ctx,0);
   }

   Dynamic __run(D a)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      return doRun(ctx,1);
   }
   Dynamic __run(D a,D b)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      pushArgDynamic(ctx,1,b);
      return doRun(ctx,2);
   }
   Dynamic __run(D a,D b,D c)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      pushArgDynamic(ctx,1,b);
      pushArgDynamic(ctx,2,c);
      return doRun(ctx,3);
   }
   Dynamic __run(D a,D b,D c,D d)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      pushArgDynamic(ctx,1,b);
      pushArgDynamic(ctx,2,c);
      pushArgDynamic(ctx,3,d);
      return doRun(ctx,4);

   }
   Dynamic __run(D a,D b,D c,D d,D e)
   {
      CppiaCtx *ctx = CppiaCtx::getCurrent();
      AutoStack aut(ctx);
      ctx->pointer += sizeof(hx::Object *);
      pushArgDynamic(ctx,0,a);
      pushArgDynamic(ctx,1,b);
      pushArgDynamic(ctx,2,c);
      pushArgDynamic(ctx,3,d);
      pushArgDynamic(ctx,4,e);
      return doRun(ctx,5);
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(*getThis());
      char *base = ((char *)this) + sizeof(CppiaClosure);
      for(int i=0;i<function->captureVars.size();i++)
         function->captureVars[i]->markClosure(base,__inCtx);
   }
#ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(*getThis());
      char *base = ((char *)this) + sizeof(CppiaClosure);
      for(int i=0;i<function->captureVars.size();i++)
         function->captureVars[i]->visitClosure(base,__inCtx);
   }
#endif
   virtual void *__GetHandle() const { return *getThis(); }

   int __Compare(const hx::Object *inRHS) const
   {
      const CppiaClosure *other = dynamic_cast<const CppiaClosure *>(inRHS);
      if (!other)
         return -1;
      return (function==other->function && *getThis()==*other->getThis())? 0 : -1;
   }



   int __GetType() const { return vtFunction; }
   int __ArgCount() const { return function->args.size(); }

   String toString() { return HX_CSTRING("function"); }
};

hx::Object *createClosure(CppiaCtx *ctx, ScriptCallable *inFunction)
{
   return new (inFunction->captureSize) CppiaClosure(ctx,inFunction);
}

hx::Object *createMemberClosure(hx::Object *inThis, ScriptCallable *inFunction)
{
   return new (sizeof(hx::Object *)) CppiaClosure(inThis,inFunction);
}






// --- CppiaFunction ----

CppiaFunction::CppiaFunction(CppiaModule *inCppia,bool inIsStatic,bool inIsDynamic) :
   cppia(*inCppia), isStatic(inIsStatic), isDynamic(inIsDynamic), funExpr(0)
{
   linked = false;
   vtableSlot = -1;
}

void CppiaFunction::load(CppiaStream &stream,bool inExpectBody)
{
   nameId = stream.getInt();
   name = cppia.strings[ nameId ].__s;
   stream.module->creatingFunction = name.c_str();
   returnType = stream.getInt();
   argCount = stream.getInt();
   DBGLOG("  Function %s(%d) : %s %s%s\n", name.c_str(), argCount, cppia.typeStr(returnType), isStatic?"static":"instance", isDynamic ? " DYNAMIC": "");
   args.resize(argCount);
   for(int a=0;a<argCount;a++)
   {
      ArgInfo arg = args[a];
      arg.nameId = stream.getInt();
      arg.optional = stream.getBool();
      arg.typeId = stream.getInt();
      DBGLOG("    arg %c%s:%s\n", arg.optional?'?':' ', cppia.identStr(arg.nameId), cppia.typeStr(arg.typeId) );
   }
   if (inExpectBody)
      funExpr = (ScriptCallable *)createCppiaExpr(stream);
   stream.module->creatingFunction = 0;
}

void CppiaFunction::link( )
{
   if (!linked)
   {
      linked = true;
      if (funExpr)
         funExpr = (ScriptCallable *)(funExpr->link(cppia));
   }
}

#ifdef CPPIA_JIT
void CppiaFunction::compile()
{
   if (funExpr)
      funExpr->compile();
}
#endif





};
