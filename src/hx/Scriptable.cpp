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
#if 0
// Ignore ABC for now
namespace hx
{


enum Type { nsPublic, nsPrivate, nsNamespace, nsInternal, nsProtected, nsExplicit, nsStaticProtected };


enum AbcOpCode
{
   #define DEFINE_OP(name,code) op_##name = code,
   #include "AbcOpCodes.h"
   #undef DEFINE_OP
};

enum MultiNameKind
{
   QNNone       = 0x00,
   QName        = 0x07,
   QNameA       = 0x0D,
   RTQName      = 0x0F,
   RTQNameA     = 0x10,
   RTQNameL     = 0x11,
   RTQNameLA    = 0x12,
   Multiname    = 0x09,
   MultinameA   = 0x0E,
   MultinameL   = 0x1B,
   MultinameLA  = 0x1C,
   TypeName     = 0x1D,
};

enum MethodFlags
{
   NEED_ARGUMENTS  = 0x01,
   NEED_ACTIVATION = 0x02,
   NEED_REST       = 0x04,
   HAS_OPTIONAL    = 0x08,
   SET_DXNS        = 0x40,
   HAS_PARAM_NAMES = 0x80,
};

enum TraitKind
{
   Trait_Slot    = 0,
   Trait_Method  = 1,
   Trait_Getter  = 2,
   Trait_Setter  = 3,
   Trait_Class   = 4,
   Trait_Function= 5,
   Trait_Const   = 6,
};

enum TraitAttr
{
   ATTR_Final    = 0x10,
   ATTR_Override = 0x20,
   ATTR_Metadata = 0x40,
};

enum InstanceFlags
{
   ClassSealed      = 0x01,
   ClassFinal       = 0x02,
   ClassInterface   = 0x04,
   ClassProtectedNs = 0x08,
};

enum ConstantKind
{
   ConstantInt = 0x03,
   ConstantUInt = 0x04,
   ConstantDouble = 0x06,
   ConstantUtf8 = 0x01,
   ConstantTrue = 0x0B,
   ConstantFalse = 0x0A,
   ConstantNull = 0x0C,
   ConstantUndefined = 0x00,
   ConstantNamespace = 0x08,
   ConstantPackageNamespace = 0x16,
   ConstantPackageInternalNs = 0x17,
   ConstantProtectedNamespace = 0x18,
   ConstantExplicitNamespace = 0x19,
   ConstantStaticProtectedNs = 0x1A,
   ConstantPrivateNs = 0x05,
};

struct Namespace
{
   Type type;
   int  index;
};

struct MultiName
{
   MultiName() : kind(QNNone), ns(0), name(0), ns_set(0) { }
   MultiNameKind kind;
   std::vector<int> types;
   int           ns;
   int           name;
   int           ns_set;
};


typedef std::vector<Namespace *> NsSet;

struct Optional
{
   int val;
   int kind;
};

struct Method
{
   Method() { body=0; }

   int paramCount;
   int returnType;
   std::vector<int> paramTypes;
   int name;
   int flags;
   std::vector<Optional> optionals;
   std::vector<int> paramNames;
   struct MethodBody *body;
};

struct KeyValue
{
   int key;
   int value;
};

struct MetaData
{
   int name;
   std::vector<KeyValue> data;
};

struct Trait
{
   int       name;
   int       flags;
   TraitKind kind;

   int slot;
   int type;
   int index;
   ConstantKind vkind;

   std::vector<int> metaData;
};


struct InstanceInfo
{
   int name;
   int superName;
   int flags;
   int protectedNs;
   std::vector<int> interfaces;
   int iinit;
   std::vector<Trait> traits;
};


struct TraitSet
{
   int init;
   std::vector<Trait> traits;
};

struct ExceptionInfo
{
   int from;
   int to;
   int target;
   int excType;
   int varName;
};

struct MethodBody
{
   int method;
   int maxStack;
   int localCount;
   int initScopeDepth;
   int maxScopeDepth;
   std::vector<unsigned char> code;
   std::vector<ExceptionInfo> exception;
   std::vector<Trait> traits;
};


static String DOT = HX_CSTRING(".");
static String FLASH_DOT = HX_CSTRING("flash.");
static String NATIVE_DOT = HX_CSTRING("native.");
static String LANGLE = HX_CSTRING("<");
static String RANGLE = HX_CSTRING(">");
static String COMMA = HX_CSTRING(",");
static String NAME_BOOLEAN = HX_CSTRING("Boolean");
static String NAME_INT = HX_CSTRING("Int");
static String NAME_UINT = HX_CSTRING("UInt");
static String NAME_FLOAT = HX_CSTRING("Number");
static String NAME_STRING = HX_CSTRING("String");

static const char *sOpCodeNames[256];

struct ABC
{
   std::vector<int> mInts;
   std::vector<unsigned int> mUInts;
   std::vector<double> mDoubles;
   std::vector<String> mStrings;
   std::vector<Namespace> mNamespaces;
   std::vector<NsSet> mNsSets;
   std::vector<MultiName> mMultiNames;

   std::vector<Method> mMethods;
   std::vector<MetaData> mMetaData;
   std::vector<InstanceInfo> mInstanceInfo;
   std::vector<TraitSet> mClassInfo;
   std::vector<TraitSet> mScriptInfo;

   std::vector<MethodBody> mMethodBody;

   String getMultiNameSet(int inIdx)
   {
      NsSet &set = mNsSets[inIdx];
      String result = DOT;
      return result;
   }

   String join(const String &s0, const String &joiner, const String &s1)
   {
      if (s0.length==0)
         return s1;
      return s0+joiner+s1;
   }

   String getNamespace(Namespace &inNS)
   {
      return mStrings[inNS.index];
   }

   String getNamespace(int inIdx)
   {
      return mStrings[mNamespaces[inIdx].index];
   }

   String getMultiName(int inIdx)
   {
      MultiName &mn = mMultiNames[inIdx];
      switch(mn.kind)
      {
         case QName      :
         case QNameA     :
            return join(getNamespace(mn.ns),DOT,mStrings[mn.name]);
         case RTQName    :
         case RTQNameLA  :
            return  mStrings[mn.name];
         case RTQNameA   :
         case RTQNameL   :
            return null();
         case Multiname  :
         case MultinameA :
            return join(getMultiNameSet(mn.ns_set),DOT,mStrings[mn.name]);
         case MultinameL :
         case MultinameLA:
            return getMultiNameSet(mn.ns_set);
         case TypeName:
            {
            String result = mStrings[mn.name];
            result+=LANGLE;
            for(int i=0;i<mn.types.size();i++)
            {
               result+=getMultiName(mn.types[i]);
               if (i+1<mn.types.size())
                  result+=COMMA;
            }
            result+=RANGLE;
            return result;
            }

      }
      return null();
   }

