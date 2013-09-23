#include <hxcpp.h>
#include <hx/Scriptable.h>
#include <hx/GC.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>

//#define DBGLOG(...) { }
#define DBGLOG printf


namespace hx
{

// todo - TLS
static CppiaCtx *sCurrent = 0;

CppiaCtx::CppiaCtx()
{
   stack = new unsigned char[64*1024];
   pointer = &stack[0];
   push((hx::Object *)0);
   frame = pointer;
   sCurrent = this;
}

CppiaCtx::~CppiaCtx()
{
   delete [] stack;
}

CppiaCtx *CppiaCtx::getCurrent() { return sCurrent; }



class ScriptRegistered
{
public:
   std::vector<std::string> vtableEntries;
   hx::ScriptableClassFactory factory;
   hx::StackFunction  construct;
   int mDataOffset;

   ScriptRegistered(int inDataOffset, String *inFunctions, hx::ScriptableClassFactory inFactory, StackFunction inConstruct)
   {
      mDataOffset = inDataOffset;
      factory = inFactory;
      construct = inConstruct;
      if (inFunctions)
         for(String *func = inFunctions; *func!=null(); func++)
            vtableEntries.push_back( func->__s );
   }
};

class ScriptRegisteredIntferface
{
public:
   const hx::type_info *mType;
   ScriptableInterfaceFactory factory;

   ScriptRegisteredIntferface(hx::ScriptableInterfaceFactory inFactory,const hx::type_info *inType)
   {
      factory = inFactory;
      mType = inType;
   }
};


typedef std::map<std::string, ScriptRegistered *> ScriptRegisteredMap;
static ScriptRegisteredMap *sScriptRegistered = 0;
static ScriptRegistered *sObject = 0;

typedef std::map<std::string, ScriptRegisteredIntferface *> ScriptRegisteredInterfaceMap;
static ScriptRegisteredInterfaceMap *sScriptRegisteredInterface = 0;


// TODO - toString?
class Object_obj__scriptable : public Object
{
   typedef Object_obj__scriptable __ME;
   typedef Object super;

   void __construct() { }
   HX_DEFINE_SCRIPTABLE(HX_ARR_LIST0)
   HX_DEFINE_SCRIPTABLE_DYNAMIC;
};



void ScriptableRegisterClass( String inName, int inDataOffset, String *inFunctions, hx::ScriptableClassFactory inFactory, hx::StackFunction inConstruct)
{

   printf("ScriptableRegisterClass %s\n", inName.__s);
   if (!sScriptRegistered)
      sScriptRegistered = new ScriptRegisteredMap();
   ScriptRegistered *registered = new ScriptRegistered(inDataOffset, inFunctions,inFactory, inConstruct);
   (*sScriptRegistered)[inName.__s] = registered;
   //printf("Registering %s -> %p\n",inName.__s,(*sScriptRegistered)[inName.__s]);
}


void ScriptableRegisterInterface( String inName, const hx::type_info *inType,
                                 hx::ScriptableInterfaceFactory inFactory )
{
   if (!sScriptRegisteredInterface)
      sScriptRegisteredInterface = new ScriptRegisteredInterfaceMap();
   ScriptRegisteredIntferface *registered = new ScriptRegisteredIntferface(inFactory,inType);
   (*sScriptRegisteredInterface)[inName.__s] = registered;
   //printf("Registering Interface %s -> %p\n",inName.__s,(*sScriptRegisteredInterface)[inName.__s]);
}



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
struct ClassData;
struct CppiaData;
struct CppiaExpr;


enum ExprType
{
  etVoid,
  etNull,
  etObject,
  etString,
  etFloat,
  etInt,
};

static int sTypeSize[] = { 0, 0, sizeof(hx::Object *), sizeof(String), sizeof(Float), sizeof(int) };



struct TypeData
{
   String name;
   Class  haxeClass;
   ClassData *cppiaClass;
   ExprType  expressionType;

   TypeData(String inData)
   {
      Array<String> parts = inData.split(HX_CSTRING("::"));
      if (parts[0].length==0)
         parts->shift();
      name = parts->join(HX_CSTRING("."));
      cppiaClass = 0;
      haxeClass = null();
   }
   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(name);
      HX_MARK_MEMBER(haxeClass);
   }
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(name);
      HX_VISIT_MEMBER(haxeClass);
   }

   void link(CppiaData &inData);
};

struct CppiaStackVar;
struct StackLayout
{
   // 'this' pointer is in slot 0 ...
   StackLayout() : size( sizeof(void *) ) { }
   int size;
   std::map<int,CppiaStackVar *> varMap;
};


struct CppiaData
{
   Array< String > strings;
   std::vector< TypeData * > types;
   std::vector< ClassData * > classes;
   std::vector< CppiaExpr * > markable;
   CppiaExpr   *main;

   StackLayout *layout;

   CppiaData()
   {
      main = 0;
      layout = 0;
      strings = Array_obj<String>::__new(0,0);
   }

   //ClassData *findClass(String inName);

   ~CppiaData();

   void link();

   void mark(hx::MarkContext *ctx);
   void visit(hx::VisitContext *ctx);

   const char *identStr(int inId) { return strings[inId].__s; }
   const char *typeStr(int inId) { return types[inId]->name.c_str(); }
};


struct CppiaStackVar
{
   int  nameId;
   int  id;
   bool capture;
   int  typeId;
   int  stackPos;

   ExprType expressionType;

   void fromStream(CppiaStream &stream)
   {
      nameId = stream.getInt();
      id = stream.getInt();
      capture = stream.getBool();
      typeId = stream.getInt();
   }

   void link(CppiaData &inData)
   {
      expressionType = inData.types[typeId]->expressionType;
      inData.layout->varMap[id] = this;
      stackPos = inData.layout->size;
      inData.layout->size += sTypeSize[expressionType];
   }

};



class CppiaObject : public hx::Object
{
public:
   CppiaData *data;
   CppiaObject(CppiaData *inData)
   {
      data = inData;
      GCSetFinalizer( this, onRelease );
   }
   static void onRelease(hx::Object *inObj)
   {
      delete ((CppiaObject *)inObj)->data;
   }
   void __Mark(hx::MarkContext *ctx) { data->mark(ctx); }
   void __Visit(hx::VisitContext *ctx) { data->visit(ctx); }
};

struct ArgInfo
{
   int  nameId;
   bool optional;
   int  typeId;
};

// --- CppiaExpr ----------------------------------------

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

template<typename T>
T &runValue(T& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   expr->runVoid(ctx);
   return outValue;
}

template<> int &runValue(int& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runInt(ctx);
}

template<> bool &runValue(bool& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runInt(ctx);
}


template<> Float &runValue(Float& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runFloat(ctx);
}

template<> String &runValue(String& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runString(ctx);
}

template<> hx::Object * &runValue(hx::Object *& outValue, CppiaCtx *ctx, CppiaExpr *expr)
{
   return outValue = expr->runObject(ctx);
}



