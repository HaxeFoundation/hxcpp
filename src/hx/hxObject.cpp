#include <hxObject.h>
#include <stdio.h>
#include <hxMath.h>
#include <hxMacros.h>
#include <cpp/CppInt32__.h>
#include <map>
#include <ctype.h>
#include <algorithm>
#include <typeinfo>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#include <time.h>
#else
#include <sys/time.h>
#include <wchar.h>
typedef  uint64_t  __int64;
#endif

// Stoopid windows ...
#ifdef RegisterClass
#undef RegisterClass
#endif
#ifdef abs
#undef abs
#endif


static String sNone[] = { String(null()) };

void StringBoot();

Class __BoolClass;
Class __IntClass;
Class __FloatClass;
Class __VoidClass;
Class __StringClass;
Class Class_obj__mClass;


// --- boot -----------------------------------------

bool NoCast(hxObject *) { return false; }


const wchar_t *hxConvertToWChar(const char *inStr, int *ioLen=0)
{
   int len = ioLen ? *ioLen : strlen(inStr);

   wchar_t *result = hxNewString(len);
   int l = 0;

   unsigned char *b = (unsigned char *)inStr;
   for(int i=0;i<len;)
   {
      int c = b[i++];
      if (c==0) break;
      else if( c < 0x80 )
      {
        result[l++] = c;
      }
      else if( c < 0xE0 )
        result[l++] = ( ((c & 0x3F) << 6) | (b[i++] & 0x7F) );
      else if( c < 0xF0 )
      {
        int c2 = b[i++];
        result[l++] += ( ((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | ( b[i++] & 0x7F) );
      }
      else
      {
        int c2 = b[i++];
        int c3 = b[i++];
        result[l++] += ( ((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 << 6) & 0x7F) | (b[i++] & 0x7F) );
      }
   }
   result[l] = '\0';
   if (ioLen)
      *ioLen = l;
   return result;
}


void hxCheckOverflow(int x)
{
   if( (((x) >> 30) & 1) != ((unsigned int)(x) >> 31) )
      throw Dynamic(String(L"Overflow ")+x);
}

// Field name management


#include <string>
#include <vector>

#ifdef INTERNAL_GC
typedef std::vector<String> FieldToString;
#else
typedef Array<String> FieldToString;
#endif

typedef std::map<std::string,int> StringToField;

// These need to be pointers because of the unknown order of static object construction.
FieldToString *sgFieldToString=0;
StringToField *sgStringToField=0;

static String sgNullString;




// --- GC helper

static wchar_t *GCStringDup(const wchar_t *inStr,int &outLen)
{
   if (inStr==0)
   {
      outLen = 0;
      return 0;
   }

   outLen = wcslen(inStr);
   if (outLen==0)
      return 0;

   wchar_t *result = hxNewString(outLen);
   memcpy(result,inStr,sizeof(wchar_t)*(outLen+1));
   return result;
}


// --- hxObject -----------------------------------------


Dynamic hxObject::__IField(int inFieldID)
{
   return __Field( __hxcpp_field_from_id(inFieldID) );
}

int hxObject::__Compare(const hxObject *inRHS) const
{
   return (int)(inRHS-this);
}


void *hxObject::__root()
{
	return this;
}

Dynamic hxObject::__Field(const String &inString) { return null(); }
// TODO: this is not quite correct...
bool hxObject::__HasField(const String &inString) { return __Field(inString)!=null(); }
Dynamic hxObject::__Run(const Array<Dynamic> &inArgs) { return 0; }
Dynamic hxObject::__GetItem(int inIndex) const { return Dynamic(); }
void hxObject::__SetItem(int inIndex,Dynamic) {  }
DynamicArray hxObject::__EnumParams() { return DynamicArray(); }
String hxObject::__Tag() const { return L"<not enum>"; }
int hxObject::__Index() const { return -1; }

void hxObject::__SetThis(Dynamic inThis) { }

bool hxObject::__Is(Dynamic inClass ) const { return __Is(inClass.GetPtr()); }

static Class hxObject__mClass;

bool AlwaysCast(hxObject *inPtr) { return inPtr!=0; }

void hxObject::__boot()
{
   Static(hxObject__mClass) = RegisterClass(STRING(L"Dynamic",7),AlwaysCast,sNone,sNone,0,0, 0 );
}

Class &hxObject::__SGetClass() { return hxObject__mClass; }

Class hxObject::__GetClass() const { return hxObject__mClass; }

hxFieldRef hxObject::__FieldRef(const String &inString) { return hxFieldRef(this,inString); }

String hxObject::__ToString() const { return L"Object"; }

char * hxObject::__CStr() const { return __ToString().__CStr(); }


Dynamic hxObject::__SetField(const String &inField,const Dynamic &inValue)
{
	throw Dynamic( String(L"Invalid field:") + inField );
	return null();
}

Dynamic hxObject::__run()
{
   return __Run(Array_obj<Dynamic>::__new());
}

Dynamic hxObject::__run(D a)
{
   return __Run( Array_obj<Dynamic>::__new(0,1) << a );
}

Dynamic hxObject::__run(D a,D b)
{
   return __Run( Array_obj<Dynamic>::__new(0,2) << a << b );
}

Dynamic hxObject::__run(D a,D b,D c)
{
   return __Run( Array_obj<Dynamic>::__new(0,3) << a << b << c);
}
Dynamic hxObject::__run(D a,D b,D c,D d)
{
   return __Run( Array_obj<Dynamic>::__new(0,4) << a << b << c << d);
}
Dynamic hxObject::__run(D a,D b,D c,D d,D e)
{
   return __Run( Array_obj<Dynamic>::__new(0,4) << a << b << c << d << e);
}
Dynamic hxObject::__run(D a,D b,D c,D d,D e,D f)
{
   return __Run( Array_obj<Dynamic>::__new(0,5) << a << b << c << d << e << f);
}
Dynamic hxObject::__run(D a,D b,D c,D d,D e,D f,D g)
{
   return __Run( Array_obj<Dynamic>::__new(0,6) << a << b << c << d << e << f << g);
}
Dynamic hxObject::__run(D a,D b,D c,D d,D e,D f,D g,D h)
{
   return __Run( Array_obj<Dynamic>::__new(0,7) << a << b << c << d << e << f << g << h);
}
Dynamic hxObject::__run(D a,D b,D c,D d,D e,D f,D g,D h,D i)
{
   return __Run( Array_obj<Dynamic>::__new(0,8) << a << b << c << d << e << f << g << h << i);
}
Dynamic hxObject::__run(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j)
{
   return __Run( Array_obj<Dynamic>::__new(0,8) << a << b << c << d << e << f << g << h << i << j);
}
Dynamic hxObject::__run(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j,D k)
{
   return __Run( Array_obj<Dynamic>::__new(0,9) << a << b << c << d << e << f << g << h << i << j<< k );
}

void hxObject::__GetFields(Array<String> &outFields) { }


String hxObject::toString() { return __ToString(); }



// --- "Simple" Data Objects ---------------------------------------------------


Class &GetBoolClass() { return __BoolClass; }
Class &GetIntClass() { return __IntClass; }
Class &GetFloatClass() { return __FloatClass; }
Class &GetVoidClass() { return __VoidClass; }
Class &GetStringClass() { return __StringClass; }


class IntData : public hxObject
{
public:
   IntData(int inValue=0) : mValue(inValue) {};

   Class __GetClass() const { return __IntClass; }
   bool __Is(hxObject *inClass) const { return dynamic_cast< IntData *>(inClass); }

   virtual int __GetType() const { return vtInt; }

   String toString() { return String(mValue); }
   String __ToString() const { return String(mValue); }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return mValue; }

   int __Compare(const hxObject *inRHS) const
   {
      double diff = inRHS->__ToDouble() - mValue;
      return diff < 0 ? -1 : diff==0 ? 0 : 1;
   }


   int mValue;
};


class BoolData : public hxObject
{
public:
   BoolData(bool inValue=false) : mValue(inValue) {};

   Class __GetClass() const { return __BoolClass; }
   bool __Is(hxObject *inClass) const { return dynamic_cast< BoolData *>(inClass); }

   virtual int __GetType() const { return vtBool; }

   String __ToString() const { return mValue ? L"true" : L"false"; }
   String toString() { return mValue ? L"true" : L"false"; }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return mValue; }

   int __Compare(const hxObject *inRHS) const
   {
      double diff = inRHS->__ToDouble() - (double)mValue;
      return diff < 0 ? -1 : diff==0 ? 0 : 1;
   }


   bool mValue;
};



