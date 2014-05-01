#ifndef CPP_POINTER_H
#define CPP_POINTER_H

namespace cpp
{

template<typename T>
class Pointer
{
public:
   T *ptr;

   inline Pointer( ) { }
   inline Pointer( const Pointer &inRHS ) : ptr(inRHS.ptr) {  }
   inline Pointer( const Dynamic &inRHS) { }
   inline Pointer( const null &inRHS ) { }
   inline Pointer( T *inValue ) : ptr(inValue) { }
   inline Pointer operator=( Pointer &inRHS ) { return ptr = inRHS.ptr; }
   inline Dynamic operator=( Dynamic &inValue ) { return inValue; }
   inline Dynamic operator=( null &inValue ) { return inValue; }

   // Allow '->' syntax
   inline Pointer *operator->() { return this; }
 	inline void inc() { ++ptr; }
	inline void dec() { -- ptr; }
	inline void add(int inInt) { ptr+=inInt; }


   inline T &__get(int inIndex) { return ptr[inIndex]; }
   inline T &__set(int inIndex, T inValue) { T *p = ptr+inIndex; *p = inValue; return *p; }

   inline T get_value() { return *ptr; }
   inline T set_value(T inValue) { return *ptr = inValue;  }

   operator Dynamic () { return null(); }
   operator T * () { return ptr; }

};



class Pointer_obj
{
public:
   template<typename T>
	inline static Pointer<T> addressOf(T &value)  { return Pointer<T>(&value); }
};



} // end namespace cpp


#endif