   Dynamic getConst(Trait &trait)
   {
      switch(trait.vkind)
      {
         case ConstantInt:
            return mInts[trait.index];
         case ConstantUInt:
            return (int)mUInts[trait.index];
         case ConstantDouble:
            return mDoubles[trait.index];
         case ConstantUtf8:
            return mStrings[trait.index];
         case ConstantTrue:
            return true;
         case ConstantFalse:
            return false;
         case ConstantNull:
            return null();
         case ConstantUndefined:
            return null();

         case ConstantNamespace:
         case ConstantPackageNamespace:
         case ConstantPackageInternalNs:
         case ConstantProtectedNamespace:
         case ConstantExplicitNamespace:
         case ConstantStaticProtectedNs:
         case ConstantPrivateNs:
            return HX_CSTRING("Namespace");
      }
      return HX_CSTRING("???");
   }
};


class ScriptRegistered
{
public:
   std::vector<std::string> mVTableEntries;
   hx::ScriptableClassFactory mFactory;

   ScriptRegistered(String *inFunctions, hx::ScriptableClassFactory inFactory)
   {
      mFactory = inFactory;
      if (inFunctions)
         for(String *func = inFunctions; *func!=null(); func++)
            mVTableEntries.push_back( func->__s );
   }
};


class ScriptRegisteredIntferface
{
public:
   const hx::type_info *mType;
   ScriptableInterfaceFactory mFactory;

   ScriptRegisteredIntferface(hx::ScriptableInterfaceFactory inFactory,const hx::type_info *inType)
   {
      mFactory = inFactory;
      mType = inType;
   }
};


typedef std::map<std::string, ScriptRegistered *> ScriptRegisteredMap;
static ScriptRegisteredMap *sScriptRegistered = 0;
static ScriptRegistered *sObject = 0;

typedef std::map<std::string, ScriptRegisteredIntferface *> ScriptRegisteredInterfaceMap;
static ScriptRegisteredInterfaceMap *sScriptRegisteredInterface = 0;

Dynamic sGetBool(const unsigned char *inPtr) { return *(int *)inPtr != 0; }
void sSetBool(unsigned char *inPtr,Dynamic inVal) { *(int *)inPtr = (bool)inVal; }
double sFGetBool(const unsigned char *inPtr) { return *(int *)inPtr != 0; }
void sFSetBool(unsigned char *inPtr,double inVal) { *(int *)inPtr = (bool)inVal; }

Dynamic sGetInt(const unsigned char *inPtr) { return *(int *)inPtr; }
void sSetInt(unsigned char *inPtr,Dynamic inVal) { *(int *)inPtr = inVal; }
double sFGetInt(const unsigned char *inPtr) { return *(int *)inPtr; }
void sFSetInt(unsigned char *inPtr,double inVal) { *(int *)inPtr = inVal; }

Dynamic sGetDouble(const unsigned char *inPtr) { return *(double *)inPtr; }
void sSetDouble(unsigned char *inPtr,Dynamic inVal) { *(double *)inPtr = inVal; }
double sFGetDouble(const unsigned char *inPtr) { return *(double *)inPtr; }
void sFSetDouble(unsigned char *inPtr,double inVal) { *(double *)inPtr = inVal; }

Dynamic sGetString(const unsigned char *inPtr) { return *(String *)inPtr; }
void sSetString(unsigned char *inPtr,Dynamic inVal) { *(String *)inPtr = inVal; }
double sFGetString(const unsigned char *inPtr) { return 0; }
void sFSetString(unsigned char *inPtr,double inVal) {  }

Dynamic sGetObject(const unsigned char *inPtr) { return *(hx::Object **)inPtr; }
// TODO - cast
void sSetObject(unsigned char *inPtr,Dynamic inVal) { *(hx::Object **)inPtr = inVal.mPtr; }
double sFGetObject(const unsigned char *inPtr) { return Dynamic(*(hx::Object **)inPtr); }
void sFSetObject(unsigned char *inPtr,double inVal) {  *(hx::Object **)inPtr = Dynamic(inVal).mPtr; }



struct FieldInfo
{
   enum Type { typeBool, typeInt, typeDouble, typeString, typeObject };

   FieldInfo(String inName, String inType, int inOffset)
   {
      mName = inName;
      mTypeName = inType.__s ? inType.__s : "Object";
      mOffset = inOffset;
      mHandler = 0;

      if (inName==NAME_BOOLEAN)
      {
         mType = typeBool;
         mGetter = sGetBool;
         mSetter = sSetBool;
         mFGetter = sFGetBool;
         mFSetter = sFSetBool;
         // Aligned ?
         mBytes = 4;
      }
      else if (inName==NAME_INT || inName==NAME_UINT)
      {
         mType = typeInt;
         mGetter = sGetInt;
         mSetter = sSetInt;
         mFGetter = sFGetInt;
         mFSetter = sFSetInt;
         mBytes = 4;
      }
      else if (inName==NAME_FLOAT)
      {
         mType = typeDouble;
         mGetter = sGetDouble;
         mSetter = sSetDouble;
         mFGetter = sFGetDouble;
         mFSetter = sFSetDouble;
         mBytes = sizeof(double);
      }
      else if (inName==NAME_STRING)
      {
         mType = typeString;
         mGetter = sGetString;
         mSetter = sSetString;
         mFGetter = sFGetString;
         mFSetter = sFSetString;
         mBytes = sizeof(String);
      }

      else
      {
         mType = typeObject;
         mGetter = sGetObject;
         mSetter = sSetObject;
         mFGetter = sFGetObject;
         mFSetter = sFSetObject;
         mBytes = sizeof(void *);
      }
   }


