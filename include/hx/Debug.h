#ifndef HX_DEBUG_H
#define HX_DEBUG_H

namespace hx { struct CallStack; }

void __hx_dump_stack();

void __hxcpp_dbg_var_pop_stack();
void __hxcpp_dbg_var_pop();
void __hxcpp_dbg_var_push(void *inVar, const char *inName, int inType);
void __hxcpp_set_source_pos(const char *inFile, int inLine);


// Do we need to keep a stack trace?
#if (defined(HXCPP_DEBUG) || defined(HXCPP_DEBUG_LOCAL_VARS) || defined(HXCPP_DEBUG_SOURCE_POS)) && !defined(HXCPP_STACK_TRACE)
#define HXCPP_STACK_TRACE
#endif

#ifdef HXCPP_STACK_TRACE

struct __AutoStack
{
   __AutoStack(const char *inName="");
   ~__AutoStack();
};

#define HX_SOURCE_PUSH(name) __AutoStack __autostack(name);
#define HX_BLOCK_PUSH() __AutoStack __autostack;

#else
#define HX_SOURCE_PUSH(x)
#define HX_BLOCK_PUSH()
#endif


#if defined(HXCPP_DEBUG) || defined(HXCPP_DEBUG_SOURCE_POS)
#define HX_SOURCE_POS(a,b) __hxcpp_set_source_pos(a,b);
#else
#define HX_SOURCE_POS(FILE,LINE)
#endif

#ifdef HXCPP_DEBUG_LOCAL_VARS
#define HX_LOCAL_VAR(x,name) __AutoVar __auto_##x(&x,name)
#define HX_LOCAL_THIS(x)     __AutoVar __auto_this(x,"this")
#else
#define HX_LOCAL_VAR(x,name)
#define HX_LOCAL_THIS(x)
#endif

#define HX_LOCAL_ARG(x,name) HX_LOCAL_VAR(x,name)





#ifdef HXCPP_DEBUG_LOCAL_VARS



struct __AutoVar
{
   enum Type { typeObject, typeDouble, typeFloat, typeInt, typeBool, typeString };


   explicit __AutoVar(const __AutoVar &);

   template<typename OBJ>
   inline __AutoVar(hx::CallStack *inStack,OBJ *inPtr,const char *inName)
   {
     __hxcpp_dbg_var_push(inStack,inPtr, inName, typeObject);
   }
   inline __AutoVar(hx::CallStack *inStack,double *inPtr,const char *inName)
   {
      __hxcpp_dbg_var_push(inStack,inPtr, inName, typeDouble);
   }
   inline __AutoVar(hx::CallStack *inStack,float *inPtr,const char *inName)
   {
      __hxcpp_dbg_var_push(inStack,inPtr, inName, typeFloat);
   }
   inline __AutoVar(hx::CallStack *inStack,int *inPtr,const char *inName)
   {
      __hxcpp_dbg_var_push(inStack,inPtr, inName, typeInt);
   }
   inline __AutoVar(hx::CallStack *inStack,bool *inPtr,const char *inName)
   {
      __hxcpp_dbg_var_push(inStack,inPtr, inName, typeBool);
   }
   inline __AutoVar(hx::CallStack *inStack,::String *inPtr,const char *inName)
   {
      __hxcpp_dbg_var_push(inStack,inPtr, inName, typeString);
   }

   inline ~__AutoVar()
   {
      __hxcpp_dbg_var_pop();
   }

};

#endif


#endif