// --- CppiaCtx functions ----------------------------------------

int CppiaCtx::runInt(void *vtable) { return ((CppiaExpr *)vtable)->runInt(this); }
Float CppiaCtx::runFloat(void *vtable) { return ((CppiaExpr *)vtable)->runFloat(this); }
String CppiaCtx::runString(void *vtable) { return ((CppiaExpr *)vtable)->runString(this); }
void CppiaCtx::runVoid(void *vtable) { ((CppiaExpr *)vtable)->runVoid(this); }
Dynamic CppiaCtx::runObject(void *vtable) { return ((CppiaExpr *)vtable)->runObject(this); }



// --- CppiaDynamicExpr ----------------------------------------
// Delegates to 'runObject'

struct CppiaDynamicExpr : public CppiaExpr
{
   CppiaDynamicExpr(const CppiaExpr *inSrc=0) : CppiaExpr(inSrc) {}

   int         runInt(CppiaCtx *ctx)    { return runObject(ctx)->__ToInt(); }
   Float       runFloat(CppiaCtx *ctx) { return runObject(ctx)->__ToDouble(); }
   ::String    runString(CppiaCtx *ctx) { return runObject(ctx)->__ToString(); }
   void        runVoid(CppiaCtx *ctx)   { runObject(ctx); }
   hx::Object *runObject(CppiaCtx *ctx) { return 0; }
};


// ------------------------------------------------------

typedef std::vector<CppiaExpr *> Expressions;

CppiaExpr *createCppiaExpr(CppiaStream &inStream);

static void ReadExpressions(Expressions &outExpressions, CppiaStream &stream, bool hasTypeId)
{
   int count = stream.getInt();
   outExpressions.resize(count);
   std::vector<int> typeIds(count);
   if (hasTypeId)
      for(int i=0;i<count;i++)
         typeIds[i] = stream.getInt();

   for(int i=0;i<count;i++)
   {
      outExpressions[i] =  createCppiaExpr(stream);
      outExpressions[i]->haxeTypeId = typeIds[i];
   }
}



static void LinkExpressions(Expressions &ioExpressions, CppiaData &data)
{
   for(int i=0;i<ioExpressions.size();i++)
      ioExpressions[i] = ioExpressions[i]->link(data);
}



struct CppiaFunction
{
   CppiaData &cppia;
   int       nameId;
   bool      isStatic;
   int       returnType;
   int       argCount;
   int       vtableSlot;
   std::string name;
   std::vector<ArgInfo> args;
   CppiaExpr *body;

   CppiaFunction(CppiaData *inCppia,bool inIsStatic) :
      cppia(*inCppia), isStatic(inIsStatic), body(0)
   {
      vtableSlot = 0;
   }

   void setVTableSlot(int inSlot) { vtableSlot = inSlot; }

   void load(CppiaStream &stream,bool inExpectBody)
   {
      nameId = stream.getInt();
      returnType = stream.getInt();
      argCount = stream.getInt();
      printf("  Function %s(%d) : %s\n", cppia.identStr(nameId), argCount, cppia.typeStr(returnType));
      args.resize(argCount);
      for(int a=0;a<argCount;a++)
      {
         ArgInfo arg = args[a];
         arg.nameId = stream.getInt();
         arg.optional = stream.getBool();
         arg.typeId = stream.getInt();
         printf("    arg %c%s:%s\n", arg.optional?'?':' ', cppia.identStr(arg.nameId), cppia.typeStr(arg.typeId) );
      }
      if (inExpectBody)
         body = createCppiaExpr(stream);
   }
   void link( )
   {
      name = cppia.strings[ nameId ].__s;
   }
};


struct CppiaVar
{
   enum Access { accNormal, accNo, accResolve, accCall, accRequire } ;
   CppiaData *cppia;
   TypeData  *type;
   bool      isStatic;
   Access    readAccess;
   Access    writeAccess;
   int       nameId;
   int       typeId;
   int       offset;
   

   CppiaVar(CppiaData *inCppia,bool inIsStatic) :
      cppia(inCppia), isStatic(inIsStatic)
   {
      nameId = 0;
      typeId = 0;
      offset = 0;
      type = 0;
   }

   void load(CppiaStream &stream)
   {
      readAccess = getAccess(stream);
      writeAccess = getAccess(stream);
      nameId = stream.getInt();
      typeId = stream.getInt();
   }

   void link(CppiaData &cppia, int &ioOffset)
   {
      offset = ioOffset;
      type = cppia.types[typeId];
      
      switch(type->expressionType)
      {
         case etInt: ioOffset += sizeof(int); break;
         case etFloat: ioOffset += sizeof(Float); break;
         case etString: ioOffset += sizeof(String); break;
         case etObject: ioOffset += sizeof(hx::Object *); break;
         case etVoid:
         case etNull:
            break;
      }
   }

   static Access getAccess(CppiaStream &stream)
   {
      std::string tok = stream.getToken();
      if (tok.size()!=1)
         throw "bad var access length";
      switch(tok[0])
      {
         case 'N': return accNormal;
         case '!': return accNo;
         case 'R': return accResolve;
         case 'C': return accCall;
         case '?': return accRequire;
      }
      throw "bad access code";
      return accNormal;
   }
};


struct CppiaConst
{
   enum Type { cInt, cFloat, cString, cBool, cNull, cThis, cSuper };

   Type type;
   int  ival;
   Float  dval;

   CppiaConst() : type(cNull), ival(0), dval(0) { }

   void fromStream(CppiaStream &stream)
   {
      std::string tok = stream.getToken();
      if (tok[0]=='i')
      {
         type = cInt;
         dval = ival = atoi(&tok[1]);
      }
      else if (tok[0]=='f')
      {
         type = cFloat;
         dval = atof(&tok[1]);
      }
      else if (tok[0]=='s')
      {
         type = cString;
         ival = atoi(&tok[1]);
      }
      else if (tok=="TRUE")
      {
         type = cInt;
         dval = ival = 1;
      }
      else if (tok=="FALSE")
         type = cInt;
      else if (tok=="NULL")
         type = cNull;
      else if (tok=="THIS")
         type = cThis;
      else if (tok=="SUPER")
         type = cSuper;
      else
         throw "unknown const value";
   }
};



struct ClassData
{
   CppiaData &cppia;
   bool      isInterface;
   bool      isLinked;
   int       typeId;
   int       superId;
   int       extraData;
   TypeData  *superType;
   void      **vtable;
   ScriptRegistered *superBase;
   std::vector<int> implements;
   std::vector<CppiaFunction *> memberFunctions;
   std::vector<CppiaVar *> memberVars;
   CppiaFunction *newFunc;

   std::vector<CppiaFunction *> staticFunctions;
   std::vector<CppiaVar *> staticVars;

   ClassData(CppiaData &inCppia) : cppia(inCppia)
   {
      superType = 0;
      superBase = 0;
      isLinked = false;
      extraData = 0;
      newFunc = 0;
   }