   std::string mName;
   std::string mTypeName;
   ScriptHandler *mHandler;
   Type        mType;
   int         mOffset;
   int         mBytes;
   Dynamic     (*mGetter)(const unsigned char *);
   void        (*mSetter)(unsigned char *, Dynamic inValue);
   double      (*mFGetter)(const unsigned char *);
   void        (*mFSetter)(unsigned char *, double inValue);
};

String RemapFlash(String inValue)
{
   if (inValue.substr(0,6)==FLASH_DOT)
      return NATIVE_DOT + inValue.substr(6,null());
   return inValue;
}

typedef std::map< std::string, int> MethodIndexMap;
typedef std::map< std::string, Method *> MethodMap;
typedef std::map< std::string, int> FieldIndexMap;

class ScriptHandler : public hx::Class_obj
{
   hx::Class mSuper;
   ScriptRegistered *mScriptBase;
   ScriptRegistered *mScript;
   std::vector<FieldInfo> mFields;
   std::vector<FieldInfo> mStaticFields;
   std::vector<unsigned char> mStaticData;
   bool mIsInterface;
   int  mDataSize;
   Method **mVTable;
   MethodIndexMap         mMethodMap;
   FieldIndexMap          mFieldMap;
   MethodMap              mStaticMethodMap;
   FieldIndexMap          mStaticFieldMap;
   std::vector<Method *>  mMethods;
   Method                 *mConstructor;
   Method                 *mStaticInit;

public:
   ScriptHandler(ABC &abc, InstanceInfo &inst, TraitSet &cinfo)
   {
      init();

      mName = abc.getMultiName(inst.name);
      String superName = HX_CSTRING("Object");
      mScriptBase = 0;
      mIsInterface = inst.flags & ClassInterface;
      mConstructor = &abc.mMethods[inst.iinit];
      mStaticInit = &abc.mMethods[cinfo.init];

      if (inst.superName>0)
      {
         superName = abc.getMultiName(inst.superName);

         mSuper = hx::Class_obj::Resolve(RemapFlash(superName));

         if (mIsInterface)
            mScriptBase = 0;
         else
            mScriptBase = sScriptRegistered ? (*sScriptRegistered)[RemapFlash(superName).__s] : 0;

         if (mSuper==null())
            superName = HX_CSTRING("Unknown - ") + superName;
      }
      if (!mScriptBase)
         mScriptBase = sObject;

      DBGLOG("Class %s, base %p\n", (mName + HX_CSTRING("::") + superName).__s, mScriptBase );

      RegisterClass(RemapFlash(mName), (hx::Class_obj *)this);

      mScript = mScriptBase;
      (*sScriptRegistered)[mName.__s] = mScript;

      ScriptHandler *superScript = dynamic_cast<ScriptHandler *>(mSuper.GetPtr());
      if (superScript)
      {
         mFields = superScript->mFields;
         mFieldMap = superScript->mFieldMap;
         mMethods = superScript->mMethods;
         mMethodMap = superScript->mMethodMap;
      }
      mDataSize = superScript ? superScript->mDataSize : 0;

      mVTable = new Method*[mScript->mVTableEntries.size()];
      for(int t=0;t<inst.traits.size();t++)
         addInstanceTrait(abc,inst.traits[t]);

      for(int t=0;t<cinfo.traits.size();t++)
         addClassTrait(abc,cinfo.traits[t]);
   }

   ScriptHandler( )
   {
      init();
      mScript = sObject;
   }

   void init()
   {
      mScriptBase = 0;
      mScript = 0;

      mConstructEmpty = 0;
      mConstructArgs = 0;
      mConstructEnum = 0;
      mMarkFunc = 0;
      mVisitFunc = 0;
      mCanCast = 0;
      mStatics = Array_obj<String>::__new(0,0);
      mMembers = Array_obj<String>::__new(0,0);
   }

   void markFields(unsigned char *inData, HX_MARK_PARAMS)
   {
      for(int i=0;i<mFields.size();i++)
      {
         FieldInfo &field = mFields[i];
         if (field.mType==FieldInfo::typeObject)
         {
            HX_MARK_OBJECT( (*(hx::Object **)inData) );
         }
         else if (field.mType==FieldInfo::typeString)
         {
            HX_MARK_STRING( (*(String *)inData).__s );
         }
         inData += field.mBytes;
      }
   }

#ifdef HXCPP_VISIT_ALLOCS
   void visitFields(unsigned char *inData, HX_VISIT_PARAMS)
   {
      for(int i=0;i<mFields.size();i++)
      {
         FieldInfo &field = mFields[i];
         if (field.mType==FieldInfo::typeObject)
         {
            HX_VISIT_OBJECT( (*(hx::Object **)inData) );
         }
         else if (field.mType==FieldInfo::typeString)
         {
            HX_VISIT_STRING( (*(String *)inData).__s );
         }

         inData += field.mBytes;
      }
   }
#endif

   int findVTableSlot(const std::string &inName)
   {
      if (!mScript)
         return -1;
      for(int i=0;i<mScript->mVTableEntries.size();i++)
         if (mScript->mVTableEntries[i]==inName)
            return i;
      return -1;
   }


   void addInstanceTrait(ABC &abc, Trait &trait)
   {
      String name = abc.getMultiName(trait.name);
      switch(trait.kind)
      {
         case Trait_Slot:
            {
            int slot = trait.slot;
            String typeName = abc.getMultiName(trait.type);
            FieldInfo field(name,typeName,mDataSize);
            mDataSize += field.mBytes;
            int fid = mFields.size();
            mFields.push_back(field);
            mFieldMap[name.__s] = fid;
            DBGLOG(" var %s:%s %d\n", name.__s, typeName.__s, slot);
            }
            break;
         case Trait_Method:
         case Trait_Getter:
         case Trait_Setter:
            {
               Method *method = &abc.mMethods[trait.index];
               std::string sname(name.__s);
               int vslot = findVTableSlot(sname);
               if (vslot>=0)
               {
                  mVTable[vslot] =method;
                  DBGLOG(" override native function %s\n", name.__s);
               }
               if (mMethodMap.find(sname)!=mMethodMap.end())
               {
                  int slot = mMethodMap[sname];
                  DBGLOG(" override script function %s (%d)\n", name.__s, slot);
                  mMethods[slot] = method;
               }
               else
               {
                  int slot = mMethods.size();
                  mMethodMap[sname] = slot;
                  mMethods.push_back(method);
               }
            }
            break;
         case Trait_Class:
            DBGLOG(" class ?\n");
            break;
         case Trait_Function:
            DBGLOG(" function ?\n");
            break;
         case Trait_Const:
            {
            DBGLOG(" const %s %s\n",name.__s, abc.getConst(trait)->toString().__s);
            }
            break;
      }
   }


   void addClassTrait(ABC &abc, Trait &trait)
   {
      String name = abc.getMultiName(trait.name);
      switch(trait.kind)
      {
         case Trait_Slot:
            {
            int slot = trait.slot;
            String typeName = abc.getMultiName(trait.type);
            FieldInfo field(name,typeName,mDataSize);
            mStaticData.resize( mStaticData.size() + field.mBytes );
            int fid = mStaticFields.size();
            mStaticFields.push_back(field);
            mStaticFieldMap[name.__s] = fid;
            DBGLOG(" static var %s:%s %d\n", name.__s, typeName.__s, slot);
            }
            break;
         case Trait_Method:
         case Trait_Getter:
         case Trait_Setter:
            {
               std::string sname(name.__s);
               Method *method = &abc.mMethods[trait.index];
               mStaticMethodMap[sname] = method;
               DBGLOG(" static function %s (%d)\n", name.__s, trait.kind);
            }
            break;
         case Trait_Class:
            DBGLOG(" static class ?\n");
            break;
         case Trait_Function:
            DBGLOG(" static function ?\n");
            break;
         case Trait_Const:
            {
            DBGLOG(" static const %s %s\n",name.__s, abc.getConst(trait)->toString().__s);
            }
            break;
      }
   }



