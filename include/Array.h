#ifndef HX_ARRAY_H
#define HX_ARRAY_H

#include <cpp/FastIterator.h>

// --- hx::ReturnNull ------------------------------------------------------
//
// Provides an "Null<Object>" of given type.  For types that can't actually be null, Dynamic is used.

namespace hx
{

template<typename T> struct ReturnNull { typedef T type; };
template<> struct ReturnNull<int> { typedef Dynamic type; };
template<> struct ReturnNull<double> { typedef Dynamic type; };
template<> struct ReturnNull<float> { typedef Dynamic type; };
template<> struct ReturnNull<bool> { typedef Dynamic type; };
template<> struct ReturnNull<char> { typedef Dynamic type; };
template<> struct ReturnNull<signed char> { typedef Dynamic type; };
template<> struct ReturnNull<unsigned char> { typedef Dynamic type; };
template<> struct ReturnNull<short> { typedef Dynamic type; };
template<> struct ReturnNull<unsigned short> { typedef Dynamic type; };
template<> struct ReturnNull<unsigned int> { typedef Dynamic type; };

}


namespace hx
{



// --- ArrayIterator -------------------------------------------
//
// An object that conforms to the standard iterator interface for arrays
template<typename FROM,typename TO>
class ArrayIterator : public cpp::FastIterator_obj<TO>
{
public:
   ArrayIterator(Array<FROM> inArray) : mArray(inArray), mIdx(0) { }

   // Fast versions ...
   bool hasNext()  { return mIdx < mArray->length; }

   inline TO toTo(const Dynamic &inD) { return inD.StaticCast<TO>(); }

   template<typename T>
   inline TO toTo(T inT) { return inT; }

   TO next() { return toTo(mArray->__get(mIdx++)); }

   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER_NAME(mArray,"mArray"); }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER_NAME(mArray,"mArray"); }
   #endif

   int      mIdx;
   Array<FROM> mArray;
};

}

namespace hx
{

// --- hx::ArrayBase ----------------------------------------------------
//
// Base class that treats array contents as a slab of bytes.
// The derived "Array_obj" adds strong typing to the "[]" operator

class HXCPP_EXTERN_CLASS_ATTRIBUTES ArrayBase : public hx::Object
{
public:
   ArrayBase(int inSize,int inReserve,int inElementSize,bool inAtomic);

   // Defined later so we can use "Array"
   static Array<Dynamic> __new(int inSize=0,int inReserve=0);


   static void __boot();

   typedef hx::Object super;

   // Used by cpp.ArrayBase
   inline int getElementSize() const { return GetElementSize(); }
   inline int getByteCount() const { return GetElementSize()*length; }
   inline char * getBase() const { return mBase; }


   Dynamic __SetField(const String &inString,const Dynamic &inValue ,hx::PropertyAccess inCallProp) { return null(); }

   static hx::Class __mClass;
   static hx::Class &__SGetClass() { return __mClass; }
   hx::Class __GetClass() const { return __mClass; }
   String toString();
   String __ToString() const;

   void setData(void *inData, int inElements)
   {
      mBase = (char *)inData;
      length = inElements;
      mAlloc = inElements;
   }

   void setUnmanagedData(void *inData, int inElements)
   {
      mBase = (char *)inData;
      length = inElements;
      mAlloc = -1;
   }


   int __GetType() const { return vtArray; }

   inline size_t size() const { return length; }
   inline int __length() const { return (int)length; }

   virtual String ItemString(int inI)  = 0;

   const char * __CStr() const { return mBase; }
   inline const char *GetBase() const { return mBase; }
   inline char *GetBase() { return mBase; }

   virtual int GetElementSize() const = 0;

   void __SetSize(int inLen);
   void __SetSizeExact(int inLen=0);

   void safeSort(Dynamic sorter, bool isString);

   inline void __unsafeStringReference(String inString)
   {
      mBase = (char *)inString.__s;
      length = inString.length / GetElementSize();
      mAlloc = length;
   }

