#ifndef CPP_POINTER_H
#define CPP_POINTER_H

namespace cpp
{

template<typename T>
class Pointer
{
public:
   T *value;


   inline Pointer( ) { }
   inline Pointer( const Pointer &inRHS ) : value(inRHS.value) {  }
   inline Pointer( const Dynamic &inRHS) { }
   inline Pointer( const null &inRHS ) { }
   inline Pointer( T *inValue ) : value(inValue) { }
   inline Pointer operator=( Pointer &inRHS ) { return value = inRHS.value; }
   inline Dynamic operator=( Dynamic &inValue ) { return inValue; }
   inline Dynamic operator=( null &inValue ) { return inValue; }

   // Allow '->' syntax
   inline Pointer *operator->() { return this; }
 	inline void inc() { ++value; }
	inline void dec() { -- value; }
	inline void add(int inInt) { value+=inInt; }


   inline T *ref() { return value; }
   inline T &__get(int inIndex) { return value[inIndex]; }
   inline T &__set(int inIndex, T inValue) { T *p = value+inIndex; *p = inValue; return *p; }

   operator Dynamic () { return null(); }
   operator T * () { return value; }

};



class Pointer_obj
{
public:
   template<typename T>
	inline static Pointer<T> fromArray(Array<T> array, int index)  { return Pointer<T>(&array[index]); }
};



} // end namespace cpp


#endif