class DoubleData : public hxObject
{
public:
   DoubleData(double inValue=0) : mValue(inValue) {};

   Class __GetClass() const { return __FloatClass; }
   bool __Is(hxObject *inClass) const { return dynamic_cast< DoubleData *>(inClass); }

   virtual int __GetType() const { return vtFloat; }
   String toString() { return String(mValue); }
   String __ToString() const { return String(mValue); }
   double __ToDouble() const { return mValue; }
   int __ToInt() const { return (int)mValue; }

   int __Compare(const hxObject *inRHS) const
   {
      double diff = inRHS->__ToDouble() - (double)mValue;
      return diff < 0 ? 1 : diff==0 ? 0 : -1;
   }


   double mValue;
};


#ifndef _WIN32
inline double _wtof(const wchar_t *inStr)
{
   return wcstod(inStr,0);
}


inline int _wtoi(const wchar_t *inStr)
{
   return wcstol(inStr,0,0);
}
#endif


class StringData : public hxObject
{
public:
   StringData(String inValue=null()) : mValue(inValue) {};

   Class __GetClass() const { return __StringClass; }
   bool __Is(hxObject *inClass) const { return dynamic_cast< StringData *>(inClass); }

   virtual int __GetType() const { return vtString; }
   String __ToString() const { return mValue; }
   String toString() { return mValue; }
   double __ToDouble() const
   {
      if (!mValue.__s) return 0;
      return _wtof(mValue.__s);
   }
   int __length() const { return mValue.length; }

   void __Mark()
   {
      MarkMember(mValue);
   }

   int __ToInt() const
   {
      if (!mValue.__s) return 0;
      return _wtoi(mValue.__s);
   }

   int __Compare(const hxObject *inRHS) const
   {
      return mValue.compare( const_cast<hxObject*>(inRHS)->toString() );
   }

   Dynamic __Field(const String &inString)
   {
      return mValue.__Field(inString);
   }


   String mValue;
};



static bool IsFloat(hxObject *inPtr)
{
	return inPtr && (TCanCast<IntData>(inPtr) || TCanCast<DoubleData>(inPtr));
}


void __boot_hxcpp()
{
   hxGCInit();


   //__hxcpp_enable(false);

   hxObject::__boot();

   Static(__BoolClass) = RegisterClass(STRING(L"Bool",4),TCanCast<BoolData>,sNone,sNone, 0,0, 0 );
   Static(__IntClass) = RegisterClass(STRING(L"Int",3),TCanCast<IntData>,sNone,sNone,0,0, 0 );
   Static(__FloatClass) = RegisterClass(STRING(L"Float",5),IsFloat,sNone,sNone, 0,0,&__IntClass );
   Static(__VoidClass) = RegisterClass(STRING(L"Void",4),NoCast,sNone,sNone,0,0,0, 0 );
   Static(Class_obj__mClass) = RegisterClass(STRING(L"Class",5),TCanCast<Class_obj>,sNone,sNone, 0,0 , 0 );

   StringBoot();

   hxAnon_obj::__boot();
   hxArrayBase::__boot();
   hxEnumBase_obj::__boot();
   Math_obj::__boot();
}





// --- Dynamic -------------------------------------------------


Dynamic::Dynamic(bool inVal) : super( new BoolData(inVal) ) { }
Dynamic::Dynamic(int inVal) : super( new IntData(inVal) ) { }
Dynamic::Dynamic(double inVal) : super( new DoubleData(inVal) ) { }
Dynamic::Dynamic(const cpp::CppInt32__ &inVal) : super( new IntData((int)inVal) ) { }
Dynamic::Dynamic(const String &inVal) : super( inVal.__s ? new StringData(inVal) : 0 ) { }
Dynamic::Dynamic(const wchar_t *inVal) : super( inVal ? new StringData(String(inVal)) : 0 ) { }


Dynamic Dynamic::operator+(const Dynamic &inRHS) const
{
   int t1 = mPtr ? mPtr->__GetType() : vtNull;
   int t2 = inRHS.mPtr ? inRHS.mPtr->__GetType() : vtNull;

   if ( (t1==vtInt || t1==vtFloat)  &&  (t2==vtInt || t2==vtFloat) )
   {
      return mPtr->__ToDouble() + inRHS.mPtr->__ToDouble();
   }
   if (!mPtr)
      return inRHS;
   if (!inRHS.mPtr)
      return this;

   return const_cast<hxObject*>(mPtr)->toString() + const_cast<Dynamic&>(inRHS)->toString();
}

Dynamic Dynamic::operator+(const int &i) const
{
   int t = mPtr ? mPtr->__GetType() : vtNull;
   if (t==vtString)
      return Cast<String>() + String(i);
   return Cast<Int>() + i;
}

Dynamic Dynamic::operator+(const double &d) const
{
   int t = mPtr ? mPtr->__GetType() : vtNull;
   if (t==vtString)
      return Cast<String>() + String(d);
   return Cast<double>() + d;
}