   void __Mark(hx::MarkContext *__inCtx)
   {
      hx::Class_obj::__Mark(__inCtx);
      hx::MarkMember(mSuper,__inCtx);
   }
   void MarkStatics(hx::MarkContext *__inCtx)
   {
   }

#ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      hx::Class_obj::__Visit(__inCtx);
      hx::VisitMember(mSuper,__inCtx);
   }
   void VisitStatics(hx::VisitContext *__inCtx)
   {
   }
#endif

   Dynamic ConstructEmpty()
   {
      unsigned char *data = mDataSize == 0 ? 0 : (unsigned char *)InternalNew(mDataSize,false);
      hx::Object *result = mScript->mFactory((void **)mVTable, this, data);
      return result;
   }
   Dynamic ConstructArgs(hx::DynamicArray inArgs)
   {
      unsigned char *data = mDataSize == 0 ? 0 : (unsigned char *)InternalNew(mDataSize,false);
      hx::Object *result = mScript->mFactory((void **)mVTable, this, data);
      result->__Construct(inArgs);
      return result;
   }
   Dynamic ConstructEnum(String inName,hx::DynamicArray inArgs)
   {
      return null();
   }

   bool VCanCast(hx::Object *inPtr) { return false; }
};

typedef ScriptHandler ABCClass_obj;

typedef hx::ObjectPtr<ABCClass_obj> ABCClass;


struct ABCReader
{
   const unsigned char *ptr;
   const unsigned char *end;
   const char *error;
   ABC &abc;

   ABCReader(ABC &inAbc,const unsigned char *inPtr, int inLen) : abc(inAbc)
   {
      ptr = inPtr;
      end = ptr+inLen;
      error = 0;
   }

   int readByte()
   {
      if (ptr>=end)
        setError("Past EOF");
      if (error)
         return 0;
      return *ptr++;
   }

   bool ok() { return !error; }

   void setError(const char *inError)
   {
      throw Dynamic(String(inError,strlen(inError)).dup());
   }
      
   int readInt()
   {
      int a = readByte();
      if( a < 128 )
         return a;
      a &= 0x7F;
      int b = readByte();
      if( b < 128 )
         return (b << 7) | a;
      b &= 0x7F;
      int c = readByte();
      if( c < 128 )
         return (c << 14) | (b << 7) | a;
      c &= 0x7F;
      int d = readByte();
      if( d < 128 )
         return (d << 21) | (c << 14) | (b << 7) | a;
      d &= 0x7F;
      int e = readByte();
      if( e > 15 ) setError("bad integer encoding");
      return (e << 28) | (d << 21) | (c << 14) | (b << 7) | a;
   }
   int readUInt30()
   {
      int ch1 = readByte();
      int ch2 = readByte();
      int ch3 = readByte();
      int ch4 = readByte();
      if( ch4 >= 64 ) setError("Uint30 out of bounds");
      return  ch1 | (ch2 << 8) | (ch3 << 16) | (ch4 << 24);
   }

   void read(int &i) { i = readInt(); }
   void read(unsigned int &i) { i = readInt(); }
   void read(double &d)
   {
      if (ptr+8>=end)
         setError("EOF");
      else 
      {
         memcpy(&d,ptr,sizeof(double));
         ptr+=8;
      }
   }
   void read(String &s)
   {
      int len = readInt();
      if (ptr+len>=end)
         setError("String too long");
      else
      {
         s = String((char *)ptr,len).dup();
         ptr+=len;
         DBGLOG(" -> %s\n", s.__s);
      }
   }
   void read(Namespace &ns)
   {
      int code = readByte();
      switch(code)
      {
         case 0x05: ns.type = nsPrivate; break;
         case 0x08: ns.type = nsNamespace; break;
         case 0x16: ns.type = nsPublic; break;
         case 0x17: ns.type = nsInternal; break;
         case 0x18: ns.type = nsProtected; break;
         case 0x19: ns.type = nsExplicit; break;
         case 0x1A: ns.type = nsStaticProtected; break;
         default:
            //setError("unknown namespace type");
            DBGLOG("unknown namespace type %d\n",code);
      }
      ns.index = readInt();
      DBGLOG("  ns (%d) ::%s\n",  code, abc.mStrings[ns.index].__s);
   }
   void read(MultiName &mn)
   {
      mn.kind = (MultiNameKind)readByte();
      switch(mn.kind)
      {
         case QName      :
         case QNameA     :
             mn.ns = readInt();
             mn.name = readInt();
             break;
         case RTQName    :
         case RTQNameLA  :
             mn.name = readInt();
             break;
         case RTQNameA   :
         case RTQNameL   :
             break;
         case Multiname  :
         case MultinameA :
             mn.name = readInt();
             mn.ns_set = readInt();
             break;
         case MultinameL :
         case MultinameLA:
             mn.ns_set = readInt();
             break;
        case TypeName:
             {
             mn.name = readInt();
             int c = readInt();
             DBGLOG("   %d...\n", c);
             for(int i=0;i<c;i++)
                mn.types.push_back(readInt());
             }
             break;

         default:
             DBGLOG("Kind %02d\n", mn.kind);
             setError("Unknown multi-name constant");
      }
      DBGLOG(" name %s\n", abc.mStrings[mn.name].c_str());
   }

   void read(Optional &optional)
   {
      optional.val = readInt();
      optional.kind = readByte();
   }

   void read(Method &method)
   {
      method.paramCount = readInt();
      method.returnType = readInt();
      readFull(method.paramTypes,method.paramCount);
      method.name = readInt();
      DBGLOG("Method %s(%d)\n", abc.mStrings[method.name].__s, method.paramCount);
      method.flags = readByte();
      if (method.flags & HAS_OPTIONAL)
      {
         //DBGLOG("optionals\n");
         readFull(method.optionals,readInt());
      }
      if (method.flags & HAS_PARAM_NAMES)
      {
         //DBGLOG("params\n");
         readFull(method.paramNames,method.paramCount);
      }
   }

   void read(KeyValue &value)
   {
      value.key = readInt();
      value.value = readInt();
   }

   void read(MetaData &meta)
   {
      meta.name = readInt();
      readFull(meta.data);
   }


   void read(ExceptionInfo &ex)
   {
      ex.from = readInt();
      ex.to = readInt();
      ex.target = readInt();
      ex.excType = readInt();
      ex.varName = readInt();
   }
   inline void read(unsigned char &byte) { byte = readByte(); }

   void read(MethodBody &body)
   {
      body.method = readInt();
      if (body.method<0 || body.method>=abc.mMethods.size())
         setError("Bad method link");
      if (abc.mMethods[body.method].body!=0)
         setError("Duplicate method link");
      abc.mMethods[body.method].body = &body;
      body.maxStack = readInt();
      body.localCount = readInt();
      body.initScopeDepth = readInt();
      body.maxScopeDepth = readInt();
      readFull(body.code);
      readFull(body.exception);
      readFull(body.traits);
   }



