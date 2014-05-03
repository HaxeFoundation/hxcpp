#ifndef CPP_POINTER_H
#define CPP_POINTER_H

namespace cpp
{

struct AutoCast
{
   void *value;

   explicit inline AutoCast(void *inValue) : value(inValue) { }
};

template<typename T>
class Pointer
{
public:
   T *ptr;

   inline Pointer( ) { }
   inline Pointer( const Pointer &inRHS ) : ptr(inRHS.ptr) {  }
   inline Pointer( const Dynamic &inRHS) { ptr = inRHS==null()?0: (T*)inRHS->__GetHandle(); }
   inline Pointer( const null &inRHS ) { }
   inline Pointer( T *inValue ) : ptr(inValue) { }
   inline Pointer( AutoCast inValue ) : ptr( (T*)inValue.value) { }
   inline Pointer operator=( Pointer &inRHS ) { return ptr = inRHS.ptr; }
   inline Dynamic operator=( Dynamic &inValue )
   {
      ptr = inValue==null() ? 0 : (T*) inValue->__GetHandle();
   }
   inline Dynamic operator=( null &inValue ) { ptr=0; return inValue; }

   // Allow '->' syntax
   inline Pointer *operator->() { return this; }
 	inline Pointer inc() { return ++ptr; }
	inline Pointer dec() { return --ptr; }
	inline Pointer add(int inInt) { return ptr+inInt; }
 	inline Pointer incBy(int inDiff) { ptr+=inDiff; return ptr; }
 	inline Pointer postIncBy(int inDiff) { T *result = ptr; ptr+=inDiff; return result; }


   inline T &__get(int inIndex) { return ptr[inIndex]; }
   inline T &__set(int inIndex, T inValue) { T *p = ptr+inIndex; *p = inValue; return *p; }

   inline T get_value() { return *ptr; }
   inline T set_value(T inValue) { return *ptr = inValue;  }

   //operator Dynamic () { return null(); }
   operator T * () { return ptr; }

   inline void destroy() { delete ptr; }
   inline void destroyArray() { delete [] ptr; }

   inline bool lt(Pointer inOther) { return ptr < inOther.ptr; }
   inline bool gt(Pointer inOther) { return ptr > inOther.ptr; }
   inline bool leq(Pointer inOther) { return ptr <= inOther.ptr; }
   inline bool geq(Pointer inOther) { return ptr >= inOther.ptr; }

};



class Pointer_obj
{
public:
   template<typename T>
	inline static AutoCast arrayElem(::Array<T> array, int inIndex)  { return AutoCast(&array[inIndex]); }
	inline static AutoCast arrayElem(Dynamic inVal, int inIndex)
   {
      if (inVal==null() || !inVal->__IsArray())
         return AutoCast(0);
      hx::ArrayBase *base = (hx::ArrayBase *)inVal.GetPtr();
      return AutoCast(base->GetBase());
   }
   template<typename T>
	inline static Pointer<T> addressOf(T &value)  { return Pointer<T>(&value); }
};



} // end namespace cpp


#endif