double Dynamic::operator%(const Dynamic &inRHS) const
{
   if (mPtr->__GetType()==vtInt && inRHS.mPtr->__GetType()==vtInt)
      return mPtr->__ToInt() % inRHS->__ToInt();
   double lhs = mPtr->__ToDouble();
   double rhs = inRHS->__ToDouble();
   int even = (int)(lhs/rhs);
   double remain = lhs - even * rhs;
   if (remain<0) remain += fabs(rhs);
   return remain;
}


void Dynamic::CheckFPtr()
{
	if (!mPtr)
		throw NULL_FUNCTION_POINTER;
}


/*
Dynamic::operator haxe::Int32 () const
{
   return mPtr ? mPtr->__ToInt() : 0;
}
*/



// -----

String hxAnon_obj::__ToString() const { return STR(L"Anon"); }


Dynamic hxAnon_obj::__Create(DynamicArray inArgs) { return hxAnon(new hxAnon_obj); }

Class hxAnon_obj::__mClass;


void hxAnon_obj::__boot()
{
   Static(__mClass) = RegisterClass(STR(L"__Anon"),TCanCast<hxAnon_obj>,sNone,sNone,0,0,0,0);
}

bool __hx_anon_remove(hxAnon inObj,String inKey)
{
	return inObj->__Remove(inKey);
}


// -------- hxArrayBase -------------------------------------

hxArrayBase::hxArrayBase(int inSize,int inReserve,int inElementSize,bool inAtomic)
{
   length = inSize;
   mAlloc = inSize < inReserve ? inReserve : inSize;
   if (mAlloc)
   {
      mBase = (char *)( (!inAtomic) ?
        hxNewGCBytes(0, mAlloc * inElementSize ) : hxNewGCPrivate(0,mAlloc*inElementSize));
   }
   else
      mBase = 0;
}


void hxArrayBase::EnsureSize(int inSize) const
{
   int s = inSize;
   if (s>length)
   {
      if (s>mAlloc)
      {
         int obytes = mAlloc * GetElementSize();
         mAlloc = s*3/2 + 10;
         int bytes = mAlloc * GetElementSize();
         if (mBase)
         {
            mBase = (char *)hxGCRealloc(mBase, bytes );
            // atomic data not cleared by gc lib ...
				#ifndef GC_CLEARS_ALL
				   #ifndef GC_CLEARS_OBJECTS
               if (AllocAtomic())
				   #endif
            		memset(mBase + obytes, 0, bytes-obytes);
				#endif
         }
         else if (AllocAtomic())
         {
            mBase = (char *)hxNewGCPrivate(0,bytes);
            // atomic data not cleared ...
				#ifndef GC_CLEARS_ALL
            		memset(mBase,0,bytes);
				#endif
         }
         else
			{
            mBase = (char *)hxNewGCBytes(0,bytes);
				#ifndef GC_CLEARS_OBJECTS
            		memset(mBase,0,bytes);
				#endif
			}
      }
      length = s;
   }
}




String hxArrayBase::__ToString() const { return "Array"; }
String hxArrayBase::toString()
{
   // Byte-array
   if (GetElementSize()==1)
   {
      return String( (const char *) mBase, length);
   }

   int n = __length();
   String result(L"[",1);
   for(int i=0;i<n;i++)
   {
      result+=(String)__GetItem(i);
      if (i+1<n)
         result+=String(L", ",1);
   }
   result+=String(L"]",1);
   return result;
}

void hxArrayBase::__SetSize(int inSize)
{
   if (inSize<length)
   {
      int s = GetElementSize();
      memset(mBase + inSize*s, 0, (length-inSize)*s);
      length = inSize;
   }
   else if (inSize>length)
   {
      EnsureSize(inSize);
      length = inSize;
   }
}


void hxArrayBase::Insert(int inPos)
{
   if (inPos>=length)
      __SetSize(length+1);
   else
   {
      __SetSize(length+1);
      int s = GetElementSize();
      memmove(mBase + inPos*s + s, mBase+inPos*s, (length-inPos-1)*s );
   }
}

void hxArrayBase::Splice(hxArrayBase *outResult,int inPos,int inLen)
{
   if (inPos>=length)
   {
      outResult->__SetSize(0);
      return;
   }
   if (inPos+inLen>length)
      inLen = length - inPos;

   outResult->__SetSize(inLen);
   int s = GetElementSize();
   memcpy(outResult->mBase, mBase+inPos*s, s*inLen);
   memmove(mBase+inPos*s, mBase + (inPos+inLen)*s, (length-(inPos+inLen))*s);
   __SetSize(length-inLen);
}

void hxArrayBase::Slice(hxArrayBase *outResult,int inPos,int inEnd)
{
   if (inPos<0) inPos += length;
   if (inEnd<0) inEnd += length + 1;
   int n = inEnd - inPos;
   if (n<=0)
      outResult->__SetSize(0);
   else
   {
      outResult->__SetSize(n);
      int s = GetElementSize();
      memcpy(outResult->mBase, mBase+inPos*s, n*s);
   }
}

void hxArrayBase::RemoveElement(int inPos)
{
   if (inPos<length)
   {
      int s = GetElementSize();
      memmove(mBase + inPos*s, mBase+inPos*s + s, (length-inPos-1)*s );
      __SetSize(length-1);
   }

}

void hxArrayBase::Concat(hxArrayBase *outResult,const char *inSecond,int inLen)
{
   char *ptr =  outResult->GetBase();
   int n = length * GetElementSize();
   memcpy(ptr,mBase,n);
   ptr += n;
   memcpy(ptr,inSecond,inLen*GetElementSize());

}


String hxArrayBase::join(String inSeparator)
{
   int len = 0;
   for(int i=0;i<length;i++)
   {
      len += ItemString(i).length;
   }
   if (length) len += (length-1) * inSeparator.length;

   wchar_t *buf = hxNewString(len);

   int pos = 0;
   bool separated = inSeparator.length>0;
   for(int i=0;i<length;i++)
   {
      String s = ItemString(i);
      memcpy(buf+pos,s.__s,s.length*sizeof(wchar_t));
      pos += s.length;
      if (separated)
      {
         memcpy(buf+pos,inSeparator.__s,inSeparator.length*sizeof(wchar_t));
         pos += inSeparator.length;
      }
   }
   buf[len] = '\0';

   return String(buf,len);
}

