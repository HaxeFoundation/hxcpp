#include <hx/Scriptable.h>
#include <hx/GC.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>


namespace hx
{

struct CppiaData;

struct CppiaStream
{
   CppiaData  *cppiaData;
   const char *data;
   const char *max;
   int line;
   int pos;

   CppiaStream(CppiaData *inCppiaData,const char *inData, int inLen)
   {
      cppiaData = inCppiaData;
      data = inData;
      max = inData + inLen;
      line = 1;
      pos = 1;
   }
   void skipWhitespace()
   {
      while(data<max && *data<=32)
         skipChar();
   }
   void skipChar()
   {
      if (data>=max)
         throw "EOF";
      if (*data=='\n')
      {
         line++;
         pos = 1;
      }
      else
         pos++;
      data++;
   }
   std::string getToken()
   {
      skipWhitespace();
      const char *data0 = data;
      while(data<max && *data>32)
      {
         data++;
         pos++;
      }
      return std::string(data0,data);
   }
   int getInt()
   {
      int result = 0;
      skipWhitespace();
      while(data<max && *data>32)
      {
         int digit = *data - '0';
         if (digit<0 || digit>9)
            throw "expected digit";
         result = result * 10 + digit;
         pos++;
         data++;
      }
      return result;
   }
   bool getBool()
   {
      int ival = getInt();
      if (ival>1)
         throw "invalid bool";
      return ival;
   }
   bool getStatic()
   {
      std::string tok = getToken();
      if (tok=="s")
         return true;
      else if (tok=="m")
         return false;

      throw "invalid function spec";
      return false;
   }

   String readString()
   {
      int len = getInt();
      skipChar();
      const char *data0 = data;
      for(int i=0;i<len;i++)
         skipChar();
      return String(data0,data-data0).dup();
   }

   void readBytes(unsigned char *outBytes, int inLen)
   {
      if (data+inLen>max)
         throw "EOF";
      memcpy(outBytes, data, inLen);
      data+=inLen;
   }

};


struct TypeData;
struct CppiaClassInfo;
struct CppiaExpr;
class  ScriptRegistered;


enum ExprType
{
  etVoid,
  etNull,
  etObject,
  etString,
  etFloat,
  etInt,
};

enum ArrayType
{
   arrNotArray = 0,
   arrBool,
   arrInt,
   arrFloat,
   arrUnsignedChar,
   arrString,
   arrObject,
   arrAny,
};

enum CrementOp
{
   coNone = -1,
   coPreInc,
   coPostInc,
   coPreDec,
   coPostDec,
};

enum AssignOp
{
   aoNone = -1,
   aoSet,
   aoAdd,
   aoMult,
   aoDiv,
   aoSub,
   aoAnd,
   aoOr,
   aoXOr,
   aoShl,
   aoShr,
   aoUShr,
   aoMod
};




struct CppiaExpr
{
   int line;
   const char *filename;
   const char *className;
   const char *functionName;
   int haxeTypeId;


   CppiaExpr() : line(0), filename(0), className(0), functionName(0)
   {
   }
   CppiaExpr(const CppiaExpr *inSrc) : line(0), filename(0), className(0), functionName(0)
   {
      if (inSrc)
      {
         line = inSrc->line;
         filename = inSrc->filename;
         className = inSrc->className;
         functionName = inSrc->functionName;
         haxeTypeId = inSrc->haxeTypeId;
      }
   }

   virtual ~CppiaExpr() {}

   virtual const char *getName() { return "CppiaExpr"; }
   virtual CppiaExpr   *link(CppiaData &data)   { return this; }
   virtual void mark(hx::MarkContext *ctx) { };
   virtual void visit(hx::VisitContext *ctx) { };


   virtual ExprType    getType() { return etObject; }

   virtual int         runInt(CppiaCtx *ctx)    { return runFloat(ctx); }
   virtual Float       runFloat(CppiaCtx *ctx) { return 0.0; }
   virtual ::String    runString(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      if (!result)
         return null();
      return result->toString();
   }
   virtual hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(runFloat(ctx)).mPtr; }
   virtual void        runVoid(CppiaCtx *ctx)   { runObject(ctx); }
   virtual CppiaExpr   *makeSetter(AssignOp op,CppiaExpr *inValue) { return 0; }
   virtual CppiaExpr   *makeCrement(CrementOp inOp) { return 0; }

};

#ifdef HXCPP_STACK_LINE
   #define CPPIA_STACK_LINE(expr) \
          __hxcpp_set_stack_frame_line(expr->line);
#else
   #define CPPIA_STACK_LINE(expr)
#endif

#define CPPIA_STACK_FRAME(expr) \
 HX_STACK_FRAME(expr->className, expr->functionName, 0, expr->className, expr->filename, expr->line, 0);


#ifdef HXCPP_CHECK_POINTER
   #ifdef HXCPP_GC_CHECK_POINTER
      #define CPPIA_CHECK(obj) if (!obj) NullReference("Object", false);  GCCheckPointer(obj);
   #else
      #define CPPIA_CHECK(obj) if (!obj) NullReference("Object", false);
   #endif