   // Dynamic interface
   Dynamic __Field(const String &inString ,hx::PropertyAccess inCallProp);
   virtual Dynamic __concat(const Dynamic &a0) = 0;
   virtual Dynamic __copy() = 0;
   virtual Dynamic __insert(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual Dynamic __iterator() = 0;
   virtual Dynamic __join(const Dynamic &a0) = 0;
   virtual Dynamic __pop() = 0;
   virtual Dynamic __push(const Dynamic &a0) = 0;
   virtual Dynamic __remove(const Dynamic &a0) = 0;
   virtual Dynamic __indexOf(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual Dynamic __lastIndexOf(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual Dynamic __reverse() = 0;
   virtual Dynamic __shift() = 0;
   virtual Dynamic __slice(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual Dynamic __splice(const Dynamic &a0,const Dynamic &a1) =0;
   virtual Dynamic __sort(const Dynamic &a0) = 0;
   virtual Dynamic __toString() = 0;
   virtual Dynamic __unshift(const Dynamic &a0) = 0;
   virtual Dynamic __map(const Dynamic &func) = 0;
   virtual Dynamic __filter(const Dynamic &func) = 0;
   inline Dynamic ____SetSize(const Dynamic &len)  { __SetSize(len); return this; } 
   inline Dynamic ____SetSizeExact(const Dynamic &len)  { __SetSizeExact(len); return this; } 
   inline Dynamic ____unsafe_set(const Dynamic &i, const Dynamic &val)  { return __SetItem(i,val); } 
   inline Dynamic ____unsafe_get(const Dynamic &i)  { return __GetItem(i); } 
   virtual Dynamic __blit(const Dynamic &a0,const Dynamic &a1,const Dynamic &a2,const Dynamic &a3) = 0;
   inline Dynamic __zero(const Dynamic &a0,const Dynamic &a1)  { zero(a0,a1); return null(); }
   virtual Dynamic __memcmp(const Dynamic &a0) = 0;


   Dynamic concat_dyn();
   Dynamic copy_dyn();
   Dynamic insert_dyn();
   Dynamic iterator_dyn();
   Dynamic join_dyn();
   Dynamic pop_dyn();
   Dynamic push_dyn();
   Dynamic remove_dyn();
   Dynamic indexOf_dyn();
   Dynamic lastIndexOf_dyn();
   Dynamic reverse_dyn();
   Dynamic shift_dyn();
   Dynamic slice_dyn();
   Dynamic splice_dyn();
   Dynamic sort_dyn();
   Dynamic toString_dyn();
   Dynamic unshift_dyn();
   Dynamic map_dyn();
   Dynamic filter_dyn();
   Dynamic __SetSize_dyn();
   Dynamic __SetSizeExact_dyn();
   Dynamic __unsafe_get_dyn();
   Dynamic __unsafe_set_dyn();
   Dynamic blit_dyn();
   Dynamic zero_dyn();
   Dynamic memcmp_dyn();

   void EnsureSize(int inLen) const;

   void RemoveElement(int inIndex);


   void Insert(int inPos);

   void Splice(hx::ArrayBase *outResult,int inPos,int inLen);

   void Slice(hx::ArrayBase *outResult,int inPos,int inEnd);

   void Concat(hx::ArrayBase *outResult,const char *inEnd, int inLen);


   void reserve(int inN);

   // Set numeric values to 0, pointers to null, bools to false
   void zero(Dynamic inFirst, Dynamic inCount);

   int Memcmp(ArrayBase *inArray);

   // Copy section of other array.
   void Blit(int inDestElement, ArrayBase *inSourceArray, int inSourceElement, int inElementCount);

   String join(String inSeparator);


   virtual bool AllocAtomic() const { return false; }

   virtual bool IsByteArray() const = 0;


   inline Dynamic __get(int inIndex) const { return __GetItem(inIndex); }


   mutable int length;
protected:
   mutable int mAlloc;
   mutable char  *mBase;
};

} // end namespace hx for ArrayBase

namespace cpp
{
   typedef hx::ArrayBase ArrayBase_obj;

   // Use by cpp.ArrayBase extern
   typedef hx::ObjectPtr<ArrayBase_obj> ArrayBase;
}



// --- Array_obj ------------------------------------------------------------------
//
// The Array_obj specialises the ArrayBase, adding typing where required


namespace hx
{
// This is to determine is we need to include our slab of bytes in garbage collection
template<typename T>
inline bool TypeContainsPointers(T *) { return true; }
template<> inline bool TypeContainsPointers(bool *) { return false; }
template<> inline bool TypeContainsPointers(int *) { return false; }
template<> inline bool TypeContainsPointers(double *) { return false; }
template<> inline bool TypeContainsPointers(float *) { return false; }
template<> inline bool TypeContainsPointers(unsigned char *) { return false; }

template<typename TYPE> inline bool ContainsPointers()
{
   return TypeContainsPointers( (TYPE *)0 );
}


// For returning "null" when out of bounds ...
template<typename TYPE>
inline TYPE *NewNull() { Dynamic d; return (TYPE *)hx::NewGCBytes(&d,sizeof(d)); }

template<> inline int *NewNull<int>() { int i=0; return (int *)hx::NewGCPrivate(&i,sizeof(i)); }
template<> inline bool *NewNull<bool>() { bool b=0; return (bool *)hx::NewGCPrivate(&b,sizeof(b)); }
template<> inline double *NewNull<double>() { double d=0.0; return (double *)hx::NewGCPrivate(&d,sizeof(d)); }
template<> inline float *NewNull<float>() { float d=0.0f; return (float *)hx::NewGCPrivate(&d,sizeof(d)); }
template<> inline unsigned char *NewNull<unsigned char>() { unsigned char u=0; return (unsigned char *)hx::NewGCPrivate(&u,sizeof(u)); }

}

// sort...
#include <algorithm>


template<typename T>
struct ArrayTraits { enum { IsByteArray = 0, IsDynamic = 0, IsString = 0  }; };
template<>
struct ArrayTraits<unsigned char> { enum { IsByteArray = 1, IsDynamic = 0, IsString = 0  }; };
template<>
struct ArrayTraits<Dynamic> { enum { IsByteArray = 0, IsDynamic = 1, IsString = 0  }; };
template<>
struct ArrayTraits<String> { enum { IsByteArray = 0, IsDynamic = 0, IsString = 1  }; };

template<typename ELEM_>
class Array_obj : public hx::ArrayBase
{
   typedef ELEM_ Elem;
   typedef hx::ObjectPtr< Array_obj<ELEM_> > ObjPtr;
   typedef typename hx::ReturnNull<ELEM_>::type NullType;


public:
   Array_obj(int inSize,int inReserve) :
        hx::ArrayBase(inSize,inReserve,sizeof(ELEM_),!hx::ContainsPointers<ELEM_>()) { }


   // Defined later so we can use "Array"
   static Array<ELEM_> __new(int inSize=0,int inReserve=0);

   virtual bool AllocAtomic() const { return !hx::ContainsPointers<ELEM_>(); }


   virtual Dynamic __GetItem(int inIndex) const { return __get(inIndex); }
   virtual Dynamic __SetItem(int inIndex,Dynamic inValue)
           { Item(inIndex) = inValue; return inValue; }

   inline ELEM_ *Pointer() { return (ELEM_ *)mBase; }

   inline ELEM_ &Item(int inIndex)
   {
      if (inIndex>=(int)length) EnsureSize(inIndex+1);
      else if (inIndex<0) { return * hx::NewNull<ELEM_>(); }
      return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_));
   }
   inline ELEM_ __get(int inIndex) const
   {
      if (inIndex>=(int)length || inIndex<0) return null();
      return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_));
   }