#define DEFINE_ARRAY_FUNC(func,array_list,dynamic_arg_list,arg_list) \
struct hxArrayBase_##func : public hxObject \
{ \
   bool __IsFunction() const { return true; } \
   hxArrayBase *mThis; \
   hxArrayBase_##func(hxArrayBase *inThis) : mThis(inThis) { } \
   String toString() const{ return L###func ; } \
   String __ToString() const{ return L###func ; } \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      return mThis->__##func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      return mThis->__##func(arg_list); return Dynamic(); \
   } \
}; \
Dynamic hxArrayBase::func##_dyn()  { return new hxArrayBase_##func(this);  }


#define DEFINE_ARRAY_FUNC0(func) DEFINE_ARRAY_FUNC(func,ARRAY_LIST0,DYNAMIC_ARG_LIST0,ARG_LIST0)
#define DEFINE_ARRAY_FUNC1(func) DEFINE_ARRAY_FUNC(func,ARRAY_LIST1,DYNAMIC_ARG_LIST1,ARG_LIST1)
#define DEFINE_ARRAY_FUNC2(func) DEFINE_ARRAY_FUNC(func,ARRAY_LIST2,DYNAMIC_ARG_LIST2,ARG_LIST2)


DEFINE_ARRAY_FUNC1(concat);
DEFINE_ARRAY_FUNC2(insert);
DEFINE_ARRAY_FUNC0(iterator);
DEFINE_ARRAY_FUNC1(join);
DEFINE_ARRAY_FUNC0(pop);
DEFINE_ARRAY_FUNC1(push);
DEFINE_ARRAY_FUNC1(remove);
DEFINE_ARRAY_FUNC0(reverse);
DEFINE_ARRAY_FUNC0(shift);
DEFINE_ARRAY_FUNC2(slice);
DEFINE_ARRAY_FUNC2(splice);
DEFINE_ARRAY_FUNC1(sort);
DEFINE_ARRAY_FUNC0(toString);
DEFINE_ARRAY_FUNC1(unshift);

Dynamic hxArrayBase::__Field(const String &inString)
{
   if (inString==STR(L"length")) return Dynamic((int)size());
   if (inString==STR(L"concat")) return concat_dyn();
   if (inString==STR(L"insert")) return insert_dyn();
   if (inString==STR(L"iterator")) return iterator_dyn();
   if (inString==STR(L"join")) return join_dyn();
   if (inString==STR(L"pop")) return pop_dyn();
   if (inString==STR(L"push")) return push_dyn();
   if (inString==STR(L"remove")) return remove_dyn();
   if (inString==STR(L"reverse")) return reverse_dyn();
   if (inString==STR(L"shift")) return shift_dyn();
   if (inString==STR(L"splice")) return splice_dyn();
   if (inString==STR(L"slice")) return slice_dyn();
   if (inString==STR(L"sort")) return sort_dyn();
   if (inString==STR(L"toString")) return toString_dyn();
   if (inString==STR(L"unshift")) return unshift_dyn();
   return null();
}


static String sArrayFields[] = {
	STRING(L"length",6),
	STRING(L"concat",6),
	STRING(L"insert",6),
	STRING(L"iterator",8),
	STRING(L"join",4),
	STRING(L"pop",3),
	STRING(L"push",4),
	STRING(L"remove",6),
	STRING(L"reverse",7),
	STRING(L"shift",5),
	STRING(L"slice",5),
	STRING(L"splice",6),
	STRING(L"sort",4),
	STRING(L"toString",8),
	STRING(L"unshift",7),
	String(null())
};



// TODO;
Class hxArrayBase::__mClass;

void hxArrayBase::__boot()
{
   Static(__mClass) = RegisterClass(STRING(L"Array",5),TCanCast<hxArrayBase>,sArrayFields,sNone,0,0,0,0);
}



// -------- ArrayIterator -------------------------------------

DEFINE_DYNAMIC_FUNC0(ArrayIterator,hasNext,return)
DEFINE_DYNAMIC_FUNC0(ArrayIterator,next,return)

Dynamic ArrayIterator::__Field(const String &inString)
{
   if (inString==STR(L"hasNext")) return hasNext_dyn();
   if (inString==STR(L"next")) return next_dyn();
   return null();
}

void  ArrayIterator::__Mark()
{
	MarkMember(mArray);
}

// -------- String ----------------------------------------

#ifdef _MSC_VER

#define WPRINTF wprintf

// vc 7...
#if _MSC_VER < 1400 

#define SPRINTF _snwprintf
#define SSCANF _snwscanf

#else // vc8+

#define SPRINTF _snwprintf_s
#define SSCANF _snwscanf_s

#endif

#else // not _MSC_VER ..
#define SPRINTF swprintf
#define SSCANF(str,len,fmt,ptr) swscanf(str,fmt,ptr)
#define WPRINTF printf
#endif

String::String(const Dynamic &inRHS)
{
   if (inRHS.GetPtr())
      (*this)=const_cast<Dynamic &>(inRHS)->toString();
   else
   {
      __s = 0;
      length = 0;
   }
}


String::String(const int &inRHS)
{
   wchar_t buf[100];
   SPRINTF(buf,100,L"%d",inRHS);
   __s = GCStringDup(buf,length);
}


String::String(const cpp::CppInt32__ &inRHS)
{
   wchar_t buf[100];
   SPRINTF(buf,100,L"%d",inRHS.mValue);
   __s = GCStringDup(buf,length);
}

// Construct from utf8 string
String::String(const char *inPtr,int inLen)
{
   length = inLen<0 ? 0 : inLen;
   if (inPtr)
   {
      if (inLen<0)
         length = strlen(inPtr);
      __s = hxConvertToWChar(inPtr,&length);
   }
   else
   {
      __s = hxNewString(length);
   }
}

String::String(const wchar_t *inStr)
{
   __s = GCStringDup(inStr,length);
}





String::String(const double &inRHS)
{
   wchar_t buf[100];
   SPRINTF(buf,100,L"%g",inRHS);
   __s = GCStringDup(buf,length);
}

String::String(const bool &inRHS)
{
   if (inRHS)
   {
      *this = STR(L"true");
   }
   else
   {
      *this = STR(L"false");
   }
}

String String::__URLEncode() const
{
   Array<unsigned char> bytes(0,length);
   // utf8-encode
   __hxcpp_bytes_of_string(bytes,*this);

   int extra = 0;
   bool has_space = false;
   int utf8_chars = bytes->__length();
   for(int i=0;i<utf8_chars;i++)
      if ( !isalnum(bytes[i]) && bytes[i]!=' ' && bytes[i]!='-')
         extra++;
      else if (bytes[i]==' ')
         has_space = true;
   if (extra==0 && !has_space)
      return *this;

   int l = utf8_chars + extra*2;
   wchar_t *result = hxNewString(l);
   wchar_t *ptr = result;
   bool has_plus = false;

   for(int i=0;i<utf8_chars;i++)
   {
      if ( bytes[i]==' ')
      {
         *ptr++ = '+';
      }
      else if ( !isalnum(bytes[i]) && bytes[i]!='-' )
      {
         static wchar_t hex[] = L"0123456789ABCDEF";
         unsigned char b = bytes[i];
         *ptr++ = '%';
         *ptr++ = hex[ b>>4 ];
         *ptr++ = hex[ b & 0x0f ];
      }
      else
         *ptr++ = bytes[i];
   }
   return String(result,l);
}

String String::toUpperCase() const
{
   wchar_t *result = hxNewString(length);
   for(int i=0;i<length;i++)
      result[i] = toupper( __s[i] );
   return String(result,length);
}

String String::toLowerCase() const
{
   wchar_t *result = hxNewString(length);
   for(int i=0;i<length;i++)
      result[i] = tolower( __s[i] );
   return String(result,length);
}


static int hex(int inChar)
{
   if (inChar>='0' && inChar<='9')
      return inChar-'0';
   if (inChar>='a' && inChar<='f')
      return inChar-'a' + 10;
   if (inChar>='A' && inChar<='F')
      return inChar-'A' + 10;
   return 0;
}

String String::__URLDecode() const
{
   Array<unsigned char> bytes(0,length);
   for(int i=0;i<length;i++)
   {
      int c = __s[i];
      if (c>127)
         bytes->push('?');
      else if (c=='+')
         bytes->push(' ');
      else if (c=='%')
      {
         i++;
         int h0 = __s[i];
         if (h0!=0)
         {
            i++;
            int h1 = __s[i];
            bytes->push( (hex(h0)<<4) | hex(h1) );
         }
      }
      else
         bytes->push(c);
   }
   // utf8 -> unicode ...
   int len = bytes->__length();
   bytes->push(' ');
   return String( bytes->GetBase(), len );
}


String &String::dup()
{
   __s = GCStringDup(__s,length);
   return *this;
}

int String::indexOf(const String &inValue, Dynamic inStart) const
{
   if (__s==0)
      return -1;
   int s = inStart==null() ? 0 : inStart->__ToInt();
   int l = inValue.length;
   if (l==1)
   {
      wchar_t test = *inValue.__s;
      while(s<length)
      {
         if (__s[s]==test)
            return s;
         ++s;
      }
   }
   else
   {
      while(s<=length-l)
      {
         if (!memcmp(__s + s,inValue.__s,l*sizeof(wchar_t)))
            return s;
         s++;
      }
   }
   return -1;
}



int String::lastIndexOf(const String &inValue, Dynamic inStart) const
{
   if (__s==0)
      return -1;
   int l = inValue.length;
   if (l>length) return -1;
   int s = inStart==null() ? length : inStart->__ToInt();
   if (s+l>length) s = length-l;

   if (l==1)
   {
      wchar_t test = *inValue.__s;
      while(s>=0)
      {
         if (__s[s]==test)
            return s;
         --s;
      }
   }
   else
   {
      while(s>=0)
      {
         if (!memcmp(__s + s,inValue.__s,l*sizeof(wchar_t)))
            return s;
         --s;
      }
   }
   return -1;
}





Dynamic String::charCodeAt(int inPos) const
{
   if (inPos<0 || inPos>=length)
      return null();

   return (int)(__s[inPos]);
}



String String::fromCharCode( int c )
{
   wchar_t *result = hxNewString(1);
   result[0] = c;
   result[1] = '\0';
   return String(result,1);
}

String String::charAt( int at ) const
{
   if (at<0 || at>=length) return STR(L"");
   return fromCharCode(__s[at]);
}

void __hxcpp_bytes_of_string(Array<unsigned char> &outBytes,const String &inString)
{
   for(int i=0;i<inString.length;i++)
   {
      int c = inString.__s[i];
      if( c <= 0x7F )
         outBytes->push(c);
      else if( c <= 0x7FF )
      {
         outBytes->push( 0xC0 | (c >> 6) );
         outBytes->push( 0x80 | (c & 63) );
      }
      else if( c <= 0xFFFF )
      {
         outBytes->push( 0xE0 | (c >> 12) );
         outBytes->push( 0x80 | ((c >> 6) & 63) );
         outBytes->push( 0x80 | (c & 63) );
      }
      else
      {
         outBytes->push( 0xF0 | (c >> 18) );
         outBytes->push( 0x80 | ((c >> 12) & 63) );
         outBytes->push( 0x80 | ((c >> 6) & 63) );
         outBytes->push( 0x80 | (c & 63) );
      }
   }
}

void __hxcpp_string_of_bytes(Array<unsigned char> &inBytes,String &outString,int pos,int len)
{
   const unsigned char *ptr = (unsigned char *)inBytes->GetBase() + pos;
   const unsigned char *last = ptr + len;
   wchar_t *result = hxNewString(len);
   wchar_t *out = result;

   // utf8-encode
   while( ptr < last )
   {
      int c = *ptr++;
      if( c < 0x80 )
      {
         if( c == 0 ) break;
         *out++ = c;
      }
      else if( c < 0xE0 )
      {
         *out++ = ((c & 0x3F) << 6) | ((*ptr++) & 0x7F);
      }
      else if( c < 0xF0 )
      {
         int c2 = *ptr++;
         *out++ += ((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | ( (*ptr++) & 0x7F);
      }
      else
      {
         int c2 = *ptr++;
         int c3 = *ptr++;
         *out++ +=((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 << 6) & 0x7F) | ((*ptr++) & 0x7F);
      }
   }
   int l = out - result;
   *out++ = '\0';

   outString = String(result,l);
}

char * String::__CStr() const
{
   Array<unsigned char> bytes(0,length+1);
   __hxcpp_bytes_of_string(bytes,*this);
	bytes.Add(0);
   char *result =  bytes->GetBase();
   if (result)
      return result;
   return (char *)"";
}



Array<String> String::split(const String &inDelimiter) const
{
   int len = inDelimiter.length;
   int pos = 0;
   int last = 0;
   if (len==0)
   {
      // Special case of splitting into characters - use efficient code...
      int chars = length;
      Array<String> result(chars,chars);
      for(int i=0;i<chars;i++)
      {
         wchar_t *ptr = hxNewString(1);
         ptr[0] = __s[i];
         ptr[1] = '\0';
         result[i] = String(ptr,1);
      }
      return result;
   }

   Array<String> result(0,1);
   while(pos+len <=length )
   {
      if (!wcsncmp(__s+pos,inDelimiter.__s,len))
      {
         result.Add( substr(last,pos-last) );
         pos += len;
         last = pos;
      }
      else
      {
         pos++;
      }
   }

   result.Add( substr(last,null()) );

   return result;
}

Dynamic CreateEmptyString() { return String(); }

Dynamic CreateString(DynamicArray inArgs)
{
   if (inArgs->__length()>0)
      return inArgs[0]->toString();
   return String();
}


String String::substr(int inFirst, Dynamic inLen) const
{
   int len = inLen == null() ? -1 : inLen->__ToInt();

   if (inFirst>=length || length==0) return STRING(L"",0);
   if (inFirst<0)
      inFirst += length;
   if (inFirst<0)
      inFirst = 0;
   if (len<0 || (len+inFirst > length) ) len = length - inFirst;
   if (len==0)
      return STRING(L"",0);

   wchar_t *ptr = hxNewString(len);
   memcpy(ptr,__s+inFirst,len*sizeof(wchar_t));
   ptr[len] = 0;
   return String(ptr,len);
}

String String::operator+(String inRHS) const
{
   if (!__s) return String(L"null",4) + inRHS;
   if (!length) return inRHS;
   if (!inRHS.__s) return *this + String(L"null",4);
   if (!inRHS.length) return *this;
   int l = length + inRHS.length;
   wchar_t *result = hxNewString(l);
   memcpy(result,__s,length*sizeof(wchar_t));
   memcpy(result+length,inRHS.__s,inRHS.length*sizeof(wchar_t));
   result[l] = '\0';
   return String(result,l);
}


String &String::operator+=(String inRHS)
{
   if (length==0)
   {
      *this = inRHS;
   }
   else if (inRHS.length>0)
   {
      int l = length + inRHS.length;
      wchar_t *s = hxNewString(l);
      memcpy(s,__s,length*sizeof(wchar_t));
      memcpy(s+length,inRHS.__s,inRHS.length*sizeof(wchar_t));
      s[l] = '\0';
      __s = s;
      length = l;
   }
   return *this;
}


#define DEFINE_STRING_FUNC(func,array_list,dynamic_arg_list,arg_list) \
struct __String_##func : public hxObject \
{ \
   bool __IsFunction() const { return true; } \
   String *mThis; \
   __String_##func(String *inThis) : mThis(inThis) { } \
   String toString() const{ return L###func ; } \
   String __ToString() const{ return L###func ; } \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      return mThis->func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      return mThis->func(arg_list); return Dynamic(); \
   } \
}; \
Dynamic String::func##_dyn()  { return new __String_##func(this);  }


#define DEFINE_STRING_FUNC0(func) DEFINE_STRING_FUNC(func,ARRAY_LIST0,DYNAMIC_ARG_LIST0,ARG_LIST0)
#define DEFINE_STRING_FUNC1(func) DEFINE_STRING_FUNC(func,ARRAY_LIST1,DYNAMIC_ARG_LIST1,ARG_LIST1)
#define DEFINE_STRING_FUNC2(func) DEFINE_STRING_FUNC(func,ARRAY_LIST2,DYNAMIC_ARG_LIST2,ARG_LIST2)

DEFINE_STRING_FUNC1(charAt);
DEFINE_STRING_FUNC1(charCodeAt);
DEFINE_STRING_FUNC2(indexOf);
DEFINE_STRING_FUNC2(lastIndexOf);
DEFINE_STRING_FUNC1(split);
DEFINE_STRING_FUNC2(substr);
DEFINE_STRING_FUNC0(toLowerCase);
DEFINE_STRING_FUNC0(toUpperCase);
	DEFINE_STRING_FUNC0(toString);

Dynamic String::__Field(const String &inString)
{
   if (inString==STR(L"length")) return length;
   if (inString==STR(L"charAt")) return charAt_dyn();
   if (inString==STR(L"charCodeAt")) return charCodeAt_dyn();
   if (inString==STR(L"indexOf")) return indexOf_dyn();
   if (inString==STR(L"lastIndexOf")) return lastIndexOf_dyn();
   if (inString==STR(L"split")) return split_dyn();
   if (inString==STR(L"substr")) return substr_dyn();
   if (inString==STR(L"toLowerCase")) return toLowerCase_dyn();
   if (inString==STR(L"toUpperCase")) return toUpperCase_dyn();
   if (inString==STR(L"toString")) return toString_dyn();
   return null();
}


static String sStringStatics[] = {
   STRING(L"fromCharCode",12),
   String(null())
};
static String sStringFields[] = {
   STRING(L"length",6),
   STRING(L"charAt",6),
   STRING(L"charCodeAt",10),
   STRING(L"indexOf",7),
   STRING(L"lastIndexOf",11),
   STRING(L"split",5),
   STRING(L"substr",6),
   STRING(L"toLowerCase",11),
   STRING(L"toUpperCase",11),
   STRING(L"toString",8),
   String(null())
};

/*
Dynamic String::__Field(const String &inString)
{
	return null();
}
*/

void StringBoot()
{
   Static(__StringClass) = RegisterClass(STRING(L"String",6),TCanCast<StringData>,sStringStatics, sStringFields,
            &CreateEmptyString, &CreateString, &hxObject__mClass);
}



STATIC_DEFINE_DYNAMIC_FUNC1(String,fromCharCode,return )


Dynamic ParseInt(const String &inString)
{
	wchar_t *end = 0;
	long result = wcstol(inString.__s,&end,0);
	if (inString.__s==end)
		return null();
	return result;
}

double ParseFloat(const String &inString)
{
	wchar_t *end;
   double result =  wcstod(inString.__s,&end);
	if (end==inString.__s)
		return std::numeric_limits<double>::infinity();
	return result;
}


// -------- Class ---------------------------------------

typedef std::map<String,Class> ClassMap;
static ClassMap *sClassMap = 0;

Class_obj::Class_obj(const String &inClassName,String inStatics[], String inMembers[],
             ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
             Class *inSuperClass,ConstructEnumFunc inConstructEnum,
             CanCastFunc inCanCast, MarkFunc inFunc)
{
   mName = inClassName;
   mSuper = inSuperClass;
   mConstructEmpty = inConstructEmpty;
   mConstructArgs = inConstructArgs;
   mConstructEnum = inConstructEnum;
   mMarkFunc = inFunc;
   mStatics = Array_obj<String>::__new(0,0);
   for(String *s = inStatics; s->length; s++)
      mStatics->Add( *s );
   mMembers = Array_obj<String>::__new(0,0);
   for(String *m = inMembers; m->length; m++)
      mMembers->Add( *m );
   CanCast = inCanCast;
}

Class Class_obj::GetSuper()
{
   if (!mSuper)
      return null();
   return *mSuper;
}

void Class_obj::__Mark()
{
   MarkMember(mName);
   MarkMember(mStatics);
   MarkMember(mMembers);
}


Class  Class_obj::__GetClass() const { return Class_obj__mClass; }
Class &Class_obj::__SGetClass() { return Class_obj__mClass; }


Class RegisterClass(const String &inClassName, CanCastFunc inCanCast,
                    String inStatics[], String inMembers[],
                    ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
                    Class *inSuperClass, ConstructEnumFunc inConstructEnum,
                    MarkFunc inMarkFunc)
{
   if (sClassMap==0)
      sClassMap = new ClassMap;

   Class_obj *obj = new Class_obj(inClassName, inStatics, inMembers,
                                  inConstructEmpty, inConstructArgs, inSuperClass,
                                  inConstructEnum, inCanCast, inMarkFunc);
   Class c(obj);
   (*sClassMap)[inClassName] = c;
   return c;
}

void Class_obj::MarkStatics()
{
   if (mMarkFunc)
		mMarkFunc();
}

Class Class_obj::Resolve(String inName)
{
   ClassMap::const_iterator i = sClassMap->find(inName);
   if (i==sClassMap->end())
      return null();
   return i->second;
}


String Class_obj::__ToString() const { return mName; }


Array<String> Class_obj::GetInstanceFields()
{
   Array<String> result = mSuper ? (*mSuper)->GetInstanceFields() : Array<String>(0,0);
   for(int m=0;m<mMembers->size();m++)
   {
      const String &mem = mMembers[m];
      if (result->Find(mem)==-1)
         result.Add(mem);
   }
   return result;
}

Array<String> Class_obj::GetClassFields()
{
   Array<String> result = mSuper ? (*mSuper)->GetClassFields() : Array<String>(0,0);
   for(int s=0;s<mStatics->size();s++)
   {
      const String &mem = mStatics[s];
      if (result->Find(mem)==-1)
         result.Add(mem);
   }
   return result;
}

Dynamic Class_obj::__Field(const String &inString)
{
   // Not the most efficient way of doing this!
   if (!mConstructEmpty)
      return null();
   Dynamic instance = mConstructEmpty();
   return instance->__Field(inString);
}

Dynamic Class_obj::__SetField(const String &inString,const Dynamic &inValue)
{
   // Not the most efficient way of doing this!
   if (!mConstructEmpty)
      return null();
   Dynamic instance = mConstructEmpty();
   return instance->__SetField(inString,inValue);
}

void hxMarkClassStatics()
{
   ClassMap::iterator end = sClassMap->end();
   for(ClassMap::iterator i = sClassMap->begin(); i!=end; ++i)
   {
      // all strings should be constants anyhow - MarkMember(i->first);
      HX_MARK_OBJECT(i->second.mPtr);
      i->second->MarkStatics();
   }
}



bool __instanceof(const Dynamic &inValue, const Dynamic &inType)
{
	if (inType==hxObject__mClass) return true;
	if (inValue==null()) return false;
	Class c = inType;
	if (c==null()) return false;
	return c->CanCast(inValue.GetPtr());
}


int __int__(double x)
{
	if (x < -0x7fffffff || x>0x80000000 )
	{
		__int64 big_int = (__int64)x;
		return big_int & 0xffffffff;
	}
	else
		return (int)x;
}


// -------- Enums ---------------------------------

Dynamic hxEnumBase_obj::__Create(DynamicArray inArgs) { return new hxEnumBase_obj; }
Dynamic hxEnumBase_obj::__CreateEmpty() { return new hxEnumBase_obj; }


int hxEnumBase_obj::__FindIndex(String inName) { return -1; }
int hxEnumBase_obj::__FindArgCount(String inName) { return -1; }
Dynamic hxEnumBase_obj::__Field(const String &inString) { return null(); }

static Class hxEnumBase_obj__mClass;
Class &hxEnumBase_obj::__SGetClass() { return hxEnumBase_obj__mClass; }

//void hxEnumBase_obj::__GetFields(Array<String> &outFields) { }

void hxEnumBase_obj::__boot()
{
   Static(hxEnumBase_obj__mClass) = RegisterClass(STRING(L"__EnumBase",10) ,TCanCast<hxEnumBase_obj>,
                       sNone,sNone,
                       &__CreateEmpty, &__Create, 0 );
}

void hxEnumBase_obj::__Mark()
{
   MarkMember(tag);
   MarkMember(mArgs);
}

String hxEnumBase_obj::toString() { return tag; }


// -------- Int32 ---------------------------------------

namespace cpp
{

STATIC_DEFINE_DYNAMIC_FUNC2(CppInt32___obj,make,return )

}

// -------- Math ---------------------------------------




bool Math_obj::isNaN(double inX) { return inX!=inX; }
bool Math_obj::isFinite(double inX)
  { return !isNaN(inX) && inX!=NEGATIVE_INFINITY && inX!=POSITIVE_INFINITY; }

double Math_obj::NaN = std::numeric_limits<double>::quiet_NaN();
double Math_obj::NEGATIVE_INFINITY = -std::numeric_limits<double>::infinity();
double Math_obj::PI = 3.1415926535897932385;
double Math_obj::POSITIVE_INFINITY = std::numeric_limits<double>::infinity();

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,floor,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,ceil,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,round,return);
STATIC_DEFINE_DYNAMIC_FUNC0(Math_obj,random,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,sqrt,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,cos,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,sin,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,tan,return);
STATIC_DEFINE_DYNAMIC_FUNC2(Math_obj,atan2,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,abs,return);
STATIC_DEFINE_DYNAMIC_FUNC2(Math_obj,pow,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,log,return);
STATIC_DEFINE_DYNAMIC_FUNC2(Math_obj,min,return);
STATIC_DEFINE_DYNAMIC_FUNC2(Math_obj,max,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,atan,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,asin,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,acos,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,exp,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,isNaN,return);
STATIC_DEFINE_DYNAMIC_FUNC1(Math_obj,isFinite,return);

Dynamic Math_obj::__Field(const String &inString)
{
   if (inString==STR(L"floor")) return floor_dyn();
   if (inString==STR(L"ceil")) return ceil_dyn();
   if (inString==STR(L"round")) return round_dyn();
   if (inString==STR(L"random")) return random_dyn();
   if (inString==STR(L"sqrt")) return sqrt_dyn();
   if (inString==STR(L"cos")) return cos_dyn();
   if (inString==STR(L"sin")) return sin_dyn();
   if (inString==STR(L"tan")) return tan_dyn();
   if (inString==STR(L"atan2")) return atan2_dyn();
   if (inString==STR(L"abs")) return abs_dyn();
   if (inString==STR(L"pow")) return pow_dyn();
   if (inString==STR(L"log")) return log_dyn();
   if (inString==STR(L"min")) return min_dyn();
   if (inString==STR(L"max")) return max_dyn();
   if (inString==STR(L"atan")) return max_dyn();
   if (inString==STR(L"acos")) return max_dyn();
   if (inString==STR(L"asin")) return max_dyn();
   if (inString==STR(L"exp")) return max_dyn();
   if (inString==STR(L"isNaN")) return isNaN_dyn();
   if (inString==STR(L"isFinite")) return isFinite_dyn();
   return null();
}

Dynamic Math_obj::__IField(int inFieldID)
{
   return __Field( __hxcpp_field_from_id(inFieldID) );
}

void Math_obj::__GetFields(Array<String> &outFields) { }

static String sMathFields[] = {
   STRING(L"floor",5),
   STRING(L"ceil",4),
   STRING(L"round",5),
   STRING(L"random",6),
   STRING(L"sqrt",4),
   STRING(L"cos",3),
   STRING(L"sin",3),
   STRING(L"tan",3),
   STRING(L"atan2",5),
   STRING(L"abs",3),
   STRING(L"pow",3),
   STRING(L"atan",4),
   STRING(L"acos",4),
   STRING(L"asin",4),
   STRING(L"exp",3),
   STRING(L"isFinite",8),
   String(null()) };


Dynamic Math_obj::__SetField(const String &inString,const Dynamic &inValue) { return null(); }

Dynamic Math_obj::__CreateEmpty() { return new Math_obj; }

Class Math_obj::__mClass;

/*
Class &Math_obj::__SGetClass() { return __mClass; }
Class Math_obj::__GetClass() const { return __mClass; }
bool Math_obj::__Is(hxObject *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; } \
*/


void Math_obj::__boot()
{
   Static(Math_obj::__mClass) = RegisterClass(STRING(L"Math",4),TCanCast<Math_obj>,sMathFields,sNone, &__CreateEmpty,0 , 0 );
}

double hxDoubleMod(double inLHS,double inRHS)
{
   if (inRHS==0) return 0;
   double divs = inLHS/inRHS;
   int lots = divs<0 ? (int)(-divs) : (int)divs;
   return inLHS - lots*inRHS;
}

// -------- Resources ---------------------------------------


typedef std::map<String,hxResource> ResourceSet;
static ResourceSet sgResources;

void RegisterResources(hxResource *inResources)
{
   while(inResources->mData)
   {
      sgResources[inResources->mName] = *inResources;
      inResources++;
   }
}

Array<String> __hxcpp_resource_names()
{
   Array<String> result(0,0);
   for(ResourceSet::iterator i=sgResources.begin(); i!=sgResources.end();++i)
      result->push( i->first );
   return result;
}

String __hxcpp_resource_string(String inName)
{
   ResourceSet::iterator i=sgResources.find(inName);
   if (i==sgResources.end())
      return null();
   return String((const char *) i->second.mData, i->second.mDataLength );
}

Array<unsigned char> __hxcpp_resource_bytes(String inName)
{
   ResourceSet::iterator i=sgResources.find(inName);
   if (i==sgResources.end())
      return null();
   int len = i->second.mDataLength;
   Array<unsigned char> result( len, 0);
   memcpy( result->GetBase() , i->second.mData, len );
   return result;
}




// -------- Externs ---------------------------------------


void __trace(Dynamic inObj, Dynamic inData)
{
   printf( "%S:%d: %S\n",
               inData->__Field(L"fileName")->__ToString().__s,
               inData->__Field(L"lineNumber")->__ToInt(),
               inObj.GetPtr() ? inObj->toString().__s : L"null" );
}

static double t0 = 0;
double  __time_stamp()
{
#ifdef _WIN32
   static __int64 t0=0;
   static double period=0;
   __int64 now;

   if (QueryPerformanceCounter((LARGE_INTEGER*)&now))
   {
      if (t0==0)
      {
         t0 = now;
         __int64 freq;
         QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
         if (freq!=0)
            period = 1.0/freq;
      }
      if (period!=0)
         return (now-t0)*period;
   }

   return (double)clock() / ( (double)CLOCKS_PER_SEC);
#else
   struct timeval tv;
   if( gettimeofday(&tv,NULL) )
      throw Dynamic("Could not get time");
   double t =  ( tv.tv_sec + ((double)tv.tv_usec) / 1000000.0 );
   if (t0==0) t0 = t;
   return t-t0;
#endif
}


bool __hxcpp_same_closure(Dynamic &inF1,Dynamic &inF2)
{
   hxObject *p1 = inF1.GetPtr();
   hxObject *p2 = inF2.GetPtr();
   if (p1==0 || p2==0)
      return false;
   if (p1->__GetHandle() != p2->__GetHandle())
      return false;
   return typeid(*p1) == typeid(*p2);
}

#ifdef __APPLE__
   extern "C" {
   extern int *_NSGetArgc(void);
   extern char ***_NSGetArgv(void);
   }
#endif
Array<String> __get_args()
{
   Array<String> result(0,0);

   #ifdef _WIN32
   LPTSTR str =  GetCommandLine();
   bool skip_first = true;
   while(*str != '\0')
   {
      bool in_quote = false;
      LPTSTR end = str;
      String arg;
      while(*end!=0)
      {
         if (*end=='\0') break;
         if (!in_quote && *end==' ') break;
         if (*end=='"')
            in_quote = !in_quote;
         else
            arg += String::fromCharCode(*end);
         ++end;
      }

      if (!skip_first)
         result.Add( arg );
         skip_first = false;

      while(*end==' ') end++;
      str = end;
   }
   #else
   #ifdef __APPLE__

   int argc = *_NSGetArgc();
   char **argv = *_NSGetArgv();
   for(int i=1;i<argc;i++)
      result->push( String(argv[i],strlen(argv[i])) );


   #else // linux

   char buf[80];
   sprintf(buf, "/proc/%d/cmdline", getpid());
   FILE *cmd = fopen(buf,"rb");
   bool real_arg = 0;
   if (cmd)
   {
      String arg;
      buf[1] = '\0';
      while (fread(buf, 1, 1, cmd))
      {
         if ((unsigned char)buf[0]<32) // line terminator
         {
            if (real_arg)
               result->push(arg);
            real_arg = true;
            arg = String();
         }
         else
            arg+=String(buf,1);
      }
      fclose(cmd);
   }
   #endif

   #endif
   return result;
}

const String &__hxcpp_field_from_id( int f )
{
   if (!sgFieldToString)
      return sgNullString;

   return (*sgFieldToString)[f];
}


int  __hxcpp_field_to_id( const char *inFieldName )
{
   if (!sgFieldToString)
   {
      sgFieldToString = new FieldToString;

      #ifndef INTERNAL_GC
      __RegisterStatic(sgFieldToString,sizeof(*sgFieldToString));
      (*sgFieldToString) = Array<String>(0,100);
      #endif
      sgStringToField = new StringToField;
   }

   std::string f(inFieldName);
   StringToField::iterator i = sgStringToField->find(f);
   if (i!=sgStringToField->end())
      return i->second;

   #ifdef INTERNAL_GC
   int result = sgFieldToString->size();
   (*sgStringToField)[f] = result;
   String str(inFieldName,strlen(inFieldName));
   String cstr((wchar_t *)hxInternalCreateConstBuffer(str.__s,(str.length+1) * sizeof(wchar_t)), str.length );
   sgFieldToString->push_back( cstr );
   #else
   int result = (*sgFieldToString)->size();
   (*sgStringToField)[f] = result;
   String str(inFieldName,strlen(inFieldName));
   (*sgFieldToString)->push(str);
   #endif
   return result;
}

void __hxcpp_print(Dynamic &inV)
{
   printf("%S",inV->toString().c_str());
}

void __hxcpp_println(Dynamic &inV)
{
   printf("%S\n",inV->toString().c_str());
}