#else
   #define CPPIA_CHECK(obj)
#endif

#ifdef HXCPP_CHECK_POINTER
   #define CPPIA_CHECK_FUNC(obj) if (!obj) Dynamic::ThrowBadFunctionError(); GCCheckPointer(obj);
#else
   #define CPPIA_CHECK_FUNC(obj) if (!obj) Dynamic::ThrowBadFunctionError();
#endif



struct BCRReturn
{
   inline BCRReturn() {}
   inline operator hx::Object *() const {  return 0; }
   inline operator bool() const {  return false; }
   inline operator int() const {  return 0; }
   inline operator unsigned char() const {  return 0; }
   inline operator char() const {  return 0; }
   inline operator short() const {  return 0; }
   inline operator unsigned short() const {  return 0; }
   inline operator float() const {  return 0; }
   inline operator double() const {  return 0; }
   inline operator Dynamic() const {  return null(); }
   inline operator String() const {  return String(); }
};


#define BCR_CHECK if (ctx->breakContReturn) return BCRReturn();
#define BCR_CHECK_RET(x) if (ctx->breakContReturn) return x;
#define BCR_VCHECK if (ctx->breakContReturn) return;



typedef std::vector<CppiaExpr *> Expressions;

CppiaExpr *createArrayBuiltin(CppiaExpr *inSrc, ArrayType inType, CppiaExpr *inThisExpr,
                              String inField, Expressions &ioExpressions );
CppiaExpr *createStringBuiltin(CppiaExpr *src, CppiaExpr *inThisExpr, String field, Expressions &ioExpressions );

CppiaExpr *createGlobalBuiltin(CppiaExpr *src, String function, Expressions &ioExpressions );

template<typename T>
inline T &runValue(T& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   expr->runVoid(ctx);
   return null();
}

template<> inline int &runValue(int& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runInt(ctx);
}


template<> inline unsigned char &runValue(unsigned char& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runInt(ctx);
}

template<> inline bool &runValue(bool& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runInt(ctx);
}


template<> inline Float &runValue(Float& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runFloat(ctx);
}

template<> inline String &runValue(String& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runString(ctx);
}

template<> inline hx::Object * &runValue(hx::Object *& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runObject(ctx);
}

template<> inline Dynamic &runValue(Dynamic & outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runObject(ctx);
}




inline static int ValToInt( bool v ) { return v; }
inline static int ValToInt( int v ) { return v; }
inline static int ValToInt( const Float &v ) { return v; }
inline static int ValToInt( const String &v ) { return 0; }
inline static int ValToInt( hx::Object *v ) { return v ? v->__ToInt() : 0; }
inline static int ValToInt( const Dynamic &v ) { return v.mPtr ? v->__ToInt() : 0; }

inline static Float ValToFloat( const bool &v ) { return v; }
inline static Float ValToFloat( const int &v ) { return v; }
inline static Float ValToFloat( const unsigned char &v ) { return v; }
inline static Float ValToFloat( const Float &v ) { return v; }
inline static Float ValToFloat( const String &v ) { return 0; }
inline static Float ValToFloat( hx::Object *v ) { return v ? v->__ToDouble() : 0; }
inline static Float ValToFloat( const Dynamic &v ) { return v.mPtr ? v->__ToDouble() : 0.0; }

inline static String ValToString( const bool &v ) { return v; }
inline static String ValToString( const int &v ) { return String(v); }
inline static String ValToString( const unsigned char &v ) { return String(v); }
inline static String ValToString( const Float &v ) { return String(v); }
inline static String ValToString( const String &v ) { return v; }
inline static String ValToString( hx::Object *v ) { return v ? ((hx::Object *)v)->toString() : String(); }
inline static String ValToString( const Dynamic &v ) { return v.mPtr ? v.mPtr->toString() : String(); }

template<typename T>
struct ExprTypeOf { enum { value = etObject }; };
template<> struct ExprTypeOf<int> { enum { value = etInt }; };
template<> struct ExprTypeOf<unsigned char> { enum { value = etInt }; };
template<> struct ExprTypeOf<bool> { enum { value = etInt }; };
template<> struct ExprTypeOf<Float> { enum { value = etFloat }; };
template<> struct ExprTypeOf<String> { enum { value = etString }; };


struct NoCrement
{
   enum { OP = coNone };
   template<typename T>
   static inline T run(const T&inVal) { return inVal; }
};

struct CrementPreInc
{
   enum { OP = hx::coPreInc };
   template<typename T>
   static T run(T&inVal) { return ++inVal; }
   static bool run(bool &inVal) { return inVal; }
   static String run(String &inVal) { return inVal; }
};

struct CrementPostInc
{
   enum { OP = hx::coPostInc };
   template<typename T>
   static T run(T& inVal) { return inVal++; }
   static bool run(bool &inVal) { return inVal; }
   static String run(String &inVal) { return inVal; }
};

struct CrementPreDec
{
   enum { OP = hx::coPreDec };
   template<typename T>
   static T run(T&inVal) { return --inVal; }
   static bool run(bool &inVal) { return inVal; }
   static String run(String &inVal) { return inVal; }
};

