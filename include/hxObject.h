#ifndef HX_HXOBJECT_H
#define HX_HXOBJECT_H

#pragma once

#include <wchar.h>
#include <string.h>

#ifdef HX_LINUX
#include <unistd.h>
#include <stdint.h>
#endif

#include <hxMacros.h>
#include <hxGCInternal.h>

// sort...
#include <algorithm>

#pragma warning(disable:4251)
#pragma warning(disable:4800)

#ifdef HX_WINDOWS
// MSVC hacks
#define SHARED __declspec(dllexport)
#elif defined(HX_LINUX) || defined (HX_MACOS)
#define SHARED __attribute__ ((visibility("default")))
#else
#define SHARED
#endif

#ifdef assert
#undef assert
#endif



#if defined(_MSC_VER) && _MSC_VER < 1201
#error MSVC 7.1 does not support template specialization and is not sopported by HXCPP
#endif



// TODO: Construct array-dynamic from foreign array

// Basic mapping from haxe -> c++

typedef int Int;
typedef double Float;
typedef bool Bool;

namespace haxe { namespace io { typedef unsigned char Unsigned_char__; } }

// --- Constants -------------------------------------------------------

enum
{
   vtUnknown = -1,
   vtInt = 0xff,
   vtNull = 0,
   vtFloat = 1,
   vtBool = 2,
   vtString = 3,
   vtObject = 4,
   vtArray = 5,
   vtFunction = 6,
   vtEnum,
   vtClass,
   vtAbstractBase = 0x100,
};


// Application should call this first
void __boot_all();


// --- Forward decalarations --------------------------------------------


template<typename ELEM_> class Array_obj;
template<typename ELEM_> class Array;
// Starting a class with a lowername letter ensures is does not clash with
//  any haxe classes
template<typename O> class hxObjectPtr;
class hxFieldRef;
namespace cpp { class CppInt32__; }

class Dynamic;
class Class_obj;
typedef hxObjectPtr<Class_obj> Class;
typedef Array<Dynamic> DynamicArray;
class String;
class hxObject;

// --- Exteral constants, used inline
#define INVALID_CAST          Dynamic(STRING(L"Invalid Cast",11))
#define INDEX_OUT_OF_BOUNDS   Dynamic(STRING(L"Index Out of Bounds",18))
#define INVALID_CONSTRUCTOR   Dynamic(STRING(L"Invalid constructor",18))
#define INVALID_OBJECT        Dynamic(STRING(L"Invalid object",13))
#define INVALID_ARG_COUNT     Dynamic(STRING(L"Invalid Arg Count",16))
#define NULL_FUNCTION_POINTER Dynamic(STRING(L"Null Function Pointer",20))
#define LOGIC_ERROR           Dynamic(STRING(L"Logic Error",11))

//extern Dynamic  __InvalidConstructor;
//extern Dynamic  __InvalidArgCount;



// This is used internally in hxcpp
wchar_t *hxNewString(int inLen);

// Internal for arrays
void *hxGCRealloc(void *inData,int inSize);
void hxGCInit();
void hxMarkClassStatics();
void hxLibMark();
void hxGCMark(class hxObject *inPtr);

// This will be GC'ed
 void *hxNewGCBytes(void *inData,int inSize);
// This wont be GC'ed
 void *hxNewGCPrivate(void *inData,int inSize);


void  hxGCAddFinalizer( hxObject *, finalizer f );

inline int hxUShr(int inData,int inShift)
{
   return ((unsigned int)inData) >> inShift;
}


 double hxDoubleMod(double inLHS,double inRHS);

template<typename TL,typename TR>
double hxMod(TL inLHS,TR inRHS) { return hxDoubleMod(inLHS,inRHS); }
#if !defined(_MSC_VER) || _MSC_VER > 1399
inline int hxMod(int inLHS,int inRHS) { return inLHS % inRHS; }
#endif


// --- null value  ---------------------------------------------------------
//
// This is used by external operatator and return statments - Most will
//  use operator overloading to convert to the null pointer

class null
{
   public:
     inline null(){ } 

     template<typename T> explicit inline null(const hxObjectPtr<T> &){ } 
     template<typename T> explicit inline null(const String &){ } 
     explicit inline null(double){ } 
     explicit inline null(int){ } 
     explicit inline null(bool){ } 

     operator char * () const { return 0; }
     operator wchar_t * () const { return 0; }
     operator bool () const { return false; }
     operator int () const { return 0; }
     operator double () const { return 0; }
     operator unsigned char () const { return 0; }

     bool operator == (const null &inRHS) const { return true; }
     bool operator != (const null &inRHS) const { return false; }

     bool operator == (int inRHS) const { return false; }
     bool operator != (int inRHS) const { return true; }
     bool operator == (double inRHS) const { return false; }
     bool operator != (double inRHS) const { return true; }
     bool operator == (bool inRHS) const { return false; }
     bool operator != (bool inRHS) const { return true; }
};

typedef null Void;

struct BreakFromFunction { };



// --- hxObject ------------------------------------------------------------
//
// Base for all hxcpp objects.
// This contains the virtual functions required by the core to provide
//  a generic interface to the specific classes.
//
// Hxcpp classes inherit from this.  If the derived class (or any of its children)
//  may in turn implement an interface (ie, multiple inheritance) then the class
//  inherit virtually from this base.
//
class  hxObject
{
public:
   // These allocate the function using the garbage-colleced malloc
   void *operator new( size_t inSize, bool inContainer=true );
   void operator delete( void *, bool ) { }

   static void __boot();

   virtual void *__root();
   virtual bool __Is(hxObject *inClass) const { return true; }
   // helpers...
   bool __Is(Dynamic inClass ) const;
   bool __IsArray() const { return __GetType()==vtArray; }

   virtual int __GetType() const { return vtUnknown; }
   virtual void *__GetHandle() const { return 0; }


   virtual hxFieldRef __FieldRef(const String &inString);

   virtual String __ToString() const;

   virtual int __ToInt() const { return 0; }
   virtual double __ToDouble() const { return 0.0; }
   virtual char * __CStr() const;
   virtual String toString();
   virtual bool __HasField(const String &inString);
   virtual Dynamic __Field(const String &inString);
   virtual Dynamic __IField(int inFieldID);
   virtual Dynamic __SetField(const String &inField,const Dynamic &inValue);
   virtual void  __SetThis(Dynamic inThis);
   virtual Dynamic __Run(const Array<Dynamic> &inArgs);
   virtual void __GetFields(Array<String> &outFields);
   virtual Class __GetClass() const;

   virtual int __Compare(const hxObject *inRHS) const;
   virtual DynamicArray __EnumParams();
   virtual String __Tag() const;
   virtual int __Index() const;

   virtual int __length() const { return 0; }
   virtual Dynamic __GetItem(int inIndex) const;
   virtual void __SetItem(int inIndex,Dynamic inValue);
   virtual void __SetSize(int inLen) { }

