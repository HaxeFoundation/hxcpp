#ifndef HX_STRING_H
#define HX_STRING_H

#ifndef HXCPP_H
#error "Please include hxcpp.h, not hx/Object.h"
#endif


// --- String --------------------------------------------------------
//
// Basic String type for hxcpp.
// It's based on garbage collection of the wchar_t *ptr.
// Note: this does not inherit from "hx::Object", so in some ways it acts more
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

   static void __boot();

	hx::Object *__ToObject() const;

   template<typename T>
   inline String(const hx::ObjectPtr<T> &inRHS)
   {
      if (inRHS.mPtr)
      {
         String s = static_cast<hx::Object *>(inRHS.mPtr)->toString();
         __s = s.__s;
         length = s.length;
      }
      else { __s = 0; length = 0; }
   }
    String(const Dynamic &inRHS);

   inline String &operator=(const String &inRHS)
           { length = inRHS.length; __s = inRHS.__s; return *this; }

   String Default(const String &inDef) { return __s ? *this : inDef; }


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
   inline String operator+(const hx::ObjectPtr<T> &inRHS) const
      { return *this + (inRHS.mPtr ? const_cast<hx::ObjectPtr<T>&>(inRHS)->toString() : String(L"null",4) ); }

   inline bool operator==(const String &inRHS) const
                     { return length==inRHS.length && compare(inRHS)==0; }
   inline bool operator!=(const String &inRHS) const
                     { return length != inRHS.length || compare(inRHS)!=0; }
   inline bool operator<(const String &inRHS) const { return compare(inRHS)<0; }
   inline bool operator<=(const String &inRHS) const { return compare(inRHS)<=0; }
   inline bool operator>(const String &inRHS) const { return compare(inRHS)>0; }
   inline bool operator>=(const String &inRHS) const { return compare(inRHS)>=0; }

   inline int cca(int inPos) const { return __s[inPos]; }


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

	// The actual implementation.
	// Note that "__s" is const - if you want to change it, you should create a new string.
	//  this allows for multiple strings to point to the same data.
   int length;
   const wchar_t *__s;
};



#endif
