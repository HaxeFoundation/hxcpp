#ifndef HX_CPPIA_INCLUDED_H
#define HX_CPPIA_INCLUDED_H

#include <hx/Scriptable.h>
#include <hx/GC.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>

#ifdef CPPIA_JIT
#include "CppiaCompiler.h"
#endif

namespace hx
{

class CppiaModule;

#define DBGLOG(...) { }
//#define DBGLOG printf

struct TypeData;
struct CppiaClassInfo;
struct CppiaExpr;
struct CppiaStream;
struct CppiaStackVar;
struct StackLayout;
struct ScriptCallable;
class  ScriptRegistered;
class  HaxeNativeClass;
class  HaxeNativeInterface;

enum CppiaOp
{
   #define CPPIA_OP(ident, name, val) ident = val,
   #include "CppiaOps.inc"
   #undef CPPIA_OP
};


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


enum VarLocation
{
   locObj,
   locThis,
   locStack,
   locAbsolute,
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
   virtual CppiaExpr   *link(CppiaModule &data)   { return this; }
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

   virtual void        runFunction(CppiaCtx *ctx)   { NullReference("Function", false); }

   virtual CppiaExpr   *makeSetter(AssignOp op,CppiaExpr *inValue) { return 0; }
   virtual CppiaExpr   *makeCrement(CrementOp inOp) { return 0; }


   #ifdef CPPIA_JIT
   virtual void preGen(CppiaCompiler &compiler) { }
   virtual void genCode(CppiaCompiler &compiler, const Addr &inDest, ExprType resultType);
   #endif
};

CppiaExpr *createCppiaExpr(CppiaStream &inStream);
CppiaExpr *createStaticAccess(CppiaExpr *inSrc, ExprType inType, void *inPtr);
CppiaExpr *createStaticAccess(CppiaExpr *inSrc, FieldStorage inType, void *inPtr);
hx::Object *createClosure(CppiaCtx *ctx, ScriptCallable *inFunction);
hx::Object *createMemberClosure(hx::Object *, ScriptCallable *inFunction);
hx::Object *createEnumClosure(struct CppiaEnumConstructor &inContructor);



struct TypeData
{
   String              name;
   hx::Class               haxeClass;
   CppiaClassInfo      *cppiaClass;
   ExprType            expressionType;
   HaxeNativeClass     *haxeBase;
   HaxeNativeInterface *interfaceBase;
   bool                linked;
   bool                isInterface;
   ArrayType           arrayType;

   TypeData(String inData);

   bool isClassOf(Dynamic inInstance);
   void mark(hx::MarkContext *__inCtx);
   void visit(hx::VisitContext *__inCtx);
   void link(CppiaModule &inData);
};



class CppiaModule
{
public:
   Array< String >                 strings;
   std::vector< std::string >      cStrings;
   std::vector< TypeData * >       types;
   std::vector< CppiaClassInfo * > classes;
   std::vector< CppiaExpr * >      markable;
   typedef std::map< std::string, int > InterfaceSlots;
   InterfaceSlots                  interfaceSlots;

   StackLayout                     *layout;
   CppiaClassInfo                  *linkingClass;
   const char                      *creatingClass;
   const char                      *creatingFunction;
   int                             scriptId;

   ScriptCallable                  *main;

   CppiaModule();
   ~CppiaModule();

   void link();
   void compile();
   void setDebug(CppiaExpr *outExpr, int inFileId, int inLine);
   void boot(CppiaCtx *ctx);
   void where(CppiaExpr *e);
   void mark(hx::MarkContext *ctx);
   void visit(hx::VisitContext *ctx);
   int  getInterfaceSlot(const std::string &inName);
   int  findInterfaceSlot(const std::string &inName);

   inline const char *identStr(int inId) { return strings[inId].__s; }
   inline const char *typeStr(int inId) { return types[inId]->name.c_str(); }
};


struct StackLayout
{
   std::map<int,CppiaStackVar *> varMap;
   std::vector<CppiaStackVar *>  captureVars;
   StackLayout                   *parent;
   ExprType                      returnType;
   int                           captureSize;
   int                           size;

   StackLayout(StackLayout *inParent);

   void dump(Array<String> &inStrings, std::string inIndent);
   CppiaStackVar *findVar(int inId);
};



struct ArgInfo
{
   int  nameId;
   bool optional;
   int  typeId;
};


struct CppiaFunction
{
   CppiaModule          &cppia;
   int                  nameId;
   bool                 isStatic;
   bool                 isDynamic;
   int                  returnType;
   int                  argCount;
   int                  vtableSlot;
   bool                 linked;
   std::string          name;
   std::vector<ArgInfo> args;
   ScriptCallable       *funExpr;

   CppiaFunction(CppiaModule *inCppia,bool inIsStatic,bool inIsDynamic);

   inline void setVTableSlot(int inSlot) { vtableSlot = inSlot; }
   inline String getName() { return cppia.strings[nameId]; }
   void load(CppiaStream &stream,bool inExpectBody);
   void link( );
   void compile();
};


struct CppiaStackVar
{
   int      nameId;
   int      id;
   bool     capture;
   int      typeId;
   int      stackPos;
   int      fromStackPos;
   int      capturePos;
   FieldStorage storeType;
   ExprType expressionType;

   CppiaStackVar();
   CppiaStackVar(CppiaStackVar *inVar,int &ioSize, int &ioCaptureSize);