   typedef const Dynamic &D;
   virtual Dynamic __run();
   virtual Dynamic __run(D a);
   virtual Dynamic __run(D a,D b);
   virtual Dynamic __run(D a,D b,D c);
   virtual Dynamic __run(D a,D b,D c,D d);
   virtual Dynamic __run(D a,D b,D c,D d,D e);
   virtual Dynamic __run(D a,D b,D c,D d,D e,D f);
   virtual Dynamic __run(D a,D b,D c,D d,D e,D f,D g);
   virtual Dynamic __run(D a,D b,D c,D d,D e,D f,D g,D h);
   virtual Dynamic __run(D a,D b,D c,D d,D e,D f,D g,D h,D i);
   virtual Dynamic __run(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j);
   virtual Dynamic __run(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j,D k);

   virtual int __ArgCount() const { return -1; }
   virtual void __Mark() { }


   static Class &__SGetClass();
};


// --- hxObjectPtr ---------------------------------------------------------------
//
// This class simply provides syntax so that pointers can be written as objects,
//  and overloaded operators can be used

template<typename OBJ_>
class hxObjectPtr
{
public:
   typedef OBJ_ Obj;
   typedef OBJ_ *Ptr;

   hxObjectPtr() : mPtr(0) { }
   hxObjectPtr(OBJ_ *inObj) : mPtr(inObj) { }
   hxObjectPtr(const null &inNull) : mPtr(0) { }

   template<typename SOURCE_>
   hxObjectPtr(const hxObjectPtr<SOURCE_> &inObjectPtr)
      { mPtr = dynamic_cast<OBJ_ *>(inObjectPtr.GetPtr()); }

   hxObjectPtr(const hxObjectPtr<OBJ_> &inOther) : mPtr( inOther.GetPtr() ) {  }

   hxObjectPtr &operator=(const null &inNull) { mPtr = 0; return *this; }
   hxObjectPtr &operator=(Ptr inRHS) { mPtr = inRHS; return *this; }
   hxObjectPtr &operator=(const hxObjectPtr &inRHS) { mPtr = inRHS.mPtr; return *this; }

   inline OBJ_ *GetPtr() const { return mPtr; }
   inline OBJ_ *operator->() { return mPtr; }
   inline const OBJ_ *operator->() const { return mPtr; }

   bool operator==(const hxObjectPtr &inRHS) const { return mPtr==inRHS.mPtr; }
   bool operator!=(const hxObjectPtr &inRHS) const { return mPtr!=inRHS.mPtr; }
   bool operator==(const null &inRHS) const { return mPtr==0; }
   bool operator!=(const null &inRHS) const { return mPtr!=0; }
   //explicit operator bool() const { return mPtr!=0; }


   inline class hxFieldRef FieldRef(const String &inString);
   static Class &__SGetClass() { return OBJ_::__SGetClass(); }

   OBJ_ *mPtr;
};

template<typename T>
inline bool operator==(const null &inLHS,const hxObjectPtr<T> &inRHS) { return inRHS==inLHS; }

template<typename T>
inline bool operator!=(const null &inLHS,const hxObjectPtr<T> &inRHS) { return inRHS!=inLHS; }

// --- String --------------------------------------------------------
//
// Basic String type for hxcpp.
// It's based on garbage collection of the char *ptr.
// Note: this does not inherit from "hxObject", so in some ways it acts more
// like a standard "int" type than a mode generic class.

class String
{
public:
  // These allocate the function using the garbage-colleced malloc
   void *operator new( size_t inSize );
   void operator delete( void * ) { }

   inline String() : length(0), __s(0) { }
   String(const wchar_t *inPtr);
   inline String(const wchar_t *inPtr,int inLen) : __s(inPtr), length(inLen) { }
   inline String(const String &inRHS) : __s(inRHS.__s), length(inRHS.length) { }
   String(const int &inRHS);
   String(const cpp::CppInt32__ &inRHS);
   String(const double &inRHS);
   String(const bool &inRHS);
   inline String(const null &inRHS) : __s(0), length(0) { }
   // Construct from utf8 string
    String(const char *inPtr,int inLen);

   template<typename T>
   inline String(const hxObjectPtr<T> &inRHS)
   {
      if (inRHS.GetPtr()) { String s = const_cast<hxObjectPtr<T> & >(inRHS)->toString(); __s = s.__s; length = s.length; }
      else { __s = 0; length = 0; }
   }
    String(const Dynamic &inRHS);

   inline String &operator=(const String &inRHS)
           { length = inRHS.length; __s = inRHS.__s; return *this; }

   String toString() { return *this; }

    String __URLEncode() const;
    String __URLDecode() const;

    String &dup();

    String toUpperCase() const;
    String toLowerCase() const;
    String charAt(int inPos) const;
    Dynamic charCodeAt(int inPos) const;
    int indexOf(const String &inValue, Dynamic inStart) const;
    int lastIndexOf(const String &inValue, Dynamic inStart) const;
    Array<String> split(const String &inDelimiter) const;
    String substr(int inPos,Dynamic inLen) const;

   inline const wchar_t *c_str() const { return __s; }
    char *__CStr() const;

   static  String fromCharCode(int inCode);

   inline bool operator==(const null &inRHS) const { return __s==0; }
   inline bool operator!=(const null &inRHS) const { return __s!=0; }

   inline int getChar( int index ) { return __s[index]; }


   inline int compare(const String &inRHS) const
   {
      const wchar_t *r = inRHS.__s;
      if (__s == r) return inRHS.length-length;
      if (__s==0) return -1;
      if (r==0) return 1;
      return wcscmp(__s,r);
   }


   String &operator+=(String inRHS);
   String operator+(String inRHS) const;
   String operator+(const int &inRHS) const { return *this + String(inRHS); }
   String operator+(const bool &inRHS) const { return *this + String(inRHS); }
   String operator+(const double &inRHS) const { return *this + String(inRHS); }
   String operator+(const null &inRHS) const{ return *this + String(L"null",4); } 
   String operator+(const wchar_t *inRHS) const{ return *this + String(inRHS); } 
   String operator+(const cpp::CppInt32__ &inRHS) const{ return *this + String(inRHS); } 
   template<typename T>
   inline String operator+(const hxObjectPtr<T> &inRHS) const
      { return *this + (inRHS.GetPtr() ? const_cast<hxObjectPtr<T>&>(inRHS)->toString() : String(L"null",4) ); }

   inline bool operator==(const String &inRHS) const
                     { return length==inRHS.length && compare(inRHS)==0; }
   inline bool operator!=(const String &inRHS) const
                     { return length != inRHS.length || compare(inRHS)!=0; }
   inline bool operator<(const String &inRHS) const { return compare(inRHS)<0; }
   inline bool operator<=(const String &inRHS) const { return compare(inRHS)<=0; }
   inline bool operator>(const String &inRHS) const { return compare(inRHS)>0; }
   inline bool operator>=(const String &inRHS) const { return compare(inRHS)>=0; }

   inline int cca(int inPos) const { return __s[inPos]; }

   int length;
   const wchar_t *__s;


   static  Dynamic fromCharCode_dyn();

    Dynamic charAt_dyn();
    Dynamic charCodeAt_dyn();
    Dynamic indexOf_dyn();
    Dynamic lastIndexOf_dyn();
    Dynamic split_dyn();
    Dynamic substr_dyn();
    Dynamic toLowerCase_dyn();
    Dynamic toString_dyn();
    Dynamic toUpperCase_dyn();