   void read(Trait &trait)
   {
      trait.name = readInt();
      trait.flags = readByte();
      trait.kind = (TraitKind)(trait.flags & 0x0f);
      //DBGLOG("  trait %d\n", trait.kind);
      switch(trait.kind)
      {
         case Trait_Slot:
         case Trait_Const:
            trait.slot = readInt();
            trait.type = readInt();
            trait.index = readInt();
            if (trait.index==0)
               trait.vkind = ConstantUndefined;
            else
               trait.vkind = (ConstantKind)readByte();
            break;
         case Trait_Method:
         case Trait_Getter:
         case Trait_Setter:
            trait.slot = readInt();
            trait.index = readInt();
            break;
         case Trait_Class:
            trait.slot = readInt();
            trait.index = readInt();
            break;
         case Trait_Function:
            trait.slot = readInt();
            trait.index = readInt();
            break;
         default:
            setError("Unknown trait type");
      }

      if (trait.flags & ATTR_Metadata)
      {
         readFull(trait.metaData);
      }
   }

   void read(InstanceInfo &inst)
   {
      inst.name = readInt();
      DBGLOG("Instance Name : %s", abc.mStrings[ abc.mMultiNames[inst.name].name].__s);
      inst.superName = readInt();
      DBGLOG(", super : %s\n", abc.mStrings[ abc.mMultiNames[inst.superName].name].__s);
      inst.flags = readByte();
      if (inst.flags & ClassProtectedNs)
         inst.protectedNs = readInt();
      readFull(inst.interfaces);
      inst.iinit = readInt();
      readFull(inst.traits);
   }


   void read(TraitSet &ts)
   {
      ts.init = readInt();
      readFull(ts.traits);
   }



   template<typename T>
   void read(std::vector<T> &list)
   {
      int n = readInt();
      if (n==0)
         n = 1;
         
      DBGLOG(" %d...\n",n);
      list.resize(n);
      for(int i=1;i<n;i++)
         read(list[i]);
   }


   template<typename T>
   void readFull(std::vector<T> &list,int inN=-1)
   {
      int n = inN<0 ? readInt() : inN;
         
      list.resize(n);
      for(int i=0;i<n;i++)
         read(list[i]);
   }

   void readSets(std::vector<NsSet> &outSets)
   {
      int n = readInt();
      if (n==0)
         n = 1;
      outSets.resize(n);
      DBGLOG("Sets %d\n", n);
      for(int i=1;i<n;i++)
      {
         int c = readInt();
         DBGLOG(" [%d]\n", c);
         for(int n=0;n<c;n++)
            outSets[i].push_back( &abc.mNamespaces[readInt()] );
      }
   }


   void read(ABC &abc)
   {
      DBGLOG("Ints...\n");
      read(abc.mInts);
      DBGLOG("UInts...\n");
      read(abc.mUInts);
      DBGLOG("Double...\n");
      read(abc.mDoubles);
      DBGLOG("Strings...\n");
      read(abc.mStrings);
      DBGLOG("Namespaces...\n");
      read(abc.mNamespaces);
      DBGLOG("Namespace sets...\n");
      readSets(abc.mNsSets);
      DBGLOG("Multinames...\n");
      read(abc.mMultiNames);

      DBGLOG("Methods...\n");
      readFull(abc.mMethods);
      DBGLOG("Meta...\n");
      readFull(abc.mMetaData);

      DBGLOG("Classes...\n");
      int classes = readInt();

      DBGLOG("Instance...\n");
      readFull(abc.mInstanceInfo,classes);
      DBGLOG("Classes...\n");
      readFull(abc.mClassInfo,classes);
      DBGLOG("Script...\n");
      readFull(abc.mScriptInfo);

      DBGLOG("Bodies...\n");
      readFull(abc.mMethodBody);

      DBGLOG("Read ABC!\n");
   }

};

class ABCGlobalObject : public hx::Object
{
public:
   ABC *abc;

   ABCGlobalObject(ABC *inAbc) : abc(inAbc)
   {
   }
   Dynamic __Field(const String &inName, hx::PropertyAccess inCallProp)
   {
      return hx::Class_obj::Resolve(RemapFlash(inName));
   }
   bool __HasField(const String &inName)
   {
      hx::Class clazz = hx::Class_obj::Resolve(RemapFlash(inName));
      return clazz.mPtr;
   }


};


class ABCContext
{
public:
   Array<Dynamic> stack;
   Array<Dynamic> scope;
   ABCGlobalObject *global;

   ABCContext(ABCGlobalObject *inGlobal)
   {
      GCAddRoot((hx::Object**)&stack.mPtr);
      GCAddRoot((hx::Object**)&scope.mPtr);
      GCAddRoot((hx::Object**)&global);
      stack = Array_obj<Dynamic>::__new(0,0);
      scope = Array_obj<Dynamic>::__new(0,0);
      global = inGlobal;
      scope->push(global);
   }
   ~ABCContext()
   {
      GCRemoveRoot((hx::Object**)&stack.mPtr);
      GCRemoveRoot((hx::Object**)&scope.mPtr);
      GCRemoveRoot((hx::Object**)&global);
   }

   inline void pushThis(hx::Object *inObj)
   {
      stack->push(inObj ? inObj : global);
   }

   inline void push(Dynamic inValue)
   {
      stack->push(inValue);
   }

   inline void pushScope(Dynamic inValue)
   {
      DBGLOG("   push %p\n", inValue.mPtr);
      scope->push(inValue);
   }

   int getU30(const unsigned char *&pc)
   {
      int a = *pc++;
      if( a < 128 )
         return a;
      a &= 0x7F;
      int b = *pc++;
      if( b < 128 )
         return (b << 7) | a;
      b &= 0x7F;
      int c = *pc++;
      if( c < 128 )
         return (c << 14) | (b << 7) | a;
      c &= 0x7F;
      int d = *pc++;
      if( d < 128 )
         return (d << 21) | (c << 14) | (b << 7) | a;
      d &= 0x7F;
      int e = *pc++;
      if( e > 15 )
         return 0; // ???
      return (e << 28) | (d << 21) | (c << 14) | (b << 7) | a;
   }

   void getLex(int inMultinameId)
   {
      String name = global->abc->getMultiName(inMultinameId);
      for(int s = scope->length-1; s>=0; s--)
      {
         if (scope[s]->__HasField(name))
         {
            stack->push(scope[s]->__Field(name,HX_PROP_DYNAMIC));
            return;
         }
      }
      DBGLOG("getLex %s - not found\n", name.__s);
      hx::Throw( HX_CSTRING("Invalid field") );
   }