   hx::Object *createInstance(Expressions &inArgs)
   {
      hx::Object *obj = superBase->factory(vtable,extraData);
     

      // Construct
 
      return obj;
   }

   CppiaExpr *findFunction(bool inStatic,int inId)
   {
      if (inStatic && !inId)
         return newFunc ? newFunc->body : 0;

      std::vector<CppiaFunction *> &funcs = inStatic ? staticFunctions : memberFunctions;
      for(int i=0;i<funcs.size();i++)
      {
         if (funcs[i]->nameId == inId)
            return funcs[i]->body;
      }
      return 0;
   }

   void load(CppiaStream &inStream)
   {
      std::string tok = inStream.getToken();

      if (tok=="CLASS")
         isInterface = false;
      else if (tok=="INTERFACE")
         isInterface = true;
      else
         throw "Bad class type";

       typeId = inStream.getInt();
       cppia.types[typeId]->cppiaClass = this;
       superId = inStream.getInt();
       int implementCount = inStream.getInt();
       implements.resize(implementCount);
       for(int i=0;i<implementCount;i++)
          implements[i] = inStream.getInt();
       printf("Class %s\n", cppia.typeStr(typeId));

       int fields = inStream.getInt();
       for(int f=0;f<fields;f++)
       {
          tok = inStream.getToken();
          if (tok=="FUNCTION")
          {
             bool isStatic = inStream.getStatic();
             CppiaFunction *func = new CppiaFunction(&cppia,isStatic);
             if (isStatic)
                staticFunctions.push_back(func);
             else
                memberFunctions.push_back(func);
             func->load(inStream,!isInterface);
          }
          else if (tok=="VAR")
          {
             bool isStatic = inStream.getStatic();
             CppiaVar *var = new CppiaVar(&cppia,isStatic);
             if (isStatic)
                staticVars.push_back(var);
             else
                memberVars.push_back(var);
             var->load(inStream);
          }
          else if (tok=="INLINE")
          {
             // OK
          }
          else
             throw "unknown field type";
       }
   }

   void link()
   {
      if (isLinked)
         return;

      isLinked = true;

      for(int i=0;i<staticFunctions.size();i++)
         staticFunctions[i]->link();

      for(int i=0;i<staticFunctions.size();i++)
         if (staticFunctions[i]->name == "new")
         {
            newFunc = staticFunctions[i];
            staticFunctions.erase( staticFunctions.begin() + i);
            break;
         }


      for(int i=0;i<memberFunctions.size();i++)
         memberFunctions[i]->link();

      if (superId>0)
      {
         superType = cppia.types[ superId ];
         if (superType->cppiaClass)
         {
            superType->cppiaClass->link();
            if (!newFunc)
               newFunc = superType->cppiaClass->newFunc;
            superBase = superType->cppiaClass->superBase;

            std::vector<CppiaVar *> combinedVars(superType->cppiaClass->memberVars );
            for(int i=0;i<memberVars.size();i++)
            {
               for(int j=0;j<combinedVars.size();j++)
                  if (combinedVars[j]->nameId==memberVars[i]->nameId)
                     printf("Warning duplicate member var %s\n", cppia.strings[memberVars[i]->nameId].__s);
               combinedVars.push_back(memberVars[i]);
            }
            memberVars.swap(combinedVars);

            std::vector<CppiaFunction *> combinedFunctions(superType->cppiaClass->memberFunctions );
            for(int i=0;i<memberFunctions.size();i++)
            {
               bool found = false;
               for(int j=0;j<combinedFunctions.size();j++)
                  if (combinedFunctions[j]->name==memberFunctions[i]->name)
                  {
                     combinedFunctions[j] = memberFunctions[i];
                     found = true;
                     break;
                  }
               if (!found)
                  combinedFunctions.push_back(memberFunctions[i]);
            }
            memberFunctions.swap(combinedFunctions);
         }
      }

      if (!superBase)
         superBase = (*sScriptRegistered)["hx.Object"];

      int d0 = superBase->mDataOffset;
      int offset = d0;
      for(int i=0;i<memberVars.size();i++)
         memberVars[i]->link(cppia,offset);

      std::vector<std::string> &table = superBase->vtableEntries;
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
            idx = vtableSlot++;
         memberFunctions[i]->setVTableSlot(idx);
      }
      vtable = new void*[vtableSlot + 1];
      memset(vtable, 0, sizeof(void *)*(vtableSlot+1));
      *vtable++ = this;
      for(int i=0;i<memberFunctions.size();i++)
         vtable[ memberFunctions[i]->vtableSlot ] = memberFunctions[i]->body;

      extraData = offset - d0;

      //printf("Found superbase %s = %p / %d\n", cppia.types[typeId]->name.__s, superBase, dataSize );
   }
};

struct FunExpr : public CppiaExpr
{
   int returnType;
   int argCount;
   int stackSize;
   std::vector<CppiaStackVar> args;
   std::vector<bool>          hasDefault;
   std::vector<CppiaConst>    initVals;
   CppiaExpr *body;

   FunExpr(CppiaStream &stream)
   {
      body = 0;
      stackSize = 0;
      returnType = stream.getInt();
      argCount = stream.getInt();
      args.resize(argCount);
      hasDefault.resize(argCount);
      for(int a=0;a<argCount;a++)
      {
         args[a].fromStream(stream);
         bool init = stream.getBool();
         hasDefault.push_back(init);
         if (init)
            initVals[a].fromStream(stream);
      }
      body = createCppiaExpr(stream);
   }

   CppiaExpr *link(CppiaData &inData)
   {
      StackLayout *oldLayout = inData.layout;
      StackLayout layout;
      inData.layout = &layout;

      for(int a=0;a<args.size();a++)
         args[a].link(inData);

      body = body->link(inData);

      stackSize = layout.size;
      inData.layout = oldLayout;
      return this;
   }

   void pushArgs(CppiaCtx *ctx, Expressions &inArgs)
   {
      ctx->push( (hx::Object *)0 ); // this
      if (argCount!=inArgs.size())
      {
         printf("Arg count mismatch?\n");
         return;
      }

      for(int a=0;a<argCount;a++)
      {
         CppiaStackVar &var = args[a];
         // TODO capture
         if (hasDefault[a])
         {
            hx::Object *obj = inArgs[a]->runObject(ctx);
            switch(var.expressionType)
            {
               case etInt:
                  ctx->push( obj ? obj->__ToInt() : initVals[a].ival );
                  break;
               case etFloat:
                  ctx->push( (Float)(obj ? obj->__ToDouble() : initVals[a].dval) );
                  break;
               /* todo - default strings.
               case etString:
                  ctx.push( obj ? obj->__ToString() : initVals[a].ival );
                  break;
               */
               default:
                  ctx->push(obj);
            }
         }
         else
         {
            switch(var.expressionType)
            {
               case etInt:
                  ctx->push(inArgs[a]->runInt(ctx));
                  break;
               case etFloat:
                  ctx->push(inArgs[a]->runFloat(ctx));
                  break;
               case etString:
                  ctx->push(inArgs[a]->runString(ctx));
                  break;
               default:
                  ctx->push(inArgs[a]->runObject(ctx));
            }
         }
      }
   }
};