	// This is used by the string-wrapped-as-dynamic class
    Dynamic __Field(const String &inString);
};



// --- Dynamic ---------------------------------------------------------------
//
// The Dynamic class views all classes through the hxObject interface, and
//  provides generic access to its pointer.
// It uses dynamic_cast to provide strongly-typed access to the real class.

class Dynamic : public hxObjectPtr<hxObject>
{
   typedef  hxObjectPtr<hxObject> super;

public:

   Dynamic() {};
   Dynamic(int inVal);
   Dynamic(const cpp::CppInt32__ &inVal);
   Dynamic(bool inVal);
   Dynamic(double inVal);
   Dynamic(hxObject *inObj) : super(inObj) { }
   Dynamic(const String &inString);
   Dynamic(const null &inNull) : super(0) { }
   Dynamic(const Dynamic &inRHS) : super(inRHS.GetPtr()) { }
   Dynamic(const wchar_t *inStr);

    void Set(bool inVal);
    void Set(int inVal);
    void Set(double inVal);

   operator double () const { return mPtr ? mPtr->__ToDouble() : 0.0; }
   operator int () const { return mPtr ? mPtr->__ToInt() : 0; }
   operator unsigned char () const { return mPtr ? mPtr->__ToInt() : 0; }
   operator bool() const { return mPtr && mPtr->__ToInt(); }
   bool operator !() const { return !mPtr || !mPtr->__ToInt(); }

   inline Dynamic operator[](int inIndex) const { return mPtr->__GetItem(inIndex); }
   inline Dynamic __get(int inIndex) const { return mPtr->__GetItem(inIndex); }

   template<typename SOURCE_>
   Dynamic(const hxObjectPtr<SOURCE_> &inObjectPtr) :
          hxObjectPtr<hxObject>(inObjectPtr.GetPtr()) { }

   Dynamic Default(const Dynamic &inDef) { return mPtr ? *this : inDef; }

   template<typename RETURN_>
   RETURN_ Cast() const { return RETURN_(*this); }

   template<typename CLASS_>
   bool IsClass() { return dynamic_cast<typename CLASS_::Ptr>(mPtr)!=0; }


   int Compare(const Dynamic &inRHS) const
   {
      if (mPtr==inRHS.mPtr) return 0;
      if (mPtr==0) return -1;
      if (inRHS.mPtr==0) return -1;
      return mPtr->__Compare(inRHS.mPtr);
   }

   bool operator==(const null &inRHS) const { return mPtr==0; }
   bool operator!=(const null &inRHS) const { return mPtr!=0; }

   DYNAMIC_COMPARE_OP_NOT_EQUAL

   bool operator == (const Dynamic &inRHS) const
   {
      if (mPtr==inRHS.mPtr) return true;
      if (!mPtr || !inRHS.mPtr) return false;
      return mPtr->__Compare(inRHS.mPtr)==0;
   }

   DYNAMIC_COMPARE_OP( == )
   DYNAMIC_COMPARE_OP_ALL( < )
   DYNAMIC_COMPARE_OP_ALL( <= )
   DYNAMIC_COMPARE_OP_ALL( >= )
   DYNAMIC_COMPARE_OP_ALL( >  )

   template<typename T_>
   bool operator==(const hxObjectPtr<T_> &inRHS) const { return mPtr == inRHS.GetPtr(); }
   template<typename T_>
   bool operator!=(const hxObjectPtr<T_> &inRHS) const { return mPtr != inRHS.GetPtr(); }


   // Operator + is different, since it must consider strings too...
    Dynamic operator+(const Dynamic &inRHS) const;
   inline String operator+(const String &s) const;
    Dynamic operator+(const int &i) const;
    Dynamic operator+(const double &d) const;

   double operator%(const Dynamic &inRHS) const;
   double operator-() const { return mPtr ? - mPtr->__ToDouble() : 0.0; }

   DYNAMIC_ARITH( - )
   DYNAMIC_ARITH( * )
   DYNAMIC_ARITH( / )

	 void CheckFPtr();

   // Hmm, ugly.
   typedef const Dynamic &D;
   inline Dynamic operator()() { CheckFPtr(); return mPtr->__run(); }
   inline Dynamic operator()(D a) { CheckFPtr(); return mPtr->__run(a); }
   inline Dynamic operator()(D a,D b) { CheckFPtr(); return mPtr->__run(a,b); }
   inline Dynamic operator()(D a,D b,D c) { CheckFPtr(); return mPtr->__run(a,b,c); }
   inline Dynamic operator()(D a,D b,D c,D d) { CheckFPtr(); return mPtr->__run(a,b,c,d); }
   inline Dynamic operator()(D a,D b,D c,D d,D e) { CheckFPtr(); return mPtr->__run(a,b,c,d,e); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f) { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g,D h)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g,h); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g,D h,D i)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g,h,i); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g,h,i,j); }
   inline Dynamic operator()(D a,D b,D c,D d,D e,D f,D g,D h,D i,D j,D k)
      { CheckFPtr(); return mPtr->__run(a,b,c,d,e,f,g,h,i,j,k); }

};





template<>
inline int Dynamic::Cast<int>() const { return mPtr ? mPtr->__ToInt() : 0; }
template<>
inline bool Dynamic::Cast<bool>() const { return mPtr ? mPtr->__ToInt() : 0; }
template<>
inline double Dynamic::Cast<double>() const { return mPtr ? mPtr->__ToDouble() : 0; }
template<>
inline String Dynamic::Cast<String>() const { return mPtr ? mPtr->toString() : String(null()); }


//
// Gets the class definition that relates to a specific type.
// Most classes have their own class data, by the standard types (non-classes)
//  use the template traits to get the class

 Class &GetIntClass();
 Class &GetFloatClass();
 Class &GetBoolClass();
 Class &GetVoidClass();
 Class &GetStringClass();

template<>
inline bool Dynamic::IsClass<int>() { return mPtr && mPtr->__GetClass()==GetIntClass(); }
template<>
inline bool Dynamic::IsClass<double>() { return mPtr && mPtr->__GetClass()==GetFloatClass(); }
template<>
inline bool Dynamic::IsClass<bool>() { return mPtr && mPtr->__GetClass()==GetBoolClass(); }
template<>
inline bool Dynamic::IsClass<null>() { return !mPtr; }
template<>
inline bool Dynamic::IsClass<String>() { return mPtr && mPtr->__GetClass()==GetStringClass(); }

// Converting to Array<Dynamic> will actuallt do a copy, so this is Ok...
// See below for specialization...


// --- hxClassOf --------------------------------------------------------------
//
// Gets the class definition that relates to a specific type.
// Most classes have their own class data, by the standard types (non-classes)
//  use the template traits to get the class


template<typename T> 
inline Class &hxClassOf() { typedef typename T::Obj Obj; return Obj::__SGetClass(); }

template<> 
inline Class &hxClassOf<int>() { return GetIntClass(); }

template<> 
inline Class &hxClassOf<double>() { return GetFloatClass(); }

template<> 
inline Class &hxClassOf<bool>() { return GetBoolClass(); }

template<> 
inline Class &hxClassOf<null>() { return GetVoidClass(); }

template<> 
inline Class &hxClassOf<String>() { return GetStringClass(); }