struct CrementPostDec
{
   enum { OP = hx::coPostDec };
   template<typename T>
   static T run(T& inVal) { return inVal--; }
   static bool run(bool &inVal) { return inVal; }
   static String run(String &inVal) { return inVal; }
};



struct AssignSet
{
   template<typename T>
   static T run(T& ioVal, CppiaCtx *ctx, CppiaExpr *value )
   {
      T val;
      runValue(val, ctx, value );
      BCR_CHECK;
      return ioVal = val;
   }
};


struct AssignAdd
{
   template<typename T>
   static T run(T &ioVal, CppiaCtx *ctx, CppiaExpr *value)
   {
      T rhs;
      runValue(rhs,ctx,value);
      BCR_CHECK;
      ioVal += rhs;
      return ioVal;
   }
   static bool run(bool &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) { value->runVoid(ctx); return ioVal; } \
   static hx::Object *run(hx::Object * &ioVal, CppiaCtx *ctx, CppiaExpr *value)
   {
      Dynamic rhs(value->runObject(ctx));
      BCR_CHECK;
      ioVal = ( Dynamic(ioVal) + rhs).mPtr;
      return ioVal;
   }
   static hx::Object *run(Dynamic &ioVal, CppiaCtx *ctx, CppiaExpr *value)
   {
      Dynamic rhs(value->runObject(ctx));
      BCR_CHECK;
      ioVal = ioVal + rhs;
      return ioVal.mPtr;
   }
};


#define DECL_STRUCT_ASSIGN(NAME,OPEQ,OP) \
struct NAME \
{ \
   template<typename T> \
   inline static T &run(T &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) \
   { \
      Float f = value->runFloat(ctx); \
      BCR_CHECK_RET(ioVal); \
      ioVal OPEQ f; \
      return ioVal; \
   } \
   static bool run(bool &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) { value->runVoid(ctx); return ioVal; } \
   static String run(String &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) { value->runVoid(ctx); return ioVal; } \
   static hx::Object *run(hx::Object * &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) \
   { \
      Float f =  value->runFloat(ctx); \
      BCR_CHECK_RET(ioVal); \
      ioVal = Dynamic(Dynamic(ioVal) OP f).mPtr; \
      return ioVal; \
   } \
   static Dynamic &run(Dynamic &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) \
   { \
      Float f = value->runFloat(ctx); \
      BCR_CHECK_RET(ioVal); \
      ioVal = Dynamic(ioVal) OP f; \
      return ioVal; \
   } \
};

DECL_STRUCT_ASSIGN(AssignMult,*=,*)
DECL_STRUCT_ASSIGN(AssignSub,-=,-)
DECL_STRUCT_ASSIGN(AssignDiv,/=,/)

#define DECL_STRUCT_ASSIGN_FUNC(NAME,OP_FUNC,RUN_FUNC,TMP) \
struct NAME \
{ \
   template<typename T> \
   static T run(T &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) \
   { \
      TMP t = value->RUN_FUNC(ctx); \
      BCR_CHECK; \
      ioVal = OP_FUNC(ioVal, t); \
      return ioVal; \
   } \
   static bool run(bool &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) { value->runVoid(ctx); return ioVal; } \
   static String run(String &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) { value->runVoid(ctx); return ioVal; } \
   static hx::Object *run(hx::Object * &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) \
   { \
      TMP t = value->RUN_FUNC(ctx); \
      BCR_CHECK; \
      ioVal = Dynamic( OP_FUNC( Dynamic(ioVal),t )).mPtr; \
      return ioVal; \
   } \
};

DECL_STRUCT_ASSIGN_FUNC(AssignMod,hx::DoubleMod,runFloat,Float)
DECL_STRUCT_ASSIGN_FUNC(AssignUShr,hx::UShr,runInt,int)



#define DECL_STRUCT_ASSIGN_CAST(NAME,CAST,OP,RUN_FUNC) \
struct NAME \
{ \
   template<typename T> \
   static T run(T &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) \
   { \
      CAST t = value->RUN_FUNC(ctx); \
      BCR_CHECK; \
      ioVal = (CAST)ioVal OP t; \
      return ioVal; \
   } \
   static bool run(bool &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) { value->runVoid(ctx); return ioVal; } \
   static String run(String &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) { value->runVoid(ctx); return ioVal; } \
   static hx::Object *run(hx::Object * &ioVal, hx::CppiaCtx *ctx, hx::CppiaExpr *value) \
   { \
      CAST t = value->RUN_FUNC(ctx); \
      BCR_CHECK; \
      ioVal = Dynamic((CAST)Dynamic(ioVal) OP t).mPtr; \
      return ioVal; \
   } \
};

DECL_STRUCT_ASSIGN_CAST(AssignAnd,int,&,runInt)
DECL_STRUCT_ASSIGN_CAST(AssignOr,int,|,runInt)
DECL_STRUCT_ASSIGN_CAST(AssignXOr,int,^,runInt)

DECL_STRUCT_ASSIGN_CAST(AssignShl,int,<<,runInt)
DECL_STRUCT_ASSIGN_CAST(AssignShr,int,>>,runInt)





} // end namespace hx