struct BlockExpr : public CppiaExpr
{
   Expressions expressions;

   BlockExpr(CppiaStream &stream)
   {
      ReadExpressions(expressions,stream,false);
   }

   CppiaExpr *link(CppiaData &data)
   {
      LinkExpressions(expressions,data);
      return this;
   }

   virtual ExprType getType()
   {
      if (expressions.size()==0)
         return etNull;
      return expressions[expressions.size()-1]->getType();
   }

   #define BlockExprRun(ret,name,defVal) \
     ret name(CppiaCtx *ctx) \
     { \
        for(int a=0;a<expressions.size()-1;a++) \
          expressions[a]->runVoid(ctx); \
        if (expressions.size()>0) \
           return expressions[expressions.size()-1]->name(ctx); \
        return defVal; \
     }
   BlockExprRun(int,runInt,0)
   BlockExprRun(Float ,runFloat,0)
   BlockExprRun(String,runString,null())
   BlockExprRun(hx::Object *,runObject,0)
   void  runVoid(CppiaCtx *ctx)
   {
      for(int a=0;a<expressions.size();a++)
         expressions[a]->runVoid(ctx);
   }
};

struct IfElseExpr : public CppiaExpr
{
   CppiaExpr *condition;
   CppiaExpr *doIf;
   CppiaExpr *doElse;

   IfElseExpr(CppiaStream &stream)
   {
      condition = createCppiaExpr(stream);
      doIf = createCppiaExpr(stream);
      doElse = createCppiaExpr(stream);
   }
};


struct IfExpr : public CppiaExpr
{
   CppiaExpr *condition;
   CppiaExpr *doIf;

   IfExpr(CppiaStream &stream)
   {
      condition = createCppiaExpr(stream);
      doIf = createCppiaExpr(stream);
   }
};


struct IsNull : public CppiaExpr
{
   CppiaExpr *condition;

   IsNull(CppiaStream &stream) { condition = createCppiaExpr(stream); }
};


struct IsNotNull : public CppiaExpr
{
   CppiaExpr *condition;

   IsNotNull(CppiaStream &stream) { condition = createCppiaExpr(stream); }
};


struct CallFunExpr : public CppiaExpr
{
   Expressions args;
   FunExpr     *function;
   ExprType    returnType;

   CallFunExpr(const CppiaExpr *inSrc, FunExpr *inFunction, Expressions &ioArgs )
      : CppiaExpr(inSrc)
   {
      args.swap(ioArgs);
      function = inFunction;
   }

   CppiaExpr *link(CppiaData &inData)
   {
      LinkExpressions(args,inData);
      function = (FunExpr *)function->link(inData);
      returnType = inData.types[ function->returnType ]->expressionType;
      // TODO interface cast...
      return this;
   }

   ExprType getType() { return returnType; }

   #define CallFunExprVal(ret,funcName,def) \
   ret funcName(CppiaCtx *ctx) \
   { \
      AutoStack save(ctx); \
      function->pushArgs(ctx,args); \
      try \
      { \
         function->body->runVoid(ctx); \
      } \
      catch (CppiaExpr *retVal) \
      { \
         if (retVal) \
            return retVal->funcName(ctx); \
      } \
      return def; \
   }
   CallFunExprVal(int,runInt,0);
   CallFunExprVal(Float ,runFloat,0);
   CallFunExprVal(String,runString,null());
   CallFunExprVal(hx::Object *,runObject,0);

   void runVoid(CppiaCtx *ctx)
   {
      AutoStack save(ctx);
      function->pushArgs(ctx,args);
      try { function->body->runVoid(ctx); }
      catch (CppiaExpr *retVal)
      {
         if (retVal)
            retVal->runVoid(ctx);
      }
   }

};

// ---


struct CppiaExprWithValue : public CppiaExpr
{
   Dynamic     value;

   CppiaExprWithValue(const CppiaExpr *inSrc=0) : CppiaExpr(inSrc)
   {
      value.mPtr = 0;
   }

   hx::Object *runObject(CppiaCtx *ctx) { return value.mPtr; }
   void mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(value); }
   void visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(value); }

   CppiaExpr *link(CppiaData &inData)
   {
      inData.markable.push_back(this);
      return this;
   }

   void runVoid(CppiaCtx *ctx) { runObject(ctx); }

};

// ---



struct CallDynamicFunction : public CppiaExprWithValue
{
   Expressions args;
   Dynamic     value;

   CallDynamicFunction(CppiaData &inData, const CppiaExpr *inSrc,
                       Dynamic inFunction, Expressions &ioArgs )
      : CppiaExprWithValue(inSrc)
   {
      args.swap(ioArgs);
      value = inFunction;
      inData.markable.push_back(this);
   }

   CppiaExpr *link(CppiaData &inData)
   {
      LinkExpressions(args,inData);
      return this;
   }

   ExprType getType() { return etObject; }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      int n = args.size();
      switch(n)
      {
         case 0:
            return value().mPtr;
         case 1:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               return value->__run(arg0).mPtr;
            }
         case 2:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               Dynamic arg1( args[1]->runObject(ctx) );
               return value->__run(arg0,arg1).mPtr;
            }
         case 3:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               Dynamic arg1( args[1]->runObject(ctx) );
               Dynamic arg2( args[2]->runObject(ctx) );
               return value->__run(arg0,arg1,arg2).mPtr;
            }
         case 4:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               Dynamic arg1( args[1]->runObject(ctx) );
               Dynamic arg2( args[2]->runObject(ctx) );
               Dynamic arg3( args[3]->runObject(ctx) );
               return value->__run(arg0,arg1,arg2,arg3).mPtr;
            }
         case 5:
            {
               Dynamic arg0( args[0]->runObject(ctx) );
               Dynamic arg1( args[1]->runObject(ctx) );
               Dynamic arg2( args[2]->runObject(ctx) );
               Dynamic arg3( args[3]->runObject(ctx) );
               Dynamic arg4( args[4]->runObject(ctx) );
               return value->__run(arg0,arg1,arg2,arg3,arg4).mPtr;
            }
      }

      Array<Dynamic> argVals = Array_obj<Dynamic>::__new(n,n);
      for(int a=0;a<n;a++)
         argVals[a] = Dynamic( args[a]->runObject(ctx) );
      return value->__Run(argVals).mPtr;
   }

   int runInt(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      return result ? result->__ToInt() : 0;
   }
   Float  runFloat(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      return result ? result->__ToDouble() : 0;
   }
   String runString(CppiaCtx *ctx)
   {
      hx::Object *result = runObject(ctx);
      return result ? result->__ToString() : 0;
   }
};

struct SetExpr : public CppiaExpr
{
   int toTypeId;
   int fromTypeId;
   CppiaExpr *lvalue;
   CppiaExpr *value;