inline String Dynamic::operator+(const String &s) const { return Cast<String>() + s; }

COMPARE_DYNAMIC_OP_NOT_EQUAL

COMPARE_DYNAMIC_OP( == )
COMPARE_DYNAMIC_OP( < )
COMPARE_DYNAMIC_OP( <= )
COMPARE_DYNAMIC_OP( >= )
COMPARE_DYNAMIC_OP( >  )

inline bool operator == (bool inLHS,const null &inRHS)  { return false; }
inline bool operator != (bool inLHS,const null &inRHS)  { return true; }
inline bool operator == (double inLHS,const null &inRHS)  { return false; }
inline bool operator != (double inLHS,const null &inRHS)  { return true; }
inline bool operator == (int inLHS,const null &inRHS)  { return false; }
inline bool operator != (int inLHS,const null &inRHS)  { return true; }



ARITH_DYNAMIC( - )
ARITH_DYNAMIC( + )
ARITH_DYNAMIC( / )
ARITH_DYNAMIC( * )

 double operator%(const int &inLHS,const Dynamic &inRHS);
 double operator%(const double &inLHS,const Dynamic &inRHS);

// --- hxFieldRef ----------------------------------------------------------
//
//  This is used to provide syntaxe for setting fields by name.  This is because
//   the field can't be returned by reference, because it may not exist as a dynamic.
//
//  eg, consider class 'A' with variable 'x':
//  class A { int x; }
//
//  And you have a Dynamic pointing to it:
//  Dynamic d = new A;  Then you access x by name:
//  d->__Field("x") = 1;
//
//  __Field can't return a Dynamic & because x is a int, not Dynamic. So I use this class.
//  Note that this may change if I fix the generator to create __SetField("x",1) directly.

class hxFieldRef
{
public:
   explicit hxFieldRef(hxObject *inObj,const String &inName) : mObject(inObj), mName(inName)
   {
   }

   Dynamic operator=(const Dynamic &inRHS)
   {
      return mObject->__SetField(mName,inRHS);
   }
   inline operator Dynamic() const { return mObject->__Field(mName); }
   inline operator double() const { return mObject->__Field(mName); }
   inline operator int() const { return mObject->__Field(mName); }

   // post-increment
   inline double operator++(int)
   {
      double d = mObject->__Field(mName)->__ToDouble();
      mObject->__SetField(mName,d+1);
      return d;
   }
   // pre-increment
   inline double operator++()
   {
      double d = mObject->__Field(mName)->__ToDouble() + 1;
      mObject->__SetField(mName,d);
      return d;
   }
   // post-decrement
   inline double operator--(int)
   {
      double d = mObject->__Field(mName)->__ToDouble();
      mObject->__SetField(mName,d-1);
      return d;
   }
   // pre-decrement
   inline double operator--()
   {
      double d = mObject->__Field(mName)->__ToDouble() - 1;
      mObject->__SetField(mName,d);
      return d;
   }
   bool operator !() { return ! mObject->__Field(mName)->__ToInt(); }


   String  mName;
   hxObject *mObject;
};

template<typename T>
inline hxFieldRef hxObjectPtr<T>::FieldRef(const String &inString)
{
   return hxFieldRef(mPtr,inString);
}

// --- hxIndexRef ----------------------------------------------------------
//
// Like hxFieldRef, but for array[] syntax.  A bit easier because we know
// the array type.

template<typename T>
class hxIndexRef
{
   typedef typename T::Obj Obj;
   Obj *mObj;
   int mIdx;
public: 
   typedef typename Obj::__array_access Return;
   hxIndexRef(const T &inObj,int inIdx) : mObj(inObj.GetPtr()), mIdx(inIdx) { }

   inline operator Return() const { return mObj->__get(mIdx); }
   inline void operator=(Return inVal)  { mObj->__set(mIdx,inVal); }
};

template<typename T>
inline hxIndexRef<T> hxIndexRefNew(const T &inObj, int inIdx)
{
   return hxIndexRef<T>(inObj, inIdx);
}

// --- hxAnon_obj ----------------------------------------------
//
// The hxAnon_obj contains an arbitrary string map of fields.

class hxFieldMap;

 hxFieldMap *hxFieldMapCreate();
 bool hxFieldMapGet(hxFieldMap *inMap, const String &inName, Dynamic &outValue);
 bool hxFieldMapGet(hxFieldMap *inMap, int inID, Dynamic &outValue);
 void hxFieldMapSet(hxFieldMap *inMap, const String &inName, const Dynamic &inValue);
 void hxFieldMapAppendFields(hxFieldMap *inMap,Array<String> &outFields);
 void hxFieldMapMark(hxFieldMap *inMap);


class  hxAnon_obj : public hxObject
{
   typedef hxAnon_obj OBJ_;
   typedef hxObjectPtr<hxAnon_obj> hxAnon;
   typedef hxObject super;

   hxFieldMap *mFields;

public:
   hxAnon_obj();

   static hxAnon Create() { return hxAnon(new hxAnon_obj); }
   static hxAnon Create(const Dynamic &inSrc) { return hxAnon(new hxAnon_obj); }

   static Dynamic __CreateEmpty() { return hxAnon(new hxAnon_obj); }
   static Dynamic __Create(DynamicArray inArgs);
   static void __boot();
   Dynamic __Field(const String &inString);
   bool __HasField(const String &inString);
   Dynamic __SetField(const String &inString,const Dynamic &inValue);
   virtual void __GetFields(Array<String> &outFields);

   static void Destroy(hxObject * inObj);

   virtual int __GetType() const { return vtObject; }

   hxAnon_obj *Add(const String &inName,const Dynamic &inValue);
	void __Mark();

   String __ToString() const;
   String toString();

   static hxObjectPtr<Class_obj> __mClass; \
   static hxObjectPtr<Class_obj> &__SGetClass() { return __mClass; }
   bool __Is(hxObject *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; }
   hxObjectPtr<Class_obj > __GetClass() const { return __mClass; }

   bool __Remove(String inKey);
};


typedef hxObjectPtr<hxAnon_obj> hxAnon;


 bool __hx_anon_remove(hxAnon inObj,String inKey);



// --- hxBoxed ------------------------------------------------------
//
// Provides an "Object" of given type.  For types that are not actually objects,
//  Dynamic is used.

template<typename T> struct hxBoxed { typedef T type; };
template<> struct hxBoxed<int> { typedef Dynamic type; };
template<> struct hxBoxed<double> { typedef Dynamic type; };
template<> struct hxBoxed<bool> { typedef Dynamic type; };
template<> struct hxBoxed<String> { typedef Dynamic type; };


// --- ArrayIterator -------------------------------------------
//
// An object that conforms to the standard iterator interface for arrays

class  ArrayIterator : public hxObject
{
public:
   ArrayIterator(Dynamic inArray) : mArray(inArray), mIdx(0) { }

   bool   hasNext( ) { return mIdx < mArray->__length(); }
   Dynamic hasNext_dyn( );

   Dynamic next() { return mArray->__GetItem(mIdx++); }
   Dynamic next_dyn( );

   Dynamic __Field(const String &inString);

   int     mIdx;
   Dynamic mArray;
};

