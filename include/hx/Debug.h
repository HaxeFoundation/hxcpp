#ifndef HX_DEBUG_H // {
#define HX_DEBUG_H

namespace hx { struct CallStack; }

HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hx_dump_stack();

Array<String> __hxcpp_dbg_get_stack_vars(int inFrame);
void __hxcpp_dbg_set_stack_var(int inFrame,String inVar, Dynamic inValue);
Dynamic __hxcpp_dbg_get_stack_var(int inFrame,String inVar);
void __hxcpp_breakpoints_add(int inFrame);

HXCPP_EXTERN_CLASS_ATTRIBUTES
void __hxcpp_stack_begin_catch();

void __hxcpp_dbg_set_handler(Dynamic inHandler);
void __hxcpp_dbg_set_thread(Dynamic inThread);
void __hxcpp_dbg_set_break(int inMode);
void __hxcpp_breakpoints_add(int inFileId, int inLine);
bool __hxcpp_dbg_handle_error(::String inError);
Array<String> __hxcpp_dbg_breakpoints_get( );
void __hxcpp_dbg_breakpoints_delete(int inIndex);
Array<Dynamic> __hxcpp_dbg_stack_frames_get( );
Array<Dynamic> __hxcpp_dbg_get_files( );


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

   #ifdef HXCPP_DEBUGGER
   unsigned short mBPVersion;
	bool           mBPOnFile;
   #endif

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

// Code from old compiler ...
#define HX_SOURCE_PUSH(name)  hx::AutoStack __autostack(name,0,0);
#define HX_SOURCE_POS(file,line)

// Code from new compiler ...
#define HX_STACK_PUSH(name,file,line) hx::AutoStack __autostack(name,file,line);

#define HX_STACK_BEGIN_CATCH __hxcpp_stack_begin_catch();

#else // } HXCPP_STACK_TRACE {

#define HX_STACK_PUSH(n,f,l)
// Backwards compatibility
#define HX_SOURCE_PUSH(name)
#define HX_SOURCE_POS(file,line)

#define HX_STACK_BEGIN_CATCH

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
#define HX_STACK_THIS(x)     hx::AutoThis __auto_this(__autostack.mLocation,x,"this");

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
	typedef Dynamic (*HandleFunc)(void *ptr,Dynamic,bool get);

   explicit AutoVar(const AutoVar &);

   template<typename T>
   inline AutoVar(CallLocation *l,T *p, const char *n) : loc(l), ptr(p), name(n)
   {
      next = loc->mLocal;
      loc->mLocal = this;
      handler = THandle<T>;
   }

   template<typename T>
   inline AutoVar(CallLocation *l,T *p, const char *n, bool /*dummy*/) : loc(l), ptr(p), name(n)
   {
      next = loc->mLocal;
      loc->mLocal = this;
   }


   Dynamic get() { return handler(ptr,null(),true); }
   void    set(Dynamic inValue) { handler(ptr,inValue,false); }

   template<typename T>
   static Dynamic THandle(void *inPtr,Dynamic inValue, bool inGet)
   {
		if (inGet)
			return * ( (T*)inPtr );
		else
         * (T*)inPtr = inValue;
		return null();
   }

   inline ~AutoVar() { loc->mLocal = next; }


   void         *ptr;
   const char   *name;
   CallLocation *loc;
   AutoVar      *next;
   HandleFunc   handler;
};

struct AutoThis : public AutoVar
{
   template<typename T>
   inline AutoThis(CallLocation *l,T *p, const char *n) : AutoVar(l,p,n,false)
   {
      handler = TThisHandler<T>;
   }

   template<typename T>
   static Dynamic TThisHandler(void *inPtr,Dynamic inValue, bool inGet)
   {
		if (inGet)
			return (T*)inPtr;
		else
         Throw("Can't set this pointer.");
		return null();
   }
};



} // end namespace hx

#endif // } HXCPP_STACK_VARS


#endif // } HX_DEBUG_H