   Dynamic call(Method *inMethod,int inArgs)
   {
      DBGLOG("Call, passed %d/%d args\n", inMethod->paramCount, inArgs);
      int local0 = stack->length - inArgs -1;
      for(int i=inArgs; i<inMethod->paramCount;i++)
         push(null());

      MethodBody *body = inMethod->body;
      // Make room for locals...

      const unsigned char *code = &body->code[0];
      const unsigned char *pc = code;
      const unsigned char *end = pc + body->code.size();
      while(pc<end)
      {
         int code = *pc++;
         DBGLOG("op code %d = %s\n", code, sOpCodeNames[code]);
         switch(code)
         {
            case op_getlex: getLex(getU30(pc)); break;
            case op_newclass:
               getU30(pc);
               break;
            case op_initproperty:
               getU30(pc);
               break;
            case op_pushbyte:
               pc++;
               break;
            case op_pushint:
               getU30(pc);
               break;
            case op_returnvoid:
               return null();
         }
      }

      printf("Missing return statement?\n");
      return null();
   }
};


// TODO: Thread local storage
ABCContext *gABCContext = 0;
inline ABCContext *GetABCContext()
{
   return gABCContext;
}

void CreateABCContext(ABCGlobalObject *inGlobal)
{
   gABCContext = new ABCContext(inGlobal);
}


// TODO - toString?
class Object_obj__scriptable : public Object
{
   typedef Object_obj__scriptable __ME;
   typedef Object super;

   void __construct() { }
   HX_DEFINE_SCRIPTABLE(HX_ARR_LIST0)
   HX_DEFINE_SCRIPTABLE_DYNAMIC;
};



void ScriptableRegisterClass( String inName, String *inFunctions, hx::ScriptableClassFactory inFactory)
{
   printf("ScriptableRegisterClass %s\n", inName.__s);
   if (!sScriptRegistered)
      sScriptRegistered = new ScriptRegisteredMap();
   ScriptRegistered *registered = new ScriptRegistered(inFunctions,inFactory);
   (*sScriptRegistered)[inName.__s] = registered;
   //printf("Registering %s -> %p\n",inName.__s,(*sScriptRegistered)[inName.__s]);
}


void ScriptableRegisterInterface( String inName, const hx::type_info *inType,
                                 hx::ScriptableInterfaceFactory inFactory)
{
   if (!sScriptRegisteredInterface)
      sScriptRegisteredInterface = new ScriptRegisteredInterfaceMap();
   ScriptRegisteredIntferface *registered = new ScriptRegisteredIntferface(inFactory,inType);
   (*sScriptRegisteredInterface)[inName.__s] = registered;
   //printf("Registering Interface %s -> %p\n",inName.__s,(*sScriptRegisteredInterface)[inName.__s]);
}


void InitABC()
{
   String allFunctions = null();
   String *__scriptableFunctionNames = &allFunctions;


   RegisterClass(HX_CSTRING("Object"), new ScriptHandler());
   HX_SCRIPTABLE_REGISTER_CLASS("Object",Object_obj);
   sObject = (*sScriptRegistered)["Object"];

   for(int i=0;i<256;i++)
      sOpCodeNames[i] = "unknown";

   #define DEFINE_OP(op,code) sOpCodeNames[code] = #op;
   #include "AbcOpCodes.h"
   #undef DEFINE_OP
}

Dynamic ScriptableCall0(void *user, Object *thiz)
{
   Method *method = (Method *)user;
   ABCContext *context = GetABCContext();
   context->pushThis(thiz);
   return context->call(method,0);
}

Dynamic ScriptableCall1(void *user, Object *thiz,Dynamic arg0)
{
   Method *method = (Method *)user;
   ABCContext *context = GetABCContext();
   context->pushThis(thiz);
   context->push(arg0);
   return context->call(method,1);
}

Dynamic ScriptableCall2(void *user, Object *thiz,Dynamic arg0,Dynamic arg1)
{
   Method *method = (Method *)user;
   ABCContext *context = GetABCContext();
   context->pushThis(thiz);
   context->push(arg0);
   context->push(arg1);
   return context->call(method,2);
}

Dynamic ScriptableCall3(void *user, Object *thiz,Dynamic arg0,Dynamic arg1,Dynamic arg2)
{
   Method *method = (Method *)user;
   ABCContext *context = GetABCContext();
   context->pushThis(thiz);
   context->push(arg0);
   context->push(arg1);
   context->push(arg2);
   return context->call(method,3);
}

Dynamic ScriptableCall4(void *user, Object *thiz,Dynamic arg0,Dynamic arg1,Dynamic arg2,Dynamic arg3)
{
   Method *method = (Method *)user;
   ABCContext *context = GetABCContext();
   context->pushThis(thiz);
   context->push(arg0);
   context->push(arg1);
   context->push(arg2);
   context->push(arg3);
   return context->call(method,4);
}

Dynamic ScriptableCall5(void *user, Object *thiz,Dynamic arg0,Dynamic arg1,Dynamic arg2,Dynamic arg3,Dynamic arg4)
{
   Method *method = (Method *)user;
   ABCContext *context = GetABCContext();
   context->pushThis(thiz);
   context->push(arg0);
   context->push(arg1);
   context->push(arg2);
   context->push(arg3);
   context->push(arg4);
   return context->call(method,5);
}

Dynamic ScriptableCallMult(void *user, Object *thiz,Dynamic *inArgs)
{
   Method *method = (Method *)user;

   ABCContext *context = GetABCContext();
   context->pushThis(thiz);
   int argc = method->paramNames.size();
   for(int i=0;i<argc;i++)
      context->push(inArgs[i]);
   return context->call(method,argc);
}

void ScriptableMark(ScriptHandler *inHandler, unsigned char *inInstanceData, HX_MARK_PARAMS)
{
   HX_MARK_ARRAY(inInstanceData);
   inHandler->markFields(inInstanceData,HX_MARK_ARG);
}

#ifdef HXCPP_VISIT_ALLOCS
void ScriptableVisit(ScriptHandler *inHandler, unsigned char **inInstanceDataPtr, HX_VISIT_PARAMS)
{
   HX_VISIT_ARRAY(inInstanceDataPtr);
   inHandler->visitFields(*inInstanceDataPtr,HX_VISIT_ARG);
}
#endif

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