// --- hxArrayBase ----------------------------------------------------
//
// Base class that treats array contents as a slab of bytes.
// The derived "Array_obj" adds strong typing to the "[]" operator

class  hxArrayBase : public hxObject
{
public:
   hxArrayBase(int inSize,int inReserve,int inElementSize,bool inAtomic);

   static void __boot();

   typedef hxObject super;

   Dynamic __SetField(const String &inString,const Dynamic &inValue) { return null(); }

   static Class __mClass;
   static Class &__SGetClass() { return __mClass; }
   Class __GetClass() const { return __mClass; }
   String toString();
   String __ToString() const;

   int __GetType() const { return vtArray; }

   inline size_t size() const { return length; }
   inline int __length() const { return (int)length; }
   virtual String ItemString(int inI)  = 0;

   char * __CStr() const { return mBase; }
   inline const char *GetBase() const { return mBase; }
   inline char *GetBase() { return mBase; }

   virtual int GetElementSize() const = 0;

   virtual void __SetSize(int inLen);

   // Dynamic interface
   Dynamic __Field(const String &inString);
   virtual Dynamic __concat(const Dynamic &a0) = 0;
   virtual Dynamic __copy() = 0;
   virtual Dynamic __insert(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual Dynamic __iterator() = 0;
   virtual Dynamic __join(const Dynamic &a0) = 0;
   virtual Dynamic __pop() = 0;
   virtual Dynamic __push(const Dynamic &a0) = 0;
   virtual Dynamic __remove(const Dynamic &a0) = 0;
   virtual Dynamic __reverse() = 0;
   virtual Dynamic __shift() = 0;
   virtual Dynamic __slice(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual Dynamic __splice(const Dynamic &a0,const Dynamic &a1) =0;
   virtual Dynamic __sort(const Dynamic &a0) = 0;
   virtual Dynamic __toString() = 0;
   virtual Dynamic __unshift(const Dynamic &a0) = 0;


   Dynamic concat_dyn();
   Dynamic copy_dyn();
   Dynamic insert_dyn();
   Dynamic iterator_dyn();
   Dynamic join_dyn();
   Dynamic pop_dyn();
   Dynamic push_dyn();
   Dynamic remove_dyn();
   Dynamic reverse_dyn();
   Dynamic shift_dyn();
   Dynamic slice_dyn();
   Dynamic splice_dyn();
   Dynamic sort_dyn();
   Dynamic toString_dyn();
   Dynamic unshift_dyn();



   void EnsureSize(int inLen) const;

   void RemoveElement(int inIndex);


   void Insert(int inPos);

   void Splice(hxArrayBase *outResult,int inPos,int inLen);

   void Slice(hxArrayBase *outResult,int inPos,int inEnd);

   void Concat(hxArrayBase *outResult,const char *inEnd, int inLen);


   void reserve(int inN);


   String join(String inSeparator);

   Dynamic iterator() { return new ArrayIterator(this); }

   virtual bool AllocAtomic() const { return false; }


   mutable int length;
protected:
   mutable int mAlloc;
   mutable char  *mBase;
};

// --- Array_obj ------------------------------------------------------------------
//
// The Array_obj specialises the ArrayBase, adding typing where required


// This is to determine is we need to include our slab of bytes in garbage collection
template<typename T>
inline bool TypeContainsPointers(T *) { return true; }
template<> inline bool TypeContainsPointers(bool *) { return false; }
template<> inline bool TypeContainsPointers(int *) { return false; }
template<> inline bool TypeContainsPointers(double *) { return false; }
template<> inline bool TypeContainsPointers(unsigned char *) { return false; }

template<typename TYPE> inline bool ContainsPointers()
{
   return TypeContainsPointers( (TYPE *)0 );
}

// For returning "null" when out of bounds ...
template<typename TYPE>
inline TYPE *hxNewNull() { Dynamic d; return (TYPE *)hxNewGCBytes(&d,sizeof(d)); }
template<> inline int *hxNewNull<int>() { int i=0; return (int *)hxNewGCPrivate(&i,sizeof(i)); }
template<> inline bool *hxNewNull<bool>() { bool b=0; return (bool *)hxNewGCPrivate(&b,sizeof(b)); }
template<> inline double *hxNewNull<double>() { double d=0; return (double *)hxNewGCPrivate(&d,sizeof(d)); }
template<> inline unsigned char *hxNewNull<unsigned char>() { unsigned char u=0; return (unsigned char *)hxNewGCPrivate(&u,sizeof(u)); }


#ifdef HX_WINDOWS
typedef void (*ThreadFunc)(void *inUserData);
#else
typedef void *(*ThreadFunc)(void *inUserData);
#endif

 void hxStartThread(ThreadFunc inFunc,void *inUserData);



template<typename ELEM_>
class Array_obj : public hxArrayBase
{
   typedef ELEM_ Elem;
   typedef hxObjectPtr< Array_obj<ELEM_> > ObjPtr;
   typedef typename hxBoxed<ELEM_>::type NullType;


public:
   Array_obj(int inSize,int inReserve) :
        hxArrayBase(inSize,inReserve,sizeof(ELEM_),!ContainsPointers<ELEM_>()) { }


   // Defined later so we can use "Array"
   static Array<ELEM_> __new(int inSize=0,int inReserve=0);

   virtual bool AllocAtomic() const { return !ContainsPointers<ELEM_>(); }


   virtual Dynamic __GetItem(int inIndex) const { return __get(inIndex); }
   virtual void __SetItem(int inIndex,Dynamic inValue) { Item(inIndex) = inValue; }

   inline ELEM_ *Pointer() { return (ELEM_ *)mBase; }

   inline ELEM_ &Item(int inIndex)
   {
      if (inIndex>=(int)length) EnsureSize(inIndex+1);
      else if (inIndex<0) { return * hxNewNull<ELEM_>(); }
      return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_));
   }
   inline ELEM_ __get(int inIndex) const
   {
      if (inIndex>=(int)length || inIndex<0) return null();
      return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_));
   }

   // Does not check for size valid - use quth care
   inline ELEM_ &QuickItem(int inIndex) { return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_)); }


   void __Mark()
   {
      if (ContainsPointers<ELEM_>())
      {
         ELEM_ *ptr = (ELEM_ *)mBase;
         for(int i=0;i<length;i++)
            MarkMember(ptr[i]);
      }
      HX_MARK_STRING(mBase, ELEM_ *);
   }

   int GetElementSize() const { return sizeof(ELEM_); }

   String ItemString(int inI) { return __get(inI); }

   Array_obj<ELEM_> *Add(const ELEM_ &inItem) { push(inItem); return this; }


   // Haxe API
   inline int push( const ELEM_ &inVal )
   {
      int l = length;
      EnsureSize((int)l+1);
      * (ELEM_ *)(mBase + l*sizeof(ELEM_)) = inVal;
      return length;
   }
   inline NullType pop( )
   {
      if (!length) return null();
      ELEM_ result = __get((int)length-1);
      __SetSize((int)length-1);
      return result;
   }

   ObjPtr concat( ObjPtr inTail )
   {
      Array_obj *result = new Array_obj(inTail->__length()+(int)length,0);
      hxArrayBase::Concat(result,inTail->GetBase(),inTail->__length());
      return result;
   }

   ObjPtr copy( )
   {
      Array_obj *result = new Array_obj((int)length,0);
      memcpy(result->GetBase(),GetBase(),length*sizeof(ELEM_));
      return result;
   }

   int Find(ELEM_ inValue)
   {
      ELEM_ *e = (ELEM_ *)mBase;
      for(int i=0;i<length;i++)
         if (e[i]==inValue)
            return i;
      return -1;
   }


   bool remove(ELEM_ inValue)
   {
      ELEM_ *e = (ELEM_ *)mBase;
      for(int i=0;i<length;i++)
      {
         if (e[i]==inValue)
         {
            RemoveElement((int)i);
            return true;
         }
      }
      return false;
   }

   NullType shift()
   {
      if (length==0) return null();
      ELEM_ result = __get(0);
      RemoveElement(0);
      return result;
   }

   // Copies the range of the array starting at pos up to, but not including, end.
   // Both pos and end can be negative to count from the end: -1 is the last item in the array.
   ObjPtr slice(int inPos, Dynamic end = null())
   {
      int e = end==null() ? length : end->__ToInt();
      Array_obj *result = new Array_obj(0,0);
      hxArrayBase::Slice(result,inPos,(int)e);
      return result;
   }

   ObjPtr splice(int inPos, int len)
   {
      Array_obj * result = new Array_obj(len,0);
      hxArrayBase::Splice(result,inPos,len);
      return result;
   }

   void insert(int inPos, ELEM_ inValue)
   {
      hxArrayBase::Insert(inPos);
      Item(inPos) = inValue;
   }

   void unshift(ELEM_ inValue)
   {
      insert(0,inValue);
   }

   void reverse()
   {
      int half = length/2;
      ELEM_ *e = (ELEM_ *)mBase;
      for(int i=0;i<half;i++)
      {
         ELEM_ tmp = e[length-i-1];
         e[length-i-1] = e[i];
         e[i] = tmp;
      }
   }


   struct Sorter
   {
      Sorter(Dynamic inFunc) : mFunc(inFunc) { }

      bool operator()(const ELEM_ &inA, const ELEM_ &inB)
      {
         return mFunc( Dynamic(inA), Dynamic(inB))->__ToInt() < 0;
      }

      Dynamic mFunc;
   };

   void sort(Dynamic inSorter)
   {
      ELEM_ *e = (ELEM_ *)mBase;
      std::sort(e, e+length, Sorter(inSorter) );
   }


   // Dynamic interface
   virtual Dynamic __concat(const Dynamic &a0) { return concat(a0); }
   virtual Dynamic __copy() { return copy(); }
   virtual Dynamic __insert(const Dynamic &a0,const Dynamic &a1) { insert(a0,a1); return null(); }
   virtual Dynamic __iterator() { return iterator(); }
   virtual Dynamic __join(const Dynamic &a0) { return join(a0); }
   virtual Dynamic __pop() { return pop(); }
   virtual Dynamic __push(const Dynamic &a0) { return push(a0);}
   virtual Dynamic __remove(const Dynamic &a0) { return remove(a0); }
   virtual Dynamic __reverse() { reverse(); return null(); }
   virtual Dynamic __shift() { return shift(); }
   virtual Dynamic __slice(const Dynamic &a0,const Dynamic &a1) { return slice(a0,a1); }
   virtual Dynamic __splice(const Dynamic &a0,const Dynamic &a1) { return splice(a0,a1); }
   virtual Dynamic __sort(const Dynamic &a0) { sort(a0); return null(); }
   virtual Dynamic __toString() { return toString(); }
   virtual Dynamic __unshift(const Dynamic &a0) { unshift(a0); return null(); }
};