   void fromStream(CppiaStream &stream);
   void set(CppiaCtx *inCtx,Dynamic inValue);
   void markClosure(char *inBase, hx::MarkContext *__inCtx);
   void visitClosure(char *inBase, hx::VisitContext *__inCtx);
   void link(CppiaModule &inModule);
};


int getStackVarNameId(int inVarId);

hx::Object *ObjectToInterface(hx::Object *inObject, TypeData *toType);

struct CppiaVar
{
   enum Access { accNormal, accNo, accResolve, accCall, accRequire, accCallNative } ;

   TypeData         *type;
   bool             isStatic;
   bool             isVirtual;
   Access           readAccess;
   Access           writeAccess;
   int              nameId;
   int              typeId;
   int              offset;
   String           name;
   FieldStorage     storeType;
   CppiaFunction    *dynamicFunction;
   ExprType         exprType;

   CppiaExpr        *init;
   Dynamic          objVal;
   bool             boolVal;
   int              intVal;
   Float            floatVal;
   String           stringVal;
   void             *valPointer;
   

   CppiaVar(bool inIsStatic);
   CppiaVar(CppiaFunction *inDynamicFunction);

   void clear();
   void load(CppiaStream &stream);
   inline ExprType getType() { return exprType; }
   void linkVarTypes(CppiaModule &cppia);
   Dynamic getStaticValue();
   Dynamic setStaticValue(Dynamic inValue);
   CppiaExpr *createAccess(CppiaExpr *inSrc);
   void linkVarTypes(CppiaModule &cppia, int &ioOffset);
   void createDynamic(hx::Object *inBase);
   Dynamic getValue(hx::Object *inThis);
   Dynamic setValue(hx::Object *inThis, Dynamic inValue);
   void link(CppiaModule &inModule);
   static Access getAccess(CppiaStream &stream);
   void runInit(CppiaCtx *ctx);

   inline void mark(hx::Object *inThis,hx::MarkContext *__inCtx)
   {
      switch(storeType)
      {
         case fsString: HX_MARK_MEMBER(*(String *)((char *)inThis + offset)); break;
         case fsObject: HX_MARK_MEMBER(*(Dynamic*)((char *)inThis + offset)); break;
         default:;
      }
   }
   inline void visit(hx::Object *inThis,hx::VisitContext *__inCtx)
   {
      switch(storeType)
      {
         case fsString: HX_VISIT_MEMBER(*(String *)((char *)inThis + offset)); break;
         case fsObject: HX_VISIT_MEMBER(*(Dynamic *)((char *)inThis + offset)); break;
         default:;
      }
   }

   void mark(hx::MarkContext *__inCtx);
   void visit(hx::VisitContext *__inCtx);
};




class HaxeNativeClass
{
public:
   std::string  name;
   hx::ScriptableClassFactory factory;
   hx::ScriptFunction  construct;
   ScriptNamedFunction *functions;
   HaxeNativeClass *haxeSuper;
   int mDataOffset;

   HaxeNativeClass(const std::string &inName, int inDataOffset, ScriptNamedFunction *inFunctions, hx::ScriptableClassFactory inFactory, ScriptFunction inConstruct);

   void addVtableEntries( std::vector<std::string> &outVtable);
   void dump();
   ScriptFunction findFunction(const std::string &inName);

   static HaxeNativeClass *findClass(const std::string &inName);
   static HaxeNativeClass *hxObject();
   static void link();
};

class HaxeNativeInterface
{
public:
   std::string  name;
   ScriptNamedFunction *functions;

   #if (HXCPP_API_LEVEL >= 330)
   void *scriptTable;

   HaxeNativeInterface(const std::string &inName, ScriptNamedFunction *inFunctions,void *inScriptTable);

   #else
   const hx::type_info *mType;
   ScriptableInterfaceFactory factory;

   HaxeNativeInterface(const std::string &inName, ScriptNamedFunction *inFunctions,hx::ScriptableInterfaceFactory inFactory,const hx::type_info *inType);
   #endif


   ScriptFunction findFunction(const std::string &inName);

   static HaxeNativeInterface *findInterface(const std::string &inName);
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
   #define CPPIA_CHECK(obj) { }
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


template<> inline double &runValue(double& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runFloat(ctx);
}


template<> inline float &runValue(float& outValue, CppiaCtx *ctx, CppiaExpr *expr)
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
   static hx::Object *run(hx::Object *&inVal) { return inVal = Dynamic(Dynamic(inVal) + 1).mPtr; }
   static bool run(bool &inVal) { return inVal; }
   static String run(String &inVal) { return inVal; }
};

struct CrementPostInc
{
   enum { OP = hx::coPostInc };
   template<typename T>
   static T run(T& inVal) { return inVal++; }
   static hx::Object *run(hx::Object *&inVal)
   {
      hx::Object *result(inVal);
      inVal = Dynamic(Dynamic(inVal) + 1).mPtr;
      return result;
   }
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
   static hx::Object *run(hx::Object *&inVal) { return inVal = Dynamic(Dynamic(inVal) - 1).mPtr; }
};

struct CrementPostDec
{
   enum { OP = hx::coPostDec };
   template<typename T>
   static T run(T& inVal) { return inVal--; }
   static bool run(bool &inVal) { return inVal; }
   static String run(String &inVal) { return inVal; }
   static hx::Object *run(hx::Object *&inVal)
   {
      hx::Object *result(inVal);
      inVal = Dynamic(Dynamic(inVal) - 1).mPtr;
      return result;
   }
 
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


int runContextConvertInt(CppiaCtx *ctx, ExprType inType, void *inFunc);
Float runContextConvertFloat(CppiaCtx *ctx, ExprType inType, void *inFunc);
String runContextConvertString(CppiaCtx *ctx, ExprType inType, void *inFunc);
hx::Object *runContextConvertObject(CppiaCtx *ctx, ExprType inType, void *inFunc);




} // end namespace hx



#endif