static bool sAbcInit = false;
void LoadABC(const unsigned char *inBytes, int inLen)
{
   if (!sAbcInit)
   {
      sAbcInit = true;
      InitABC();
   }

   ABC *abcInstance = new ABC();
   ABC &abc = *abcInstance;
   ABCReader stream(abc, inBytes,inLen);

   int version = stream.readUInt30();
   if (version!= 0x002E0010 )
   {
      printf("Unknown version\n");
      return;
   }

   try
   {
      stream.read(abc);

      for(int i=0;i<abc.mInstanceInfo.size();i++)
      {
         InstanceInfo &inst = abc.mInstanceInfo[i];
         String className = abc.getMultiName(inst.name);

         hx::Class cls = hx::Class_obj::Resolve(RemapFlash(className));
         if (cls==null())
         {
            ABCClass cls = new ABCClass_obj(abc, inst, abc.mClassInfo[i]);
            RegisterClass(className,cls);
         }
         else
         {
            DBGLOG("Already defined %s\n", className.__s);
         }
      }

      for(int i=0;i<abc.mScriptInfo.size();i++)
      {
         TraitSet &traits = abc.mScriptInfo[i];
         int scriptInit = traits.init;
         //printf("Init function %p (%d)\n", &abc.mMethods[scriptInit], scriptInit);

         for(int t=0;t<traits.traits.size();t++)
         {
            Trait &trait = traits.traits[t];
            String name = abc.getMultiName(trait.name);
            switch(trait.kind)
            {
               case Trait_Slot:
                  {
                  printf(" script var %s\n", name.__s );
                  }
                  break;
               case Trait_Method:
               case Trait_Getter:
               case Trait_Setter:
                  {
                  printf(" script function %s ?\n", name.__s );
                  }
                  break;
               case Trait_Class:
                  //printf(" script class %d->%d\n", trait.index, trait.slot);
                  break;
               case Trait_Function:
                  //printf(" script function %d->%d\n", trait.index, trait.slot);
                  break;
               case Trait_Const:
                  {
                  printf(" script const %s %s\n",name.__s, abc.getConst(trait)->toString().__s);
                  }
                  break;
             }
          }
      }

      int entry = abc.mScriptInfo[ abc.mScriptInfo.size()-1 ].init;

      CreateABCContext(new ABCGlobalObject(abcInstance));
      ScriptableCall0(&abc.mMethods[entry],0);
   }
   catch (Dynamic d)
   {
      printf("Error on load : %s\n", d->toString().__s);
   }
}

#endif

#if 0


enum OpCode
{
   opAccNull,
   opAccTrue,
   opAccFalse,
   opAccThis,
   opAccInt,
   opAccStack,
   opAccGlobal,
   opAccEnv,
   opAccField,
   opAccArray,
   opAccIndex,
   opAccBuiltin,
   opSetStack,
   opSetGlobal,
   opSetEnv,
   opSetField,
   opSetArray,
   opSetIndex,
   opSetThis,
   opPush,
   opPop,
   opCall,
   opObjCall,
   opJump,
   opJumpIf,
   opJumpIfNot,
   opTrap,
   opEndTrap,
   opRet,
   opMakeEnv,
   opMakeArray,
   opBool,
   opIsNull,
   opIsNotNull,
   opAdd,
   opSub,
   opMult,
   opDiv,
   opMod,
   opShl,
   opShr,
   opUShr,
   opOr,
   opAnd,
   opXor,
   opEq,
   opNeq,
   opGt,
   opGte,
   opLt,
   opLte,
   opNot,
   opTypeOf,
   opCompare,
   opHash,
   opNew,
   opJumpTable,
   opApply,
   opAccStack0,
   opAccStack1,
   opAccIndex0,
   opAccIndex1,
   opPhysCompare,
   opTailCall,

   opLast,
};


const char *opNames[] =
{
   "opAccNull",
   "opAccTrue",
   "opAccFalse",
   "opAccThis",
   "opAccInt",
   "opAccStack",
   "opAccGlobal",
   "opAccEnv",
   "opAccField",
   "opAccArray",
   "opAccIndex",
   "opAccBuiltin",
   "opSetStack",
   "opSetGlobal",
   "opSetEnv",
   "opSetField",
   "opSetArray",
   "opSetIndex",
   "opSetThis",
   "opPush",
   "opPop",
   "opCall",
   "opObjCall",
   "opJump",
   "opJumpIf",
   "opJumpIfNot",
   "opTrap",
   "opEndTrap",
   "opRet",
   "opMakeEnv",
   "opMakeArray",
   "opBool",
   "opIsNull",
   "opIsNotNull",
   "opAdd",
   "opSub",
   "opMult",
   "opDiv",
   "opMod",
   "opShl",
   "opShr",
   "opUShr",
   "opOr",
   "opAnd",
   "opXor",
   "opEq",
   "opNeq",
   "opGt",
   "opGte",
   "opLt",
   "opLte",
   "opNot",
   "opTypeOf",
   "opCompare",
   "opHash",
   "opNew",
   "opJumpTable",
   "opApply",
   "opAccStack0",
   "opAccStack1",
   "opAccIndex0",
   "opAccIndex1",
   "opPhysCompare",
   "opTailCall",

   "opLast",
};



class NekoModule
{
public:
   NekoModule(const char *inFilename)
   {
      init();

      FILE *file = fopen(inFilename,"rb");
      if (!file)
      {
         printf("Could not load module: %s\n", inFilename);
      }
      else
      {
         mFile = file;
         try {
            Load();
         }
         catch (const char *inError)
         {
            printf("Error on load : %s\n", inError);
         }
         mFile = 0;
         fclose(file);
      }
   }

   NekoModule(unsigned char *inBytes, int inLength)
   {
      init();
      mBytes = inBytes;
      mBytesEnd = mBytes + inLength;
      try {
          Load();
      }
      catch (const char *inError)
      {
         printf("Error on load : %s\n", inError);
      }
   }

   void init()
   {
      mFile = 0;
      mBytesEnd = 0;
      mBytes = 0;
      mGlobalCount = 0;
      mFieldCount = 0;
      mCodeSize = 0;
      mOpCodes = 0;
      mOk = false;
   }

   ~NekoModule()
   {
      delete [] mOpCodes;
   }

   void DumpOp(const int *inOp)
   {
      int idx = inOp - mOpCodes;
      int arg = inOp[1];
      DBGLOG(" %03d %s", idx, opNames[*inOp]);
      switch(*inOp)
      {
         case opAccField:
            {
            unsigned int fid = mFieldHash[arg & 0x7fffffff];
            DBGLOG(" %s", mFields[fid].__s);
            break;
            }
         case opAccGlobal:
            if (arg>=mGlobalCount || arg<0)
            {
               DBGLOG(" %d ???", arg );
            }
            else
            {
               DBGLOG(" %s", !mGlobals[arg].mPtr ? "(null)" : mGlobals[arg]->__ToString().c_str() );
            }
            break;
      }
      DBGLOG("\n", idx, opNames[*inOp]);
   }

   void Error(const char *inMsg)
   {
      throw Dynamic(String(inMsg,strlen(inMsg)).dup());
   }

   int ReadByte()
   {
      if (mBytes)
      {
         if (mBytes+1>mBytesEnd)
            Error("Unexpected end of data");
         return *mBytes++;
      }
      unsigned char byte;
      if (fread(&byte,1,1,mFile)!=1)
         Error("Unexpected end of stream");
      return byte;
   }