// --- Array ---------------------------------------------------------------
//
// The array class adds object syntax to the Array_obj pointer

template<typename ELEM_>
class Array : public hxObjectPtr< Array_obj<ELEM_> >
{
   typedef hxObjectPtr< Array_obj<ELEM_> > super;
   typedef Array_obj<ELEM_> OBJ_;

public:
   typedef Array_obj<ELEM_> *Ptr;
   using super::mPtr;
   using super::GetPtr;

   Array() { }
   Array(int inSize,int inReserve) : super( OBJ_::__new(inSize,inReserve) ) { }
   Array(const null &inNull) : super(0) { }
   Array(Ptr inPtr) : super(inPtr) { }


   // Construct from our type ...
   Array ( const hxObjectPtr< OBJ_  > &inArray )
        :  hxObjectPtr< OBJ_ >(inArray) { }

   // Construct from general pointer (eg, dynamic)
   template<typename SOURCE_>
   Array( const hxObjectPtr<SOURCE_> &inRHS ) : super(0)
   {
      SOURCE_ *ptr = inRHS.GetPtr(); 
      if (ptr)
      {
         OBJ_ *arr = dynamic_cast<OBJ_ *>(ptr);
         if (!arr)
         {
            // Non-identical type.
            // Copy elements one-by-one
            // Not quite right, but is the best we can do...
            int n = ptr->__length();
            *this = Array_obj<ELEM_>::__new(n);
            for(int i=0;i<n;i++)
               mPtr->QuickItem(i) = ptr->__GetItem(i);
         }
         else
            mPtr = arr;
      }
   }


   // Constuct from foreign array ...
   template<typename SOURCE_ELEM_>
   Array(const Array<SOURCE_ELEM_> &inArray) : super(0)
   {
      if (inArray.GetPtr())
      {
         int n = inArray->size();
         *this = Array_obj<ELEM_>::__new(n);
         for(int i=0;i<n;i++)
            mPtr->QuickItem(i) = inArray->__get(i);
      }
   }

   Array(const Array<ELEM_> &inArray) : super(inArray.GetPtr()) { }


   Array &operator=( const hxObjectPtr<OBJ_ >&inArray )
   {
      mPtr = inArray.GetPtr();
      return *this;
   }
   Array &operator=( const Dynamic &inRHS )
   {
      hxObject *ptr = inRHS.GetPtr();
      if (ptr)
      {
         mPtr = dynamic_cast<OBJ_ *>(ptr);
         if (!mPtr) throw INVALID_CAST;
      }
      else
         mPtr = 0;
      return *this;
   }

   Array &operator=( const null &inNull )
   {
      mPtr = 0;
      return *this;
   }



   inline ELEM_ &operator[](int inIdx) { return GetPtr()->Item(inIdx); }
   inline ELEM_ operator[](int inIdx) const { return GetPtr()->__get(inIdx); }
   //inline ELEM_ __get(int inIdx) const { return GetPtr()->__get(inIdx); }
   inline int __length() const { return GetPtr()->__length(); }
   inline Array<ELEM_> &Add(const ELEM_ &inElem) { GetPtr()->Add(inElem); return *this; }
   inline Array<ELEM_> & operator<<(const ELEM_ &inElem) { GetPtr()->Add(inElem); return *this; }
};


// Now that the "Array" object is defined, we can implement this function ....

template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::__new(int inSize,int inReserve)
 { return  Array<ELEM_>(new Array_obj(inSize,inReserve)); }


template<>
inline bool Dynamic::IsClass<Array<Dynamic> >()
   { return mPtr && mPtr->__GetClass()== hxArrayBase::__mClass; }