   SetExpr(CppiaStream &stream)
   {
      toTypeId = stream.getInt();
      fromTypeId = stream.getInt();
      lvalue = createCppiaExpr(stream);
      value = createCppiaExpr(stream);
   }

   CppiaExpr *link(CppiaData &inData)
   {
      lvalue = lvalue->link(inData);
      value = value->link(inData);
      // TODO cast
      lvalue->makeSetter(value);
      CppiaExpr *result = lvalue->makeSetter(value);
      delete this;
      return result;
   }

};

struct NewExpr : public CppiaExpr
{
   int classId;
   TypeData *type;
   Expressions args;
	hx::ConstructArgsFunc constructor;


   NewExpr(CppiaStream &stream)
   {
      classId = stream.getInt();
      constructor = 0;
      ReadExpressions(args,stream,true);
   }

   CppiaExpr *link(CppiaData &inData)
   {
      type = inData.types[classId];
      if (type->haxeClass.mPtr)
         constructor = type->haxeClass.mPtr->mConstructArgs;

      LinkExpressions(args,inData);
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (constructor)
      {
         int n = args.size();
         Array< Dynamic > argList = Array_obj<Dynamic>::__new(n,n);
         for(int a=0;a<n;a++)
            argList[a].mPtr = args[a]->runObject(ctx);
            
          return constructor(argList).mPtr;
      }
      else
      {
         return type->cppiaClass->createInstance(args);
      }

      printf("Can't create non haxe type\n");
      return 0;
   }
   
};

//template<typname RETURN>
struct CallHaxe : public CppiaExpr
{
   Expressions args;
   StackFunction function;

   CallHaxe(CppiaExpr *inSrc,StackFunction inFunction, ExprType et, Expressions &ioArgs )
       : CppiaExpr(inSrc)
   {
      args.swap(ioArgs);
      function = inFunction;
   }


};

struct CallStatic : public CppiaExpr
{
   int classId;
   int fieldId;
   Expressions args;
  
   CallStatic(CppiaStream &stream,bool inIsSuperCall)
   {
      classId = stream.getInt();
      if (inIsSuperCall)
         fieldId = 0;
      else
         fieldId = stream.getInt();
      ReadExpressions(args,stream,true);
   }
   CppiaExpr *link(CppiaData &inData)
   {
      TypeData *type = inData.types[classId];
      String field = inData.strings[fieldId];
      if (type->haxeClass.mPtr)
      {
         // TODO - might change if dynamic function (eg, trace)
         Dynamic func = type->haxeClass.mPtr->__Field( field, false );
         if (func.mPtr)
         {
            CppiaExpr *replace = new CallDynamicFunction(inData, this, func, args );
            delete this;
            replace->link(inData);
            return replace;
         }
      }
      else
      {
         FunExpr *func = (FunExpr *)type->cppiaClass->findFunction(true,fieldId);
         if (!func)
         {
            if (fieldId==0)
            {
               ScriptRegistered *super = type->cppiaClass->superBase;
               if (super)
               {
                  CppiaExpr *replace = new CallHaxe(this,super->construct,etVoid,args);
                  replace->link(inData);
                  delete this;
                  return replace;
               }
            }
            else
               printf("Could not find static function %s in %s\n", field.__s, type->name.__s);
         }
         else
         {
            CppiaExpr *replace = new CallFunExpr( this, func, args );
            delete this;
            replace->link(inData);
            return replace;
         }
      }
      printf("Unknown call to %s::%s\n", type->name.__s, field.__s);
      return this;
   }
};


struct Call : public CppiaExpr
{
   Expressions args;
   CppiaExpr   *func;
  
   Call(CppiaStream &stream)
   {
      ReadExpressions(args,stream,true);
      func = createCppiaExpr(stream);
   }
};


struct GetFieldByName : public CppiaDynamicExpr
{
   int         nameId;
   CppiaExpr   *object;
   String      name;
  
   GetFieldByName(CppiaStream &stream,bool isThisObject)
   {
      nameId = stream.getInt();
      object = isThisObject ? 0 : createCppiaExpr(stream);
      name.__s = 0;
   }
   GetFieldByName(const CppiaExpr *inSrc, int inNameId, CppiaExpr *inObject)
      : CppiaDynamicExpr(inSrc)
   {
      nameId = inNameId;
      object = inObject;
      name.__s = 0;
   }
   CppiaExpr *link(CppiaData &inData)
   {
      name = inData.strings[nameId];
      inData.markable.push_back(this);
      return this;
   }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      hx::Object *instance = object ? object->runObject(ctx) : ctx->getThis();
      return instance->__Field(name,true).mPtr;
   }

   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(name);
   }
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(name);
   }

};

enum
{
   refObj,
   refThis,
   refStack,
};

inline static int ValToInt( const bool &v ) { return v; }
inline static int ValToInt( const int &v ) { return v; }
inline static int ValToInt( const Float &v ) { return v; }
inline static int ValToInt( const String &v ) { return 0; }
inline static int ValToInt( const hx::Object *v ) { return v->__ToInt(); }

inline static Float ValToFloat( const bool &v ) { return v; }
inline static Float ValToFloat( const int &v ) { return v; }
inline static Float ValToFloat( const Float &v ) { return v; }
inline static Float ValToFloat( const String &v ) { return 0; }
inline static Float ValToFloat( const hx::Object *v ) { return v->__ToDouble(); }

inline static String ValToString( const bool &v ) { return String(v); }
inline static String ValToString( const int &v ) { return String(v); }
inline static String ValToString( const Float &v ) { return String(v); }
inline static String ValToString( const String &v ) { return v; }
inline static String ValToString( const hx::Object *v ) { return v->__ToString(); }

template<typename T>
struct ExprTypeOf { enum { value = etObject }; };
template<> struct ExprTypeOf<int> { enum { value = etInt }; };
template<> struct ExprTypeOf<bool> { enum { value = etInt }; };
template<> struct ExprTypeOf<Float> { enum { value = etFloat }; };
template<> struct ExprTypeOf<String> { enum { value = etString }; };

template<typename T, int REFMODE> 
struct MemReference : public CppiaExpr
{
   int offset;
   CppiaExpr *object;

   #define MEMGETVAL \
     *(T *)( \
         ( REFMODE==refObj ? (char *)object->runObject(ctx) : \
           REFMODE==refThis ?(char *)ctx->getThis() : \
                             (char *)ctx->frame \
         ) + offset )

   MemReference(const CppiaExpr *inSrc, int inOffset, CppiaExpr *inExpr=0)
      : CppiaExpr(inSrc)
   {
      object = inExpr;
      offset = inOffset;
   }
   ExprType getType()
   {
      return (ExprType) ExprTypeOf<T>::value;
   }
   CppiaExpr *link(CppiaData &inData)
   {
      if (object)
         object = object->link(inData);
      return this;
   }


