#ifndef HX_DEBUG_H // {
#define HX_DEBUG_H

namespace hx { struct CallStack; }

void __hx_dump_stack();

Array<String> __hxcpp_dbg_get_stack_vars(int inFrame);


// Track stack variables - only really needed for debugger
#if defined(HXCPP_DEBUGGER) && !defined(HXCPP_STACK_VARS)
#define HXCPP_STACK_VARS
#endif

// Keep track of lines - more accurate stack traces for exceptions
#if (defined(HXCPP_DEBUG) || defined(HXCPP_DEBUGGER)) && !defined(HXCPP_STACK_LINE)
#define HXCPP_STACK_LINE
#endif

// Do we need to keep a stack trace - for basic exception handelling
#if (defined(HXCPP_DEBUG) || defined(HXCPP_STACK_VARS) || defined(HXCPP_STACK_LINE)) && !defined(HXCPP_STACK_TRACE)
#define HXCPP_STACK_TRACE
#endif


#ifdef HXCPP_STACK_TRACE // {

namespace hx
{

#ifdef HXCPP_DEBUGGER
extern int gBreakpoint;
extern void CheckBreakpoint();
#endif

struct AutoVar;

struct CallLocation
{
   const char *mFunction;
   const char *mFile;
   int        mLine; 
   #ifdef HXCPP_STACK_VARS
   AutoVar    *mLocal;
   #endif
};


struct AutoStack
{
   AutoStack(const char *inName, const char *inFile, int inLine);
   ~AutoStack();

   CallLocation *mLocation;
};

}

#define HX_STACK_PUSH(name,file,line) hx::AutoStack __autostack(name,file,line);

#else // } HXCPP_STACK_TRACE {

#define HX_STACK_PUSH(n,f,l)

#endif // }



#ifdef HXCPP_STACK_LINE

#ifdef HXCPP_DEBUGGER
#define HX_STACK_LINE(l) { __autostack.mLocation->mLine=l; if (hx::gBreakpoint) hx::CheckBreakpoint(); }
#else
#define HX_STACK_LINE(l) { __autostack.mLocation->mLine=l; }
#endif


#else
#define HX_STACK_LINE(l)
#endif

#ifdef HXCPP_STACK_VARS // {

#define HX_STACK_VAR(x,name) hx::AutoVar __auto_##x(__autostack.mLocation,&x,name);
#define HX_STACK_THIS(x)     hx::AutoVar __auto_this(__autostack.mLocation,x,"this");

#else // } HXCPP_STACK_VARS {

#define HX_STACK_VAR(x,name)
#define HX_STACK_THIS(x)

#endif // }

#define HX_STACK_ARG(x,name) HX_STACK_VAR(x,name)

#ifdef HXCPP_STACK_VARS // {

namespace hx
{

struct AutoVar
{
   explicit AutoVar(const AutoVar &);

   inline AutoVar(CallLocation *l,void *p, const char *n) : loc(l), ptr(p), name(n)
   {
      next = loc->mLocal;
      loc->mLocal = this;
   }

   inline ~AutoVar() { loc->mLocal = next; }

   void         *ptr;
   const char   *name;
   CallLocation *loc;
   AutoVar      *next;
};






} // end namespace hx

#endif // } HXCPP_STACK_VARS


#endif // } HX_DEBUG_H