// --- Class_obj --------------------------------------------------------------------
//
// The Class_obj provides the type information required by the Reflect and type APIs.

typedef Dynamic (*ConstructEmptyFunc)();
typedef Dynamic (*ConstructArgsFunc)(DynamicArray inArgs);
typedef Dynamic (*ConstructEnumFunc)(String inName,DynamicArray inArgs);
typedef void (*MarkFunc)();

inline bool operator!=(ConstructEnumFunc inFunc,const null &inNull) { return inFunc!=0; }


typedef bool (*CanCastFunc)(hxObject *inPtr);

class  Class_obj : public hxObject
{
public:
   Class_obj() : mSuper(0) { };
   Class_obj(const String &inClassName, String inStatics[], String inMembers[],
             ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
             Class *inSuperClass, ConstructEnumFunc inConstructEnum,
             CanCastFunc inCanCast, MarkFunc inMarkFunc);

   String __ToString() const;

   void __Mark();

   void MarkStatics();

   // the "Class class"
   Class              __GetClass() const;
   static Class      & __SGetClass();

   Dynamic __Field(const String &inString);

   Dynamic __SetField(const String &inString,const Dynamic &inValue);


   int __GetType() const { return vtObject; }

   CanCastFunc        CanCast;


   Array<String>      GetInstanceFields();
   Array<String>      GetClassFields();
   Class              GetSuper();
   static Class       Resolve(String inName);

   Class              *mSuper;
   String             mName;
   ConstructArgsFunc  mConstructArgs;
   ConstructEmptyFunc mConstructEmpty;
   ConstructEnumFunc  mConstructEnum;
   MarkFunc           mMarkFunc;
   Array<String>      mStatics;
   Array<String>      mMembers;
};

typedef hxObjectPtr<Class_obj> Class;


// --- All classes should be registered with this function via the "__boot" method

 Class RegisterClass(const String &inClassName, CanCastFunc inCanCast,
                    String inStatics[], String inMembers[],
                    ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
                    Class *inSuperClass, ConstructEnumFunc inConst=0, MarkFunc inMarkFunc=0);

template<typename T>
inline bool TCanCast(hxObject *inPtr) { return dynamic_cast<T *>(inPtr)!=0; }


// Enum (ie enum object class def)  is the same as Class.
typedef Class Enum;


// --- hxEnumBase_obj ----------------------------------------------------------
//
// Base class for Enums.
// Specializations of this class don't actually add more data, just extra constructors
//  and type information.

class  hxEnumBase_obj : public hxObject
{
   typedef hxObject super;
   typedef hxEnumBase_obj OBJ_;

   protected:
      String       tag;
      DynamicArray mArgs;
   public:
      int          index;

   public:
      DO_ENUM_RTTI_INTERNAL;
      static hxObjectPtr<Class_obj> &__SGetClass();


      String toString();

      hxEnumBase_obj() : index(-1) { }
      hxEnumBase_obj(const null &inNull) : index(-1) { }
      static Dynamic __CreateEmpty();
      static Dynamic __Create(DynamicArray inArgs);
      static void __boot();

      void __Mark();

      static hxObjectPtr<hxEnumBase_obj> Resolve(String inName);
      Dynamic __Param(int inID) { return mArgs[inID]; }
      inline int GetIndex() { return index; }

      DynamicArray __EnumParams() { return mArgs; }
      String __Tag() const { return tag; }
      int __Index() const { return index; }

      int __GetType() const { return vtEnum; }

      int __Compare(const hxObject *inRHS) const
      {
         if (inRHS->__GetType()!=vtEnum) return -1;
         const hxEnumBase_obj *rhs = dynamic_cast<const hxEnumBase_obj *>(inRHS);
         if (tag!=rhs->tag || GetEnumName()!=rhs->GetEnumName()) return -1;
         if (mArgs==null() && rhs->mArgs==null())
            return 0;
         if (mArgs==null() || rhs->mArgs==null())
            return -1;

         int n = mArgs->__length();
         if (rhs->mArgs->__length()!=n)
            return -1;
         for(int i=0;i<n;i++)
            if ( mArgs[i] != rhs->mArgs[i] )
               return -1;
         return 0;
      }

      void Set( const String &inName,int inIndex,DynamicArray inArgs)
      {
         tag = inName;
         index = inIndex;
         mArgs = inArgs;
      }
      virtual String GetEnumName( ) const { return L"Enum"; }
};


typedef hxObjectPtr<hxEnumBase_obj> hxEnumBase;

// --- hxResource -------------------------------------------------------------

struct hxResource
{
   String        mName;
   int           mDataLength;
   unsigned char *mData;

   bool operator<(const hxResource &inRHS) const { return mName < inRHS.mName; }
};

hxResource *GetResources();

 void RegisterResources(hxResource *inResources);

 Array<String> __hxcpp_resource_names();
 String __hxcpp_resource_string(String inName);
 Array<unsigned char> __hxcpp_resource_bytes(String inName);

// --- CreateEnum -------------------------------------------------------------
//
// Template function to return a strongly-typed version fo the Enum.
// Most of the common stuff is in "Set".

template<typename ENUM>
hxObjectPtr<ENUM> CreateEnum(const String &inName,int inIndex, DynamicArray inArgs=DynamicArray())
{
   ENUM *result = new ENUM;
   result->Set(inName,inIndex,inArgs);
   return result;
}

// Operators for mixing various types ....


inline String operator+(const Int &i,const String &s) { return String(i) + s; }
inline String operator+(const double &d,const String &s) { return String(d) + s; }
inline String operator+(const bool &b,const String &s) { return String(b) + s; }
inline String operator+(const wchar_t *c,const String &s) { return String(c) + s; }
inline String operator+(const null &n,const String &s) { return String(n) + s; }
inline String operator+(const cpp::CppInt32__ &i,const String &s) { return String(i) + s; }

template<typename T_>
   inline String operator+(const hxObjectPtr<T_> &inLHS,const String &s)
   { return (inLHS.GetPtr() ? const_cast<hxObjectPtr<T_> & >(inLHS)->toString() : String(L"null",4) ) + s; }

template<typename RHS_>
   inline Dynamic operator+(const hxFieldRef &inField,RHS_ &inRHS)
   { return inField.operator Dynamic() + inRHS; }