   void        runVoid(CppiaCtx *ctx) { }
   int runInt(CppiaCtx *ctx) { return ValToInt( MEMGETVAL ); }
   Float       runFloat(CppiaCtx *ctx) { return ValToFloat( MEMGETVAL ); }
   ::String    runString(CppiaCtx *ctx) { return ValToString( MEMGETVAL ); }
   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic( MEMGETVAL ).mPtr; }

   CppiaExpr  *makeSetter(CppiaExpr *value);
};

template<typename T, int REFMODE> 
struct MemReferenceSetter : public CppiaExpr
{
   int offset;
   CppiaExpr *object;
   CppiaExpr *value;

   MemReferenceSetter(MemReference<T,REFMODE> *inSrc, CppiaExpr *inValue) : CppiaExpr(inSrc)
   {
      offset = inSrc->offset;
      object = inSrc->object;
      value = inValue;
   }
   ExprType getType()
   {
      return (ExprType) ExprTypeOf<T>::value;
   }

   void        runVoid(CppiaCtx *ctx) { runValue( MEMGETVAL, ctx, value); }
   int runInt(CppiaCtx *ctx) { return ValToInt( runValue(MEMGETVAL,ctx, value ) ); }
   Float       runFloat(CppiaCtx *ctx) { return ValToFloat( runValue(MEMGETVAL,ctx, value) ); }
   ::String    runString(CppiaCtx *ctx) { return ValToString( runValue( MEMGETVAL,ctx, value) ); }

   hx::Object *runObject(CppiaCtx *ctx) { return Dynamic( runValue(MEMGETVAL,ctx,value) ).mPtr; }

};


template<typename T, int REFMODE> 
CppiaExpr *MemReference<T,REFMODE>::makeSetter(CppiaExpr *value)
{
   return new MemReferenceSetter<T,REFMODE>(this,value);
}

struct GetFieldByLinkage : public CppiaExpr
{
   int         fieldId;
   int         typeId;
   CppiaExpr   *object;
  
   GetFieldByLinkage(CppiaStream &stream,bool inThisObject)
   {
      typeId = stream.getInt();
      fieldId = stream.getInt();
      object = inThisObject ? 0 : createCppiaExpr(stream);
   }
   CppiaExpr *link(CppiaData &inData)
   {
      TypeData *type = inData.types[typeId];
      String field = inData.strings[fieldId];
      if (type->haxeClass.mPtr)
      {
         int offset;
         const StorageInfo *store = type->haxeClass.mPtr->GetMemberStorage(field);
         CppiaExpr *replace = 0;
         if (store)
         {
            switch(store->type)
            {
               case fsBool:
                  replace = object ?
                        (CppiaExpr*)new MemReference<bool,refObj>(this,store->offset,object):
                        (CppiaExpr*)new MemReference<bool,refThis>(this,store->offset);
                     break;
               case fsInt:
                  replace = object ?
                        (CppiaExpr*)new MemReference<int,refObj>(this,store->offset,object):
                        (CppiaExpr*)new MemReference<int,refThis>(this,store->offset);
                     break;
               case fsFloat:
                  replace = object ?
                        (CppiaExpr*)new MemReference<Float,refObj>(this,store->offset,object):
                        (CppiaExpr*)new MemReference<Float,refThis>(this,store->offset);
                     break;
               case fsString:
                  replace = object ?
                        (CppiaExpr*)new MemReference<String,refObj>(this,store->offset,object):
                        (CppiaExpr*)new MemReference<String,refThis>(this,store->offset);
                     break;
               case fsObject:
                  replace = object ?
                        (CppiaExpr*)new MemReference<hx::Object *,refObj>(this,store->offset,object):
                        (CppiaExpr*)new MemReference<hx::Object *,refThis>(this,store->offset);
                     break;
               case fsByte:
               case fsUnknown:
                   ;// todo
            }
         }
         if (replace)
         {
            replace = replace->link(inData);
            delete this;
            return replace;
         }
      }
      else
      {
      }

      printf("GetFieldByName dynamic fallback\n");
      CppiaExpr *result = new GetFieldByName(this, fileId, object);
      result = result->link(inData);
      delete this;
      return result;
   }
};




struct StringVal : public CppiaExprWithValue
{
   int stringId;
   String strVal;

   StringVal(int inId) : stringId(inId)
   {
   }

   ExprType getType() { return etString; }

   CppiaExpr *link(CppiaData &inData)
   {
      strVal = inData.strings[stringId];
      return CppiaExprWithValue::link(inData);
   }
   ::String    runString(CppiaCtx *ctx) { return strVal; }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (!value.mPtr)
         value = strVal;
      return value.mPtr;
   }

   void mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(value);
      HX_MARK_MEMBER(strVal);
   }
   void visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(value);
      HX_VISIT_MEMBER(strVal);
   }

};


template<typename T>
struct DataVal : public CppiaExprWithValue
{
   T data;

   DataVal(T inVal) : data(inVal)
   {
   }

   ExprType getType() { return (ExprType)ExprTypeOf<T>::value; }

   void        runVoid(CppiaCtx *ctx) {  }
   int runInt(CppiaCtx *ctx) { return ValToInt(data); }
   Float       runFloat(CppiaCtx *ctx) { return ValToFloat(data); }
   ::String    runString(CppiaCtx *ctx) { return ValToString(data); }

   hx::Object *runObject(CppiaCtx *ctx)
   {
      if (!value.mPtr)
         value = data;
      return value.mPtr;
   }
};



struct NullVal : public CppiaExpr
{
   NullVal() { }
   ExprType getType() { return etObject; }

   void        runVoid(CppiaCtx *ctx) {  }
   int runInt(CppiaCtx *ctx) { return 0; }
   Float       runFloat(CppiaCtx *ctx) { return 0.0; }
   ::String    runString(CppiaCtx *ctx) { return null(); }
   hx::Object  *runObject(CppiaCtx *ctx) { return 0; }

};


struct PosInfo : public CppiaExprWithValue
{
   int fileId;
   int line;
   int classId;
   int methodId;

   PosInfo(CppiaStream &stream)
   {
      fileId = stream.getInt();
      line = stream.getInt();
      classId = stream.getInt();
      methodId = stream.getInt();
   }
   CppiaExpr *link(CppiaData &inData)
   {
      String clazz = inData.strings[classId];
      String file = inData.strings[fileId];
      String method = inData.strings[methodId];
      value = hx::SourceInfo(file,line,clazz,method);
      return CppiaExprWithValue::link(inData);
   }
};


struct VarDecl : public CppiaExpr
{
   CppiaStackVar var;
   CppiaExpr *init;

   VarDecl(CppiaStream &stream,bool inInit)
   {
      var.fromStream(stream);
      init = 0;
      if (inInit)
      {
         int t = stream.getInt();
         init = createCppiaExpr(stream);
         init->haxeTypeId = t;
      }
   }