   // Does not check for size valid - use with care
   inline ELEM_ &__unsafe_get(int inIndex) { return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_)); }
   inline ELEM_ & __unsafe_set(int inIndex, const ELEM_ &inValue)
   {
      return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_)) = inValue;
   }

   inline int memcmp(Array<ELEM_> inOther)
   {
      return ArrayBase::Memcmp(inOther.GetPtr());
   }

   inline void blit(int inDestElement,  Array<ELEM_> inSourceArray,
                    int inSourceElement, int inElementCount)
   {
      ArrayBase::Blit(inDestElement, inSourceArray.GetPtr(), inSourceElement, inElementCount);
   }


   void __Mark(hx::MarkContext *__inCtx)
   {
      if (hx::ContainsPointers<ELEM_>())
      {
         ELEM_ *ptr = (ELEM_ *)mBase;
         for(int i=0;i<length;i++)
            HX_MARK_MEMBER(ptr[i]);
      }
      if (mAlloc>0) hx::MarkAlloc((void *)mBase, __inCtx );
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      if (mAlloc>0) __inCtx->visitAlloc((void **)&mBase);
      if (hx::ContainsPointers<ELEM_>())
      {
         ELEM_ *ptr = (ELEM_ *)mBase;
         for(int i=0;i<length;i++)
            HX_VISIT_MEMBER(ptr[i]);
      }
   }
   #endif

   inline Array<ELEM_> __SetSizeExact(int inLen);

   int GetElementSize() const { return sizeof(ELEM_); }

   String ItemString(int inI)
   {
      String result(__get(inI));
      if (result==null()) return HX_CSTRING("null");
      return result;
   }

   Array_obj<ELEM_> *Add(const ELEM_ &inItem) { push(inItem); return this; }


   // Haxe API
   inline int push( ELEM_ inVal )
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

   int indexOf(ELEM_ inValue, Dynamic fromIndex = null())
   {
      int len = length;
      int i = fromIndex==null() ? 0 : fromIndex->__ToInt();
      ELEM_ *e = (ELEM_ *)mBase;
      if (i < 0)
      {
         i += len;
         if (i < 0) i = 0;
      }
      while(i<len)
      {
         if (e[i]==inValue)
            return i;
         i++;
      }
      return -1;
   }

   int lastIndexOf(ELEM_ inValue, Dynamic fromIndex = null())
   {
      int len = length;
      int i = fromIndex==null() ? len-1 : fromIndex->__ToInt();
      ELEM_ *e = (ELEM_ *)mBase;
      if (i >= len)
         i = len - 1;
      else if (i < 0)
         i += len;
      while(i>=0)
      {
         if (e[i]==inValue)
            return i;
         i--;
      }
      return -1;
   }

   NullType shift()
   {
      if (length==0) return null();
      ELEM_ result = __get(0);
      RemoveElement(0);
      return result;
   }


   Array<ELEM_> concat( Array<ELEM_> inTail );
   Array<ELEM_> copy( );
   Array<ELEM_> slice(int inPos, Dynamic end = null());
   Array<ELEM_> splice(int inPos, int len);
   Dynamic map(Dynamic inFunc);
   Array<ELEM_> filter(Dynamic inFunc);

   void insert(int inPos, ELEM_ inValue)
   {
		if (inPos<0)
		{
			inPos+=length;
			if (inPos<0) inPos = 0;
		}
		else if (inPos>length)
			inPos = length;
		hx::ArrayBase::Insert(inPos);
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

   // Will do random pointer sorting for object pointers
   inline void sortAscending()
   {
      ELEM_ *e = (ELEM_ *)mBase;
      std::sort(e, e+length);
   }
   static inline bool greaterThan(const ELEM_ &inA, const ELEM_ &inB) { return inB < inA; }
   inline void sortDescending()
   {
      ELEM_ *e = (ELEM_ *)mBase;
      std::sort(e, e+length, greaterThan);
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

   inline void qsort(Dynamic inSorter)
   {
      ELEM_ *e = (ELEM_ *)mBase;
      std::sort(e, e+length, Sorter(inSorter) );
   }

   void sort(Dynamic inSorter)
   {
      if ( ArrayTraits<ELEM_>::IsDynamic || ArrayTraits<ELEM_>::IsString)
      {
         // Keep references from being hidden inside sorters buffers
         safeSort(inSorter, ArrayTraits<ELEM_>::IsString);
      }
      else
      {
         ELEM_ *e = (ELEM_ *)mBase;
         std::stable_sort(e, e+length, Sorter(inSorter) );
      }
   }

   Dynamic iterator() { return new hx::ArrayIterator<ELEM_,ELEM_>(this); }

   template<typename TO>
   Dynamic iteratorFast() { return new hx::ArrayIterator<ELEM_,TO>(this); }

   virtual bool IsByteArray() const { return ArrayTraits<ELEM_>::IsByteArray; }

   // Dynamic interface
   virtual Dynamic __concat(const Dynamic &a0) { return concat(a0); }
   virtual Dynamic __copy() { return copy(); }
   virtual Dynamic __insert(const Dynamic &a0,const Dynamic &a1) { insert(a0,a1); return null(); }
   virtual Dynamic __iterator() { return iterator(); }
   virtual Dynamic __join(const Dynamic &a0) { return join(a0); }
   virtual Dynamic __pop() { return pop(); }
   virtual Dynamic __push(const Dynamic &a0) { return push(a0);}
   virtual Dynamic __remove(const Dynamic &a0) { return remove(a0); }
   virtual Dynamic __indexOf(const Dynamic &a0,const Dynamic &a1) { return indexOf(a0, a1); }
   virtual Dynamic __lastIndexOf(const Dynamic &a0,const Dynamic &a1) { return lastIndexOf(a0, a1); }
   virtual Dynamic __reverse() { reverse(); return null(); }
   virtual Dynamic __shift() { return shift(); }
   virtual Dynamic __slice(const Dynamic &a0,const Dynamic &a1) { return slice(a0,a1); }
   virtual Dynamic __splice(const Dynamic &a0,const Dynamic &a1) { return splice(a0,a1); }
   virtual Dynamic __sort(const Dynamic &a0) { sort(a0); return null(); }
   virtual Dynamic __toString() { return toString(); }
   virtual Dynamic __unshift(const Dynamic &a0) { unshift(a0); return null(); }
   virtual Dynamic __map(const Dynamic &func) { return map(func); }
   virtual Dynamic __filter(const Dynamic &func) { return filter(func); }
   virtual Dynamic __blit(const Dynamic &a0,const Dynamic &a1,const Dynamic &a2,const Dynamic &a3) { blit(a0,a1,a2,a3); return null(); }
   virtual Dynamic __memcmp(const Dynamic &a0) { return memcmp(a0); }

};



// --- Array ---------------------------------------------------------------
//
// The array class adds object syntax to the Array_obj pointer

template<typename ELEM_>
class Array : public hx::ObjectPtr< Array_obj<ELEM_> >
{
   typedef hx::ObjectPtr< Array_obj<ELEM_> > super;
   typedef Array_obj<ELEM_> OBJ_;

public:
   typedef Array_obj<ELEM_> *Ptr;
   using super::mPtr;
   using super::GetPtr;

   Array() { }
   Array(int inSize,int inReserve) : super( OBJ_::__new(inSize,inReserve) ) { }
   Array(const null &inNull) : super(0) { }
   Array(Ptr inPtr) : super(inPtr) { }

   #ifdef HXCPP_CHECK_POINTER
   inline OBJ_ *CheckGetPtr() const
   {
      if (!mPtr) hx::NullReference("Array", true);
      // The handler might have fixed up the null value
      if (!mPtr) hx::NullReference("Array", false);
      return mPtr;
   }
   #else
   inline OBJ_ *CheckGetPtr() const { return mPtr; }
   #endif

   // Construct from our type ...
   Array ( const hx::ObjectPtr< OBJ_  > &inArray )
        :  hx::ObjectPtr< OBJ_ >(inArray) { }

   Array(const Array<ELEM_> &inArray) : super(inArray.GetPtr()) { }

   // Build dynamic array from foreign array
   template<typename SOURCE_>
   Array( const Array<SOURCE_> &inRHS ) : super(0)
   {
      Array_obj<SOURCE_> *ptr = inRHS.GetPtr(); 
      if (ptr)
      {
         OBJ_ *arr = dynamic_cast<OBJ_ *>(ptr);
         if (!arr)
         {
            // Non-identical type (syntactically, should be creating from Array<Dynamic>)
            // Copy elements one-by-one
            // Not quite right, but is the best we can do...
            int n = ptr->__length();
            *this = Array_obj<ELEM_>::__new(n);
            for(int i=0;i<n;i++)
               mPtr->__unsafe_set(i,ptr->__GetItem(i));
         }
         else
            mPtr = arr;
      }
   }

   inline void setDynamic( const Dynamic &inRHS )
   {
      hx::Object *ptr = inRHS.GetPtr(); 
      if (ptr)
      {
         OBJ_ *arr = dynamic_cast<OBJ_ *>(ptr);
         if (!arr && ptr->__GetClass().mPtr == super::__SGetClass().mPtr )
         {
            // Non-identical type.
            // Copy elements one-by-one
            // Not quite right, but is the best we can do...
            int n = ptr->__length();
            *this = Array_obj<ELEM_>::__new(n);
            for(int i=0;i<n;i++)
               mPtr->__unsafe_set(i,ptr->__GetItem(i));
         }
         else
            mPtr = arr;
      }
   }

   Array( const Dynamic &inRHS ) : super(0) { setDynamic(inRHS); }
   Array( const cpp::ArrayBase &inRHS ) : super(0) { setDynamic(inRHS); }


   // operator= exact match...
   Array &operator=( Array<ELEM_> inRHS )
   {
      mPtr = inRHS.GetPtr();
      return *this;
   }

   // Foreign array
   template<typename OTHER>
   Array &operator=( const Array<OTHER> &inRHS )
   {
      *this = Array(inRHS);
      return *this;
   }

   Array &operator=( const Dynamic &inRHS )
   {
      setDynamic(inRHS);
      return *this;
   }

   Array &operator=( const cpp::ArrayBase &inRHS )
   {
      setDynamic(inRHS);
      return *this;
   }


   Array &operator=( const null &inNull )
   {
      mPtr = 0;
      return *this;
   }



   inline ELEM_ &operator[](int inIdx) { return CheckGetPtr()->Item(inIdx); }
   inline ELEM_ operator[](int inIdx) const { return CheckGetPtr()->__get(inIdx); }
   //inline ELEM_ __get(int inIdx) const { return CheckGetPtr()->__get(inIdx); }
   inline int __length() const { return CheckGetPtr()->__length(); }
   inline Array<ELEM_> &Add(const ELEM_ &inElem) { CheckGetPtr()->Add(inElem); return *this; }
   inline Array<ELEM_> & operator<<(const ELEM_ &inElem) { CheckGetPtr()->Add(inElem); return *this; }
};


// Now that the "Array" object is defined, we can implement this function ....

template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::__new(int inSize,int inReserve)
 { return  Array<ELEM_>(new Array_obj(inSize,inReserve)); }


template<>
inline bool Dynamic::IsClass<Array<Dynamic> >()
   { return mPtr && mPtr->__GetClass()== hx::ArrayBase::__mClass; }


template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::concat( Array<ELEM_> inTail )
{
   Array_obj *result = new Array_obj(inTail->__length()+(int)length,0);
   hx::ArrayBase::Concat(result,inTail->GetBase(),inTail->__length());
   return result;
}

template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::copy( )
{
   Array_obj *result = new Array_obj((int)length,0);
   memcpy(result->GetBase(),GetBase(),length*sizeof(ELEM_));
   return result;
}

// Copies the range of the array starting at pos up to, but not including, end.
// Both pos and end can be negative to count from the end: -1 is the last item in the array.
template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::slice(int inPos, Dynamic end)
{
   int e = end==null() ? length : end->__ToInt();
   Array_obj *result = new Array_obj(0,0);
   hx::ArrayBase::Slice(result,inPos,(int)e);
   return result;
}

template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::splice(int inPos, int len)
{
   Array_obj * result = new Array_obj(0,0);
   hx::ArrayBase::Splice(result,inPos,len);
   return result;
}

template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::filter(Dynamic inFunc)
{
   Array_obj *result = new Array_obj(0,0);
   for(int i=0;i<length;i++)
      if (inFunc(__unsafe_get(i)))
         result->push(__unsafe_get(i));
   return result;
}

template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::__SetSizeExact(int inLen)
{
   ArrayBase::__SetSizeExact(inLen);
   return this;
}

template<typename ELEM_>
Dynamic Array_obj<ELEM_>::map(Dynamic inFunc)
{
   Array_obj<Dynamic> *result = new Array_obj<Dynamic>(length,0);
   for(int i=0;i<length;i++)
      result->__unsafe_set(i,inFunc(__unsafe_get(i)));
   return result;
}





#endif