// += -= *= /= %= &= |= ^= <<= >>= >>>=
template<typename L, typename R>
inline L& hxAddEq(L &inLHS, R inRHS) { inLHS = inLHS + inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxMultEq(L &inLHS, R inRHS) { inLHS = (double)inLHS * (double)inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxDivEq(L &inLHS, R inRHS) { inLHS = (double)inLHS / (double)inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxSubEq(L &inLHS, R inRHS) { inLHS = (double)inLHS - (double)inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxAndEq(L &inLHS, R inRHS) { inLHS = (int)inLHS & (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxOrEq(L &inLHS, R inRHS) { inLHS = (int)inLHS | (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxXorEq(L &inLHS, R inRHS) { inLHS = (int)inLHS ^ (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxShlEq(L &inLHS, R inRHS) { inLHS = (int)inLHS << (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxShrEq(L &inLHS, R inRHS) { inLHS = (int)inLHS >> (int)inRHS; return inLHS; }
template<typename L, typename R>
inline L& hxUShrEq(L &inLHS, R inRHS) { inLHS = hxUShr(inLHS,inRHS); return inLHS; }
template<typename L, typename R>
inline L& hxModEq(L &inLHS, R inRHS) { inLHS = (int)inLHS % (int)inRHS; return inLHS; }

#ifdef __GNUC__
template<typename R>
inline hxFieldRef hxAddEq(hxFieldRef inLHS, R inRHS) { inLHS = inLHS + inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxMultEq(hxFieldRef inLHS, R inRHS) { inLHS = (double)inLHS * (double)inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxDivEq(hxFieldRef inLHS, R inRHS) { inLHS = (double)inLHS / (double)inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxSubEq(hxFieldRef inLHS, R inRHS) { inLHS = (double)inLHS - (double)inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxAndEq(hxFieldRef inLHS, R inRHS) { inLHS = (int)inLHS & (int)inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxOrEq(hxFieldRef inLHS, R inRHS) { inLHS = (int)inLHS | (int)inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxXorEq(hxFieldRef inLHS, R inRHS) { inLHS = (int)inLHS ^ (int)inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxShlEq(hxFieldRef inLHS, R inRHS) { inLHS = (int)inLHS << (int)inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxShrEq(hxFieldRef inLHS, R inRHS) { inLHS = (int)inLHS >> (int)inRHS; return inLHS; }
template<typename R>
inline hxFieldRef hxUShrEq(hxFieldRef inLHS, R inRHS) { inLHS = hxUShr(inLHS,inRHS); return inLHS; }
template<typename R>
inline hxFieldRef hxModEq(hxFieldRef inLHS, R inRHS) { inLHS = (int)inLHS % (int)inRHS; return inLHS; }


#endif


// --- Garbage Collection --------------------------------------------------


// Create a new root.
// All statics are explicitly registered - this saves adding the whole data segment
// to the collection list.
void __RegisterStatic(void *inPtr,int inSize);

void hxGCAddRoot(hxObject **inRoot);
void hxGCRemoveRoot(hxObject **inRoot);

// This may not be needed now that GC memsets everything to 0.
template<typename T> inline void InitMember(T &outT) { }
template<> inline void InitMember<int>(int &outT) { outT = 0; }
template<> inline void InitMember<bool>(bool &outT) { outT = false; }
template<> inline void InitMember<double>(double &outT) { outT = 0; }


template<typename T> inline void MarkMember(T &outT) { }
template<typename T> inline void MarkMember(hxObjectPtr<T> &outT)
{
	HX_MARK_OBJECT(outT.mPtr);
}
template<> inline void MarkMember(Dynamic &outT)
{
	HX_MARK_OBJECT(outT.mPtr);
}
template<typename T> inline void MarkMember(Array<T> &outT)
{
	HX_MARK_OBJECT(outT.mPtr);
}
template<> inline void MarkMember<int>(int &outT) {  }
template<> inline void MarkMember<bool>(bool &outT) {  }
template<> inline void MarkMember<double>(double &outT) {  }
template<> inline void MarkMember<String>(String &outT)
{
   HX_MARK_STRING(outT.__s,wchar_t *);
}
template<> inline void MarkMember<Void>(Void &outT) {  }


// Template used to register and initialise the statics in the one call.
template<typename T> inline T &Static(T &inPtr) { __RegisterStatic(&inPtr,sizeof(void *)); return inPtr; }

// Make sure we get the "__s" pointer
template<> inline String &Static<String>(String &inString)
   { __RegisterStatic(&inString, sizeof(String)); return inString; }

// Do nothing
template<> inline int &Static<int>(int &inPtr) { return inPtr; }
template<> inline bool &Static<bool>(bool &inPtr) { return inPtr; }
template<> inline double &Static<double>(double &inPtr) { return inPtr; }


// Initialise the garbage collection and hxcpp statics.
void  __boot_hxcpp();

// Helpers for debugging code
void  __hxcpp_reachable(hxObject *inKeep);
void  __hxcpp_enable(bool inEnable);
void  __hxcpp_collect();

// Dynamic casting
bool  __instanceof(const Dynamic &inValue, const Dynamic &inType);


 int __int__(double x);

// Used for accessing object fields by integer ID, rather than string ID.
// Used mainly for neko ndll interaction.
 int  __hxcpp_field_to_id( const char *inField );
 const String &__hxcpp_field_from_id( int f );
// Get function pointer from dll file
 Dynamic __loadprim(String inLib, String inPrim,int inArgCount);

// Loading functions via name (dummy return value)
 int __hxcpp_register_prim(wchar_t *inName,void *inFunc);

// Throw must return a value ...
inline Dynamic hxThrow(Dynamic inError) { throw inError; return Dynamic(); }

// Used by std library...
 int ParseInt(const String &inString);
 double ParseFloat(const String &inString);
void  __trace(Dynamic inPtr, Dynamic inData);
double  __time_stamp();

 void __hxcpp_print(Dynamic &inV);
 void __hxcpp_println(Dynamic &inV);

 bool __hxcpp_same_closure(Dynamic &inF1,Dynamic &inF2);

// System access
Array<String>  __get_args();

// haxe.Int32
 void hxCheckOverflow(int inVal);

// haxe.io.BytesData

 void __hxcpp_bytes_of_string(Array<unsigned char> &outBytes,const String &inString);
 void __hxcpp_string_of_bytes(Array<unsigned char> &inBytes,String &outString,int pos,int len);

// Int hash access...
hxObject  * CreateIntHash();
void  __int_hash_set(Dynamic &inHash,int inKey,const Dynamic &value);
Dynamic   __int_hash_get(Dynamic &inHash,int inKey);
bool   __int_hash_exists(Dynamic &inHash,int inKey);
bool   __int_hash_remove(Dynamic &inHash,int inKey);
Dynamic  __int_hash_keys(Dynamic &inHash);
Dynamic  __int_hash_values(Dynamic &inHash);


// String hash access
/*
hxObject  * CreateStringHash();
void  _string_hash_set(Dynamic &inHash,String &inKey,const Dynamic &inValue);
Dynamic   _string_hash_get(Dynamic &inHash,String &inKey);
bool   _string_hash_exists(Dynamic &inHash,String &inKey);
bool   _string_hash_remove(Dynamic &inHash,String &inKey);
Dynamic  _string_hash_keys(Dynamic &inHash);
*/

// Date class

 double __hxcpp_new_date(int inYear,int inMonth,int inDay,int inHour, int inMin, int inSeconds);
 int __hxcpp_get_hours(double inSeconds);
 int __hxcpp_get_minutes(double inSeconds);
 int __hxcpp_get_seconds(double inSeconds);
 int __hxcpp_get_year(double inSeconds);
 int __hxcpp_get_month(double inSeconds);
 int __hxcpp_get_date(double inSeconds);
 int __hxcpp_get_day(double inSeconds);
 String __hxcpp_to_string(double inSeconds);
 double __hxcpp_date_now();



#ifdef BIG_ENDIAN
#undef BIG_ENDIAN
#endif

#ifdef LITTLE_ENDIAN
#undef LITTLE_ENDIAN
#endif

#endif