   void runVoid(CppiaCtx *ctx)
   {
      if (init)
      {
         switch(var.expressionType)
         {
            case etInt: *(int *)(ctx->frame+var.stackPos) = init->runInt(ctx); break;
            case etFloat: *(Float *)(ctx->frame+var.stackPos) = init->runFloat(ctx); break;
            case etString: *(String *)(ctx->frame+var.stackPos) = init->runString(ctx); break;
            case etObject: *(hx::Object **)(ctx->frame+var.stackPos) = init->runObject(ctx); break;
            case etVoid:
            case etNull:
               break;
          }
      }
   }

   CppiaExpr *link(CppiaData &inData)
   {
      var.link(inData);
      init = init ? init->link(inData) : 0;
      return this;
   }
};



struct VarRef : public CppiaExpr
{
   int varId;
   CppiaStackVar *var;
   ExprType type;
   int      stackPos;

   VarRef(CppiaStream &stream)
   {
      varId = stream.getInt();
      var = 0;
      type = etVoid;
      stackPos = 0;
   }

   ExprType getType() { return type; }

   CppiaExpr *link(CppiaData &inData)
   {
      var = inData.layout->varMap[varId];
      if (!var)
         printf("Could not link var %d\n", varId);
      else
      {
         stackPos = var->stackPos;
         type = var->expressionType;

         CppiaExpr *replace = 0;
         switch(type)
         {
            case etInt:
               replace = new MemReference<int,refStack>(this,var->stackPos);
               break;
            case etFloat:
               replace = new MemReference<Float,refStack>(this,var->stackPos);
               break;
            case etString:
               replace = new MemReference<String,refStack>(this,var->stackPos);
               break;
            case etObject:
               replace = new MemReference<hx::Object *,refStack>(this,var->stackPos);
               break;
            case etVoid:
            case etNull:
               break;
         }
         if (replace)
         {
            replace->link(inData);
            delete this;
            return replace;
         }
      }
      return this;
   }
};


struct RetVal : public CppiaExpr
{
   bool      retVal;
   CppiaExpr *value;

   RetVal(CppiaStream &stream,bool inRetVal)
   {
      retVal = inRetVal;
      if (retVal)
      {
         int t = stream.getInt();
         value = createCppiaExpr(stream);
         value->haxeTypeId = t;
      }
      else
         value = 0;
   }

   ExprType getType() { return etVoid; }

   CppiaExpr *link(CppiaData &inData)
   {
      if (value)
         value = value->link(inData);
      return this;
   }

   void runVoid(CppiaCtx *ctx)
   {
      throw value;
   }
   hx::Object *runObject(CppiaCtx *ctx) { runVoid(ctx); return 0; }
   int runInt(CppiaCtx *ctx) { runVoid(ctx); return 0; }
   Float runFloat(CppiaCtx *ctx) { runVoid(ctx); return 0; }
   String runString(CppiaCtx *ctx) { runVoid(ctx); return null(); }

};



struct BinOp : public CppiaExpr
{
   CppiaExpr *left;
   CppiaExpr *right;
   ExprType type;

   BinOp(CppiaStream &stream)
   {
      left = createCppiaExpr(stream);
      right = createCppiaExpr(stream);
      type = etFloat;
   }

   CppiaExpr *link(CppiaData &inData)
   {
      left = left->link(inData);
      right = right->link(inData);

      if (left->getType()==etInt && right->getType()==etInt)
         type = etInt;
      else
         type = etFloat;
      return this;
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      return Dynamic(runFloat(ctx)).mPtr;
   }
};



struct OpMult : public BinOp
{
   OpMult(CppiaStream &stream) : BinOp(stream) { }

   int runInt(CppiaCtx *ctx)
   {
      int lval = left->runInt(ctx);
      return lval * right->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      Float lval = left->runFloat(ctx);
      return lval * right->runFloat(ctx);
   }
};

struct StringAdd : public CppiaExpr
{
   CppiaExpr *left;
   CppiaExpr *right;

   StringAdd(const CppiaExpr *inSrc, CppiaExpr *inLeft, CppiaExpr *inRight)
      : CppiaExpr(inSrc)
   {
      left = inLeft;
      right = inRight;
   }
   String runString(CppiaCtx *ctx)
   {
      String lval = left->runString(ctx);
      return lval + right->runString(ctx);
   }
   hx::Object *runObject(CppiaCtx *ctx)
   {
      return Dynamic(runString(ctx)).mPtr;
   }
};

struct OpAdd : public BinOp
{
   OpAdd(CppiaStream &stream) : BinOp(stream)
   {
   }

   CppiaExpr *link(CppiaData &inData)
   {
      BinOp::link(inData);

      if (left->getType()==etString || right->getType()==etString)
      {
         CppiaExpr *result = new StringAdd(this,left,right);
         delete this;
         return result;
      }
      return this;
   }

   int runInt(CppiaCtx *ctx)
   {
      int lval = left->runInt(ctx);
      return lval + right->runInt(ctx);
   }
   Float runFloat(CppiaCtx *ctx)
   {
      int lval = left->runFloat(ctx);
      return lval + right->runFloat(ctx);
   }
};





CppiaExpr *createCppiaExpr(CppiaStream &stream)
{
   int fileId = stream.getInt();
   int line = stream.getInt();
   std::string tok = stream.getToken();
   printf(" expr %s\n", tok.c_str() );

   CppiaExpr *result = 0;
   if (tok=="FUN")
      result = new FunExpr(stream);
   else if (tok=="BLOCK")
      result = new BlockExpr(stream);
   else if (tok=="IFELSE")
      result = new IfElseExpr(stream);
   else if (tok=="IF")
      result = new IfExpr(stream);
   else if (tok=="ISNULL")
      result = new IsNull(stream);
   else if (tok=="NOTNULL")
      result = new IsNotNull(stream);
   else if (tok=="CALLSTATIC")
      result = new CallStatic(stream,false);
   else if (tok=="CALLSUPER")
      result = new CallStatic(stream,true);
   else if (tok[0]=='s')
      result = new StringVal(atoi(tok.c_str()+1));
   else if (tok[0]=='f')
      result = new DataVal<Float>(atof(tok.c_str()+1));
   else if (tok[0]=='i')
      result = new DataVal<int>(atoi(tok.c_str()+1));
   else if (tok=="true")
      result = new DataVal<bool>(true);
   else if (tok=="false")
      result = new DataVal<bool>(false);
   else if (tok=="POSINFO")
      result = new PosInfo(stream);
   else if (tok=="VAR")
      result = new VarRef(stream);
   else if (tok=="RETVAL")
      result = new RetVal(stream,true);
   else if (tok=="RETURN")
      result = new RetVal(stream,false);
   else if (tok=="CALL")
      result = new Call(stream);
   else if (tok=="FLINK")
      result = new GetFieldByLinkage(stream,false);
   else if (tok=="FTHISINST")
      result = new GetFieldByLinkage(stream,true);
   else if (tok=="FNAME")
      result = new GetFieldByName(stream,false);
   else if (tok=="FTHISNAME")
      result = new GetFieldByName(stream,true);
   else if (tok=="NULL")
      result = new NullVal();
   else if (tok=="VARDECL")
      result = new VarDecl(stream,false);
   else if (tok=="VARDECLI")
      result = new VarDecl(stream,true);
   else if (tok=="NEW")
      result = new NewExpr(stream);
   else if (tok=="SET")
      result = new SetExpr(stream);
   else if (tok=="+")
      result = new OpAdd(stream);
   else if (tok=="*")
      result = new OpMult(stream);

   if (!result)
      throw "invalid expression";

   result->fileId = fileId;
   result->line = line;

   return result;
}

