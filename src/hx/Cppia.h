#include <hxcpp.h>
#include <hx/Scriptable.h>
#include <hx/GC.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>


namespace hx
{

struct CppiaStream
{
   const char *data;
   const char *max;
   int line;
   int pos;

   CppiaStream(const char *inData, int inLen)
   {
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

};


struct TypeData;
struct CppiaClass;
struct CppiaData;
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
   arrDynamic,
};



struct CppiaExpr
{
   int fileId;
   int line;
   int haxeTypeId;

   CppiaExpr() : fileId(0), line(0), haxeTypeId(0) {}
   CppiaExpr(const CppiaExpr *inSrc)
   {
      if (inSrc)
      {
         fileId = inSrc->fileId;
         line = inSrc->line;
         haxeTypeId = inSrc->haxeTypeId;
      }
      else
      {
         fileId = line = haxeTypeId = 0;
      }
   }

   virtual ~CppiaExpr() {}

   virtual CppiaExpr   *link(CppiaData &data)   { return this; }
   virtual void mark(hx::MarkContext *ctx) { };
   virtual void visit(hx::VisitContext *ctx) { };


   virtual ExprType    getType() { return etObject; }

   virtual int         runInt(CppiaCtx *ctx)    { return 0; }
   virtual Float       runFloat(CppiaCtx *ctx) { return runInt(ctx); }
   virtual ::String    runString(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      if (!result)
         return null();
      return result->toString();
   }
   virtual hx::Object *runObject(CppiaCtx *ctx) { return Dynamic(runFloat(ctx)).mPtr; }
   virtual void        runVoid(CppiaCtx *ctx)   { runObject(ctx); }
   virtual CppiaExpr   *makeSetter(CppiaExpr *inValue) { return this; }

};

typedef std::vector<CppiaExpr *> Expressions;

CppiaExpr *createArrayBuiltin(CppiaExpr *inSrc, ArrayType inType, CppiaExpr *inThisExpr,
                              String inField, Expressions &ioExpressions );

template<typename T>
inline T &runValue(T& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   expr->runVoid(ctx);
   return outValue;
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



inline static int ValToInt( const bool &v ) { return v; }
inline static int ValToInt( const int &v ) { return v; }
inline static int ValToInt( const unsigned char &v ) { return v; }
inline static int ValToInt( const Float &v ) { return v; }
inline static int ValToInt( const String &v ) { return 0; }
inline static int ValToInt( const hx::Object *v ) { return v->__ToInt(); }
inline static int ValToInt( const Dynamic &v ) { return v->__ToInt(); }

inline static Float ValToFloat( const bool &v ) { return v; }
inline static Float ValToFloat( const int &v ) { return v; }
inline static Float ValToFloat( const unsigned char &v ) { return v; }
inline static Float ValToFloat( const Float &v ) { return v; }
inline static Float ValToFloat( const String &v ) { return 0; }
inline static Float ValToFloat( const hx::Object *v ) { return v->__ToDouble(); }
inline static Float ValToFloat( const Dynamic &v ) { return v->__ToDouble(); }

inline static String ValToString( const bool &v ) { return v?HX_CSTRING("true") : HX_CSTRING("false"); }
inline static String ValToString( const int &v ) { return String(v); }
inline static String ValToString( const unsigned char &v ) { return String(v); }
inline static String ValToString( const Float &v ) { return String(v); }
inline static String ValToString( const String &v ) { return v; }
inline static String ValToString( const hx::Object *v ) { return v->__ToString(); }
inline static String ValToString( const Dynamic &v ) { return v->__ToString(); }

template<typename T>
struct ExprTypeOf { enum { value = etObject }; };
template<> struct ExprTypeOf<int> { enum { value = etInt }; };
template<> struct ExprTypeOf<unsigned char> { enum { value = etInt }; };
template<> struct ExprTypeOf<bool> { enum { value = etInt }; };
template<> struct ExprTypeOf<Float> { enum { value = etFloat }; };
template<> struct ExprTypeOf<String> { enum { value = etString }; };


} // end namespace hx