   int ReadInt()
   {
      int i;
      if (mBytes)
      {
         if (mBytes+4>mBytesEnd)
            Error("Unexpected end of data");
         i = *(int *)mBytes;
         mBytes += 4;
      }
      else if (fread(&i,4,1,mFile)!=1)
         Error("Unexpected end of stream");
      return i;
   }

   int ReadUInt16()
   {
      unsigned short i;
      if (mBytes)
      {
         if (mBytes+2>mBytesEnd)
            Error("Unexpected end of data");
         i = *(unsigned short *)mBytes;
         mBytes += 2;
      }
      else if (fread(&i,2,1,mFile)!=1)
         Error("Unexpected end of stream");
      return i;
   }


   const char *ReadString()
   {
      int i = 0;
      char c;
      while( i < sizeof(mStringBuf) )
      {
         char c = ReadByte();
         mStringBuf[i++] = c;
         if( c == 0 )
            return mStringBuf;
      }
      Error("Unterminated string");
      return "";
   }

   Dynamic ReadString16()
   {
      int len = ReadUInt16();
      if (len==0)
         return String("",0).dup();

      char *buffer = (char *)malloc(len);
      if (mBytes)
      {
         if (mBytes+len>mBytesEnd)
            Error("Could not read string16 data");
         memcpy(buffer,mBytes,len);
         mBytes += len;
      }
      else if (fread(buffer,len,1,mFile)!=1)
      {
         free(buffer);
         Error("Could not read string16");
      }
      Dynamic result = String(buffer,len).dup();
      free(buffer);
      return result;
   }


   void ReadDebugInfo()
   {
      bool lot_of_files = false;
      int nfiles = 0;

      // TODO:
      int c = ReadByte();
      if( c >= 0x80 )
      {
         int c2 = ReadByte();
         nfiles = ((c & 0x7F) << 8) | c2;
         lot_of_files = true;
      } else
         nfiles = c;

      if( nfiles == 0 )
         Error("No debug files");

      for(int i=0;i<nfiles;i++)
         ReadString();

      int npos = ReadInt();
      if( npos != mCodeSize )
         Error("Code size mismatch");

      int i = 0;
      while( i < npos )
      {
         int c = ReadByte();
         if( c & 1 )
         {
            if( lot_of_files )
               ReadByte();
         }
         else if( c & 2 )
         {
            int delta = c >> 6;
            int count = (c >> 2) & 15;
            if( i + count > npos )
               Error("Bad line count");
            i+= count;
         }
         else if( c & 4 )
         {
            i++;
         }
         else
         {
            unsigned char b2 = ReadByte();
            unsigned char b3 = ReadByte();
            i++;
         }
      }
   }

   unsigned int NekoHash(const char *name)
   {
      unsigned int result = 0;
      while( *name )
      {
        result = (223 * result + *((unsigned char*)name));
        name++;
      }
      return result & 0x7fffffff;
   }


   void Load( )
   {
      unsigned int magic = ReadInt();
      if (magic!=0x4F4B454E)
         Error("Bad Magic");

      mGlobalCount = ReadInt();
      mFieldCount = ReadInt();
      mCodeSize = ReadInt();

      DBGLOG("globs=%d, fields=%d, code=%d\n", mGlobalCount, mFieldCount, mCodeSize );
      mGlobals = Array_obj<Dynamic>::__new(mGlobalCount, mGlobalCount);

      int version = 1;

      for(int g=0;g<mGlobalCount;g++)
      {
         int code = ReadByte();
         switch(code)
         {
            case 1:
               mGlobals[g] = String(ReadString()).dup();
               // null
               break;

            case 2:
               {
               int combined = ReadInt();
               int pos = combined &0xffffff;
               int extra = combined >> 24;
               if (pos>=mCodeSize)
                  Error("Bad code size");
               mGlobals[g] = String("Function @") + String(pos);
               break;
               }

            case 3:
               mGlobals[g] = ReadString16();
               break;

            case 4:
               mGlobals[g] = atof(ReadString());
               break;

            case 5:
               ReadDebugInfo();
               break;

            case 6:
               version = ReadByte();
               mGlobals[g] = null();
               break;

            default:
               DBGLOG("Code %d\n",code);
               Error("Unknown global code");
         }
      }
      DBGLOG("Read globals %d (v=%d)\n",mGlobalCount, version);

      DBGLOG("Read fields...\n");
      mFields = Array_obj<String>::__new(mFieldCount);
      for(int f=0;f<mFieldCount;f++)
      {
         mFields[f] = String(ReadString()).dup();
         unsigned int id = NekoHash( mFields[f].__s );
         mFieldHash[id] = f;
         DBGLOG(" %08x -> %d\n", id,f );
      }
      DBGLOG("Read fields\n");

      DBGLOG("Unpack op codes...\n");

      int idx = 0;
      // Unpack opcodes
      mOpCodes = new int[mCodeSize+1];
      while( idx < mCodeSize )
      {
         int t = ReadByte();
         switch( t & 3 )
         {
         case 0:
            mOpCodes[idx++] = (t >> 2);
            DumpOp(mOpCodes+idx-1);
            break;
         case 1:
            mOpCodes[idx++] = (t >> 3);
            mOpCodes[idx++] = (t >> 2) & 1;
            DumpOp(mOpCodes+idx-2);
            break;
         case 2:
            mOpCodes[idx++] = (t >> 2);
            mOpCodes[idx++] = ReadByte();
            DumpOp(mOpCodes+idx-2);
            break;
         case 3:
            mOpCodes[idx++] = (t >> 2);
            mOpCodes[idx++] = ReadInt();
            DumpOp(mOpCodes+idx-2);
            break;
         }
      }
      mOpCodes[idx] = opLast;
      mEntry = (int)mOpCodes[1];
      DBGLOG("Read %d op codes\n",idx);
   }

   Array<Dynamic> mGlobals;
   Array<String>  mFields;
   std::map<unsigned int,int> mFieldHash;

   bool mOk;
   int mGlobalCount;
   int mFieldCount;
   int mCodeSize;
   int mEntry;
   int *mOpCodes;
   char mStringBuf[256];

   FILE *mFile;
   const unsigned char *mBytes;
   const unsigned char *mBytesEnd;
};
#endif

} // end namespace hx


#if 0
void __scriptable_load_neko(String inName)
{
   new hx::NekoModule(inName.__s);
}


void __scriptable_load_neko_bytes(Array<unsigned char> inBytes)
{
   new hx::NekoModule((unsigned char *)inBytes->GetBase(), inBytes->length);
}

void __scriptable_load_abc(Array<unsigned char> inBytes)
{
   hx::LoadABC(&inBytes[0], inBytes->length);
}
#endif