// --- TypeData -------------------------

void TypeData::link(CppiaData &inData)
{
   if (name.length>0)
   {
      haxeClass = Class_obj::Resolve(name);
      if (!haxeClass.mPtr && !cppiaClass && name==HX_CSTRING("int"))
      {
         name = HX_CSTRING("Int");
         haxeClass = Class_obj::Resolve(name);
      }
      if (!haxeClass.mPtr && !cppiaClass && name==HX_CSTRING("bool"))
      {
         name = HX_CSTRING("Bool");
         haxeClass = Class_obj::Resolve(name);
      }
      if (!haxeClass.mPtr && !cppiaClass && (name==HX_CSTRING("float") || name==HX_CSTRING("double")))
      {
         name = HX_CSTRING("Float");
         haxeClass = Class_obj::Resolve(name);
      }

      if (!haxeClass.mPtr && !cppiaClass)
         printf("Could not resolve '%s'\n", name.__s);

      if (name==HX_CSTRING("Void"))
         expressionType = etVoid;
      else if (name==HX_CSTRING("Null"))
         expressionType = etNull;
      else if (name==HX_CSTRING("String"))
         expressionType = etString;
      else if (name==HX_CSTRING("Float"))
         expressionType = etFloat;
      else if (name==HX_CSTRING("Int") || name==HX_CSTRING("Bool"))
         expressionType = etInt;
      else
         expressionType = etObject;
   }
}

// --- CppiaData -------------------------

CppiaData::~CppiaData()
{
   delete main;
   for(int i=0;i<classes.size();i++)
      delete classes[i];
}

void CppiaData::link()
{
   for(int t=0;t<types.size();t++)
      types[t]->link(*this);
   for(int i=0;i<classes.size();i++)
      classes[i]->link();
   if (main)
      main = main->link(*this);
}

/*
ClassData *CppiaData::findClass(String inName)
{
   for(int i=0;i<classes.size();i++)
      if (strings[classes[i]->nameId] == inName)
         return classes[i];
   return 0;
}
*/


void CppiaData::mark(hx::MarkContext *__inCtx)
{
   HX_MARK_MEMBER(strings);
   for(int i=0;i<types.size();i++)
      types[i]->mark(__inCtx);
   for(int i=0;i<markable.size();i++)
      markable[i]->mark(__inCtx);
}

void CppiaData::visit(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(strings);
   for(int i=0;i<types.size();i++)
      types[i]->visit(__inCtx);
   for(int i=0;i<markable.size();i++)
      markable[i]->visit(__inCtx);
}

// TODO  - more than one
static hx::Object *currentCppia = 0;

bool LoadCppia(String inValue)
{
   CppiaData   *cppiaPtr = new CppiaData();
   currentCppia = new CppiaObject(cppiaPtr); 
   GCAddRoot(&currentCppia);


   CppiaData   &cppia = *cppiaPtr;
   CppiaStream stream(inValue.__s, inValue.length);
   try
   {
      std::string tok = stream.getToken();
      if (tok!="CPPIA")
         throw "Bad magic";

      int stringCount = stream.getInt();
      for(int s=0;s<stringCount;s++)
         cppia.strings[s] = stream.readString();

      int typeCount = stream.getInt();
      cppia.types.resize(typeCount);
      for(int t=0;t<typeCount;t++)
         cppia.types[t] = new TypeData(stream.readString());

      int classCount = stream.getInt();
      int enumCount = stream.getInt();

      cppia.classes.resize(classCount);
      for(int c=0;c<classCount;c++)
      {
         cppia.classes[c] = new ClassData(cppia);
         cppia.classes[c]->load(stream);
      }

      tok = stream.getToken();
      if (tok=="MAIN")
      {
         printf("Main...\n");
         cppia.main = createCppiaExpr(stream);
      }
      else if (tok!="NOMAIN")
         throw "no main specified";

      printf("Link...\n");
      cppia.link();
      printf("Run...\n");
      if (cppia.main)
      {
         CppiaCtx ctx;
         //cppia.main->runVoid(&ctx);
         printf("Result %s.\n", cppia.main->runString(&ctx).__s);
      }
      return true;
   } catch(const char *error)
   {
      printf("Error reading file: %s, line %d, char %d\n", error, stream.line, stream.pos);
   }
   return false;
}




// Called by haxe generated code ...

Dynamic ScriptableCall0(void *user, Object *thiz)
{
   return null();
}

Dynamic ScriptableCall1(void *user, Object *thiz,Dynamic arg0)
{
   return null();
}

Dynamic ScriptableCall2(void *user, Object *thiz,Dynamic arg0,Dynamic arg1)
{
   return null();
}

Dynamic ScriptableCall3(void *user, Object *thiz,Dynamic arg0,Dynamic arg1,Dynamic arg2)
{
   return null();
}

Dynamic ScriptableCall4(void *user, Object *thiz,Dynamic arg0,Dynamic arg1,Dynamic arg2,Dynamic arg3)
{
   return null();
}

Dynamic ScriptableCall5(void *user, Object *thiz,Dynamic arg0,Dynamic arg1,Dynamic arg2,Dynamic arg3,Dynamic arg4)
{
   return null();
}

Dynamic ScriptableCallMult(void *user, Object *thiz,Dynamic *inArgs)
{
   return null();
}

void ScriptableMark(void *inClass, hx::Object *inThis, HX_MARK_PARAMS)
{
   //HX_MARK_ARRAY(inInstanceData);
   //inHandler->markFields(inInstanceData,HX_MARK_ARG);
}

void ScriptableVisit(void *inClass, hx::Object *inThis, HX_VISIT_PARAMS)
{
   //HX_VISIT_ARRAY(inInstanceDataPtr);
   //inHandler->visitFields(*inInstanceDataPtr,HX_VISIT_ARG);
}

bool ScriptableField(hx::Object *, const ::String &,bool inCallProp,Dynamic &outResult)
{
   return false;
}

bool ScriptableField(hx::Object *, int inName,bool inCallProp,Float &outResult)
{
   return false;
}

bool ScriptableField(hx::Object *, int inName,bool inCallProp,Dynamic &outResult)
{
   return false;
}

void ScriptableGetFields(hx::Object *inObject, Array< ::String> &outFields)
{
}

bool ScriptableSetField(hx::Object *, const ::String &, Dynamic inValue,bool inCallProp, Dynamic &outValue)
{
   return false;
}


};


void __scriptable_load_cppia(String inCode)
{
   hx::LoadCppia(inCode);
}


