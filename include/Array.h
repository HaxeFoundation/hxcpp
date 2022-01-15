#ifndef HX_ARRAY_H
#define HX_ARRAY_H
#include <cpp/FastIterator.h>

// --- hx::ReturnNull ------------------------------------------------------
//
// Provides an "Null<Object>" of given type.  For types that can't actually be null, Dynamic is used.

namespace hx
{

enum ArrayStore
{
   arrayNull = 0,
   arrayEmpty,
   arrayFixed,
   arrayBool,
   arrayInt,
   arrayFloat,
   arrayString,
   arrayObject,
};

enum ArrayConvertId
{
   aciAlwaysConvert = -4,
   aciVirtualArray = -3,
   aciStringArray  = -2,
   aciObjectArray  = -1,
   aciNotArray     = 0,
   aciPodBase      = 1,
};

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


template<typename T>
struct ArrayTraits { enum { StoreType = arrayObject }; };
template<> struct ArrayTraits<int> { enum { StoreType = arrayInt }; };
template<> struct ArrayTraits<float> { enum { StoreType = arrayFloat}; };
template<> struct ArrayTraits<double> { enum { StoreType = arrayFloat}; };
template<> struct ArrayTraits<Dynamic> { enum { StoreType = arrayObject }; };
template<> struct ArrayTraits<String> { enum { StoreType = arrayString }; };



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
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdArrayIterator };

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

// --- ArrayKeyValueIterator -------------------------------------------
template<typename FROM,typename TO>
class ArrayKeyValueIterator : public cpp::FastIterator_obj<Dynamic>
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdArrayIterator };

   ArrayKeyValueIterator(Array<FROM> inArray) : mArray(inArray), mIdx(0) { }

   bool hasNext()  { return mIdx < mArray->length; }

   inline TO toTo(const Dynamic &inD) { return inD.StaticCast<TO>(); }

   template<typename T>
   inline TO toTo(T inT) { return inT; }


   Dynamic next();

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

// Also used by cpp::VirtualArray
class HXCPP_EXTERN_CLASS_ATTRIBUTES ArrayCommon : public hx::Object
{
   protected:
      int mArrayConvertId;
   public:
      // Plain old data element size - or 0 if not plain-old-data
      int getArrayConvertId() const { return mArrayConvertId; }

      #if (HXCPP_API_LEVEL>330)
      virtual hx::Object *__GetRealObject() { return this; }
      #endif
};

// --- hx::ArrayBase ----------------------------------------------------
//
// Base class that treats array contents as a slab of bytes.
// The derived "Array_obj" adds strong typing to the "[]" operator

class HXCPP_EXTERN_CLASS_ATTRIBUTES ArrayBase : public ArrayCommon
{
public:
   ArrayBase(int inSize,int inReserve,int inElementSize,bool inAtomic);

   // Defined later so we can use "Array"
   static Array<Dynamic> __new(int inSize=0,int inReserve=0);


   static void __boot();

   typedef hx::Object super;

   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdArrayBase };

   // Used by cpp.ArrayBase
   inline int getElementSize() const { return GetElementSize(); }
   inline int getByteCount() const { return GetElementSize()*length; }
   inline char * getBase() const { return mBase; }


   hx::Val __SetField(const String &inString,const hx::Val &inValue ,hx::PropertyAccess inCallProp) { return null(); }

   static hx::Class __mClass;
   static hx::Class &__SGetClass() { return __mClass; }
   hx::Class __GetClass() const { return __mClass; }
   String toString();
   String __ToString() const;


   #if (HXCPP_API_LEVEL>330)
   int __Compare(const hx::Object *inRHS) const;
   #endif


   void setData(void *inData, int inElements)
   {
      mBase = (char *)inData;
      length = inElements;
      mAlloc = inElements;
      HX_OBJ_WB_PESSIMISTIC_GET(this);
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

   inline void resize(int inSize)
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
   inline void __SetSize(int inLen) { resize(inLen); }

   void __SetSizeExact(int inLen=0);
   
   Dynamic __unsafe_get(const Dynamic &i);
   Dynamic __unsafe_set(const Dynamic &i, const Dynamic &val);

   void safeSort(Dynamic sorter, bool isString);

   inline void __unsafeStringReference(String inString)
   {
      mBase = (char *)inString.raw_ptr();
      length = inString.length / GetElementSize();
      mAlloc = length;
      HX_OBJ_WB_PESSIMISTIC_GET(this);
   }

   
   virtual hx::ArrayStore getStoreType() const = 0;


   // Dynamic interface
   hx::Val __Field(const String &inString ,hx::PropertyAccess inCallProp);

   #if (HXCPP_API_LEVEL < 330)
   virtual Dynamic __concat(const Dynamic &a0) = 0;
   virtual Dynamic __copy() = 0;
   virtual Dynamic __insert(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual Dynamic __iterator() = 0;
   virtual Dynamic __keyValueIterator() = 0;
   virtual Dynamic __join(const Dynamic &a0) = 0;
   virtual Dynamic __pop() = 0;
   virtual Dynamic __push(const Dynamic &a0) = 0;
   virtual Dynamic __remove(const Dynamic &a0) = 0;
   virtual Dynamic __removeAt(const Dynamic &a0) = 0;
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
   inline Dynamic ____SetSize(const Dynamic &len)  { resize(len); return this; } 
   inline Dynamic ____SetSizeExact(const Dynamic &len)  { __SetSizeExact(len); return this; } 
   inline Dynamic ____unsafe_set(const Dynamic &i, const Dynamic &val)  { return __SetItem(i,val); } 
   inline Dynamic ____unsafe_get(const Dynamic &i)  { return __GetItem(i); } 
   virtual Dynamic __blit(const Dynamic &a0,const Dynamic &a1,const Dynamic &a2,const Dynamic &a3) = 0;
   inline Dynamic __zero(const Dynamic &a0,const Dynamic &a1)  { zero(a0,a1); return null(); }
   virtual Dynamic __memcmp(const Dynamic &a0) = 0;
   virtual void __qsort(Dynamic inCompare) = 0;
   virtual Dynamic __resize(const Dynamic &a0) = 0;

   #else
   inline void ____SetSize(int len)  { resize(len); } 
   inline void ____SetSizeExact(int len)  { __SetSizeExact(len); } 
   inline Dynamic ____unsafe_set(const Dynamic &i, const Dynamic &val)  { return __SetItem(i,val); } 
   inline Dynamic ____unsafe_get(const Dynamic &i)  { return __GetItem(i); } 

   virtual hx::ArrayBase *__concat(const cpp::VirtualArray &a0) = 0;
   virtual hx::ArrayBase *__copy() = 0;
   virtual void __insert(int inIndex,const Dynamic &a1) = 0;
   virtual Dynamic __iterator() = 0;
   virtual Dynamic __keyValueIterator() = 0;
   virtual ::String __join(::String a0) = 0;
   virtual Dynamic __pop() = 0;
   virtual int __push(const Dynamic &a0) = 0;
   virtual bool __contains(const Dynamic &a0) = 0;
   virtual bool __remove(const Dynamic &a0) = 0;
   virtual bool __removeAt(int inIndex) = 0;
   virtual int __indexOf(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual int __lastIndexOf(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual void __reverse() = 0;
   virtual Dynamic __shift() = 0;
   virtual hx::ArrayBase *__slice(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual hx::ArrayBase *__splice(const Dynamic &a0,const Dynamic &a1) = 0;
   virtual void __sort(const Dynamic &a0) = 0;
   virtual ::String __toString() = 0;
   virtual void  __unshift(const Dynamic &a0) = 0;
   virtual cpp::VirtualArray_obj *__map(const Dynamic &func) = 0;
   virtual hx::ArrayBase *__filter(const Dynamic &func) = 0;
   virtual void __blit(int inDestElement,const cpp::VirtualArray &inSourceArray,int inSourceElement,int inElementCount) = 0;
   virtual int __memcmp(const cpp::VirtualArray &a0) = 0;
   inline void __zero(const Dynamic &a0,const Dynamic &a1)  { zero(a0,a1); }
   virtual void __qsort(Dynamic inCompare) = 0;
   virtual void __resize(int inLen) = 0;

   virtual void set(int inIdx, const cpp::Variant &inValue) = 0;
   virtual void setUnsafe(int inIdx, const cpp::Variant &inValue) = 0;
   #endif

   Dynamic concat_dyn();
   Dynamic copy_dyn();
   Dynamic insert_dyn();
   Dynamic iterator_dyn();
   Dynamic keyValueIterator_dyn();
   Dynamic join_dyn();
   Dynamic pop_dyn();
   Dynamic push_dyn();
   Dynamic contains_dyn();
   Dynamic remove_dyn();
   Dynamic removeAt_dyn();
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
   Dynamic resize_dyn();

   void Realloc(int inLen) const;

   inline void EnsureSize(int inLen) const
   {
      if (inLen>length)
      {
         if (inLen>mAlloc)
            Realloc(inLen);
         length = inLen;
      }
   }

   void RemoveElement(int inIndex);


   void Insert(int inPos);

   void Splice(hx::ArrayBase *outResult,int inPos,int inLen);

   void Slice(hx::ArrayBase *outResult,int inPos,int inEnd);

   void Concat(hx::ArrayBase *outResult,const char *inEnd, int inLen);


   void reserve(int inN) const;

   inline int capacity() const { return mAlloc; }

   // Set numeric values to 0, pointers to null, bools to false
   void zero(Dynamic inFirst, Dynamic inCount);

   int Memcmp(ArrayBase *inArray);

   // Copy section of other array.
   void Blit(int inDestElement, ArrayBase *inSourceArray, int inSourceElement, int inElementCount);

   static String joinArray(hx::ArrayBase *inBase, String inSeparator);
   static String joinArray(Array_obj<String> *inArray, String inSeparator);

   virtual bool AllocAtomic() const { return false; }

   inline bool IsByteArray() const { return getStoreType()==arrayBool; }


   inline Dynamic __get(int inIndex) const { return __GetItem(inIndex); }

   // Plain old data element size - or 0 if not plain-old-data
   int getArrayConvertId() const { return mArrayConvertId; }

   mutable int length;

   static inline int baseOffset() { return (int)offsetof(ArrayBase,mBase); }
   static inline int allocOffset() { return (int)offsetof(ArrayBase,mAlloc); }
   static inline int lengthOffset() { return (int)offsetof(ArrayBase,length); }

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


#if (HXCPP_API_LEVEL>=330)
#include "cpp/VirtualArray.h"
#endif




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
template<> inline bool TypeContainsPointers(short *) { return false; }
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


bool DynamicEq(const Dynamic &a, const Dynamic &b);

}

template<typename T> struct ArrayClassId { enum { id=hx::clsIdArrayObject }; };
template<> struct ArrayClassId<unsigned char> { enum { id=hx::clsIdArrayByte }; };
template<> struct ArrayClassId<signed char> { enum { id=hx::clsIdArrayByte }; };
template<> struct ArrayClassId<unsigned short> { enum { id=hx::clsIdArrayShort }; };
template<> struct ArrayClassId<signed short> { enum { id=hx::clsIdArrayShort }; };
template<> struct ArrayClassId<unsigned int> { enum { id=hx::clsIdArrayInt }; };
template<> struct ArrayClassId<signed int> { enum { id=hx::clsIdArrayInt }; };
template<> struct ArrayClassId<float> { enum { id=hx::clsIdArrayFloat32 }; };
template<> struct ArrayClassId<double> { enum { id=hx::clsIdArrayFloat64 }; };
template<> struct ArrayClassId<String> { enum { id=hx::clsIdArrayString }; };

// sort...
#include <algorithm>

namespace hx
{
template<typename T>
inline bool arrayElemEq(const T &a, const T &b) { return a==b; }

template<>
inline bool arrayElemEq<Dynamic>(const Dynamic &a, const Dynamic &b) {
   return hx::DynamicEq(a,b);
}
}


template<typename ELEM_>
class Array_obj : public hx::ArrayBase
{
   typedef ELEM_ Elem;
   typedef hx::ObjectPtr< Array_obj<ELEM_> > ObjPtr;
   typedef typename hx::ReturnNull<ELEM_>::type NullType;

public:
   enum { _hx_ClassId = ArrayClassId<ELEM_>::id };


   Array_obj(int inSize,int inReserve) :
        hx::ArrayBase(inSize,inReserve,sizeof(ELEM_),!hx::ContainsPointers<ELEM_>()) { }


   // Defined later so we can use "Array"
   static Array<ELEM_> __new(int inSize=0,int inReserve=0);
   static Array<ELEM_> __newConstWrapper(ELEM_ *inData,int inSize);
   static Array<ELEM_> fromData(const ELEM_ *inData,int inCount);


#if (HXCPP_API_LEVEL>331)
   bool _hx_isInstanceOf(int inClassId)
   {
      return inClassId==1 || inClassId==(int)hx::clsIdArrayBase || inClassId==(int)_hx_ClassId;
   }
#endif

   virtual bool AllocAtomic() const { return !hx::ContainsPointers<ELEM_>(); }

   virtual Dynamic __GetItem(int inIndex) const { return __get(inIndex); }
   virtual Dynamic __SetItem(int inIndex,Dynamic inValue)
   {
      ELEM_ &elem = Item(inIndex);
      elem = inValue;
      if (hx::ContainsPointers<ELEM_>()) { HX_OBJ_WB_GET(this,hx::PointerOf(elem)); }
      return inValue;
   }

   inline ELEM_ *Pointer() { return (ELEM_ *)mBase; }

   inline ELEM_ &Item(int inIndex)
   {
      if (inIndex>=(int)length) EnsureSize(inIndex+1);
      else if (inIndex<0) { return * hx::NewNull<ELEM_>(); }
      return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_));
   }
   inline ELEM_ __get(int inIndex) const
   {
      if ((unsigned int)inIndex>=(unsigned int)length ) return null();
      return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_));
   }

   // Does not check for size valid - use with care
   inline ELEM_ &__unsafe_get(int inIndex) { return * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_)); }


   inline ELEM_ & __unsafe_set(int inIndex, ELEM_ inValue)
   {
      ELEM_ &elem = *(ELEM_*)(mBase + inIndex*sizeof(ELEM_));
      elem = inValue;
      if (hx::ContainsPointers<ELEM_>()) { HX_OBJ_WB_GET(this, hx::PointerOf(elem)); }
      return elem;
   }


   inline int memcmp(Array<ELEM_> inOther)
   {
      return ArrayBase::Memcmp(inOther.GetPtr());
   }


   inline void memcpy(int inStart, const ELEM_ *inData, int inElements)
   {
      EnsureSize(inStart+inElements);
      int s = GetElementSize();
      ::memcpy(mBase + s*inStart, inData, s*inElements);
      if (hx::ContainsPointers<ELEM_>())
      {
         HX_OBJ_WB_PESSIMISTIC_GET(this);
      }
   }


   inline void blit(int inDestElement,  Array<ELEM_> inSourceArray,
                    int inSourceElement, int inElementCount)
   {
      ArrayBase::Blit(inDestElement, inSourceArray.GetPtr(), inSourceElement, inElementCount);
   }


   void __Mark(hx::MarkContext *__inCtx)
   {
      if (mAlloc>0) hx::MarkAlloc((void *)mBase, __inCtx );
      if (length && hx::ContainsPointers<ELEM_>())
      {
         ELEM_ *ptr = (ELEM_ *)mBase;
         HX_MARK_MEMBER_ARRAY(ptr,length);
      }
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      if (mAlloc>0) __inCtx->visitAlloc((void **)&mBase);
      if (hx::ContainsPointers<ELEM_>())
      {
         ELEM_ *ptr = (ELEM_ *)mBase;
         for(int i=0;i<length;i++)
         {
            HX_VISIT_MEMBER(ptr[i]);
         }
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

   Array<ELEM_> init(int inIndex, ELEM_ inValue)
   {
      * (ELEM_ *)(mBase + inIndex*sizeof(ELEM_)) = inValue;
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM_>())
         { HX_OBJ_WB_GET(this, hx::PointerOf(inValue)); }
      #endif
      return this;
   }


   #ifdef HXCPP_GC_GENERATIONAL
   inline int pushCtx(hx::StackContext *_hx_ctx, ELEM_ inVal )
   {
      int l = length;
      EnsureSize((int)l+1);
      * (ELEM_ *)(mBase + l*sizeof(ELEM_)) = inVal;
      if (hx::ContainsPointers<ELEM_>()) { HX_ARRAY_WB(this,inIdx, hx::PointerOf(inVal) ); }
      return length;
   }
   #endif


   // Haxe API
   inline int push( ELEM_ inVal )
   {
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM_>())
         return pushCtx(HX_CTX_GET,inVal);
      #endif
      int l = length;
      EnsureSize((int)l+1);
      * (ELEM_ *)(mBase + l*sizeof(ELEM_)) = inVal;
      return length;
   }
   inline NullType pop( )
   {
      if (!length) return null();
      ELEM_ result = __get((int)length-1);
      resize((int)length-1);
      return result;
   }



   int Find(ELEM_ inValue)
   {
      ELEM_ *e = (ELEM_ *)mBase;
      for(int i=0;i<length;i++)
         if (hx::arrayElemEq(e[i],inValue))
            return i;
      return -1;
   }

   bool contains(ELEM_ inValue)
   {
      ELEM_ *e = (ELEM_ *)mBase;
      for(int i=0;i<length;i++)
      {
         if (hx::arrayElemEq(e[i],inValue))
            return true;
      }
      return false;
   }

   bool remove(ELEM_ inValue)
   {
      ELEM_ *e = (ELEM_ *)mBase;
      for(int i=0;i<length;i++)
      {
         if (hx::arrayElemEq(e[i],inValue))
         {
            RemoveElement((int)i);
            return true;
         }
      }
      return false;
   }

   bool removeAt( int idx )
   { 
      if( idx < 0 ) idx += length; 
      if (idx>=length || idx<0) return false; 
      RemoveElement(idx); 
      return true; 
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
         if (hx::arrayElemEq(e[i],inValue))
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
         if (hx::arrayElemEq(e[i],inValue))
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

   String join(String inSeparator) { return ArrayBase::joinArray(this, inSeparator); }

   Array<ELEM_> concat( Array<ELEM_> inTail );
   Array<ELEM_> copy( );
   Array<ELEM_> slice(int inPos, Dynamic end = null());
   Array<ELEM_> splice(int inPos, int len);
   inline void removeRange(int inPos, int len)
   {
      hx::ArrayBase::Splice(0,inPos,len);
   }

   #if HXCPP_API_LEVEL>=330
   cpp::VirtualArray map(Dynamic inFunc);
   #else
   Dynamic map(Dynamic inFunc);
   #endif
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
      #ifdef HXCPP_GC_GENERATIONAL
      if (hx::ContainsPointers<ELEM_>())
         { HX_OBJ_WB_GET(this,hx::PointerOf(inValue)); }
      #endif
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
      if ( (int)hx::ArrayTraits<ELEM_>::StoreType==(int)hx::arrayObject ||
          (int)hx::ArrayTraits<ELEM_>::StoreType==(int)hx::arrayString)
      {
         // Keep references from being hidden inside sorters buffers
         safeSort(inSorter, (int)hx::ArrayTraits<ELEM_>::StoreType==(int)hx::arrayString);
      }
      else
      {
         ELEM_ *e = (ELEM_ *)mBase;
         std::stable_sort(e, e+length, Sorter(inSorter) );
      }
   }

   Dynamic iterator() { return new hx::ArrayIterator<ELEM_,ELEM_>(this); }
   Dynamic keyValueIterator() { return new hx::ArrayKeyValueIterator<ELEM_,ELEM_>(this); }

   template<typename TO>
   Dynamic iteratorFast() { return new hx::ArrayIterator<ELEM_,TO>(this); }

   template<typename TO>
   Dynamic keyValueIteratorFast() { return new hx::ArrayKeyValueIterator<ELEM_,TO>(this); }
   
   virtual hx::ArrayStore getStoreType() const
   {
      return (hx::ArrayStore) hx::ArrayTraits<ELEM_>::StoreType;
   }

   inline ELEM_ &setCtx(hx::StackContext *_hx_ctx, int inIdx, ELEM_ inValue)
   {
      ELEM_ &elem = Item(inIdx);
      HX_ARRAY_WB(this,inIdx, hx::PointerOf(inValue) );
      return elem = inValue;
   }


   // Dynamic interface
   #if (HXCPP_API_LEVEL < 330)
   virtual Dynamic __concat(const Dynamic &a0) { return concat(a0); }
   virtual Dynamic __copy() { return copy(); }
   virtual Dynamic __insert(const Dynamic &a0,const Dynamic &a1) { insert(a0,a1); return null(); }
   virtual Dynamic __iterator() { return iterator(); }
   virtual Dynamic __keyValueIterator() { return keyValueIterator(); }
   virtual Dynamic __join(const Dynamic &a0) { return join(a0); }
   virtual Dynamic __pop() { return pop(); }
   virtual Dynamic __push(const Dynamic &a0) { return push(a0);}
   virtual Dynamic __remove(const Dynamic &a0) { return remove(a0); }
   virtual Dynamic __removeAt(const Dynamic &a0) { return removeAt(a0); }
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
   virtual Dynamic __resize(const Dynamic &a0) { resize(a0); return null(); }
   virtual void __qsort(Dynamic inCompare) { this->qsort(inCompare); };
   #else //(HXCPP_API_LEVEL < 330)

   virtual hx::ArrayBase *__concat(const cpp::VirtualArray &a0) { return concat(a0).mPtr; }
   virtual hx::ArrayBase *__copy() { return copy().mPtr; }
   virtual void __insert(int inIndex,const Dynamic &a1) { insert(inIndex,a1);}
   virtual Dynamic __iterator() { return iterator(); }
   virtual Dynamic __keyValueIterator() { return keyValueIterator(); }
   virtual ::String __join(::String a0) { return join(a0); }
   virtual Dynamic __pop() { return pop(); }
   virtual int __push(const Dynamic &a0) { return push(a0);}
   virtual bool __contains(const Dynamic &a0) { return contains(a0); }
   virtual bool __remove(const Dynamic &a0) { return remove(a0); }
   virtual bool __removeAt(int inIndex) { return removeAt(inIndex); }
   virtual int __indexOf(const Dynamic &a0,const Dynamic &a1) { return indexOf(a0, a1); }
   virtual int __lastIndexOf(const Dynamic &a0,const Dynamic &a1) { return lastIndexOf(a0, a1); }
   virtual void __reverse() { reverse(); }
   virtual Dynamic __shift() { return shift(); }
   virtual hx::ArrayBase *__slice(const Dynamic &a0,const Dynamic &a1) { return slice(a0,a1).mPtr; }
   virtual hx::ArrayBase *__splice(const Dynamic &a0,const Dynamic &a1) { return splice(a0,a1).mPtr; }
   virtual void __sort(const Dynamic &a0) { sort(a0); }
   virtual ::String __toString() { return toString(); }
   virtual void  __unshift(const Dynamic &a0) { unshift(a0); }
   virtual cpp::VirtualArray_obj *__map(const Dynamic &func) { return map(func).mPtr; }
   virtual void __resize(int inLen) { resize(inLen); }

   virtual hx::ArrayBase *__filter(const Dynamic &func) { return filter(func).mPtr; }
   virtual void __blit(int inDestElement,const cpp::VirtualArray &inSourceArray,int inSourceElement,int inElementCount)
   {
      blit(inDestElement,inSourceArray,inSourceElement,inElementCount);
   }
   virtual int __memcmp(const cpp::VirtualArray &a0) { return memcmp(a0); }
   virtual void __qsort(Dynamic inCompare) { this->qsort(inCompare); };

   virtual void set(int inIndex, const cpp::Variant &inValue) {
      ELEM_ &elem = Item(inIndex);
      elem = ELEM_(inValue);
      if (hx::ContainsPointers<ELEM_>()) {
         HX_OBJ_WB_GET(this, hx::PointerOf(elem));
      }
   }
   virtual void setUnsafe(int inIndex, const cpp::Variant &inValue) {
      ELEM_ &elem = *(ELEM_ *)(mBase + inIndex*sizeof(ELEM_));
      elem = ELEM_(inValue);
      if (hx::ContainsPointers<ELEM_>()) { HX_OBJ_WB_GET(this,hx::PointerOf(elem)); }
   }


   #endif
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
   typedef ELEM_ Elem;
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

   #ifdef HX_VARRAY_DEFINED
   // From VirtualArray
   Array( const cpp::VirtualArray &inVArray) { fromVArray(inVArray.mPtr); }

   void fromVArray(cpp::VirtualArray_obj *inVArray)
   {
      if (!inVArray || inVArray->store==hx::arrayNull)
      {
         mPtr = 0;
         return;
      }
      inVArray->fixType<ELEM_>();
      // Switch on type?
      setDynamic(inVArray->base,true);
   }

   Array &operator=( const cpp::VirtualArray &inRHS )
   {
      fromVArray(inRHS.mPtr);
      return *this;
   }

   #endif

   inline void setDynamic( const Dynamic &inRHS, bool inIgnoreVirtualArray=false )
   {
      hx::Object *ptr = inRHS.GetPtr(); 
      if (ptr)
      {
         OBJ_ *arr = dynamic_cast<OBJ_ *>(ptr);
         if (!arr && ptr->__GetClass().mPtr == super::__SGetClass().mPtr )
         {
            #ifdef HX_VARRAY_DEFINED
            cpp::VirtualArray_obj *varray = inIgnoreVirtualArray ? 0 :
                                            dynamic_cast<cpp::VirtualArray_obj *>(ptr);
            if (varray)
               fromVArray(varray);
            else
            #endif
            {
               // Non-identical type.
               // Copy elements one-by-one
               // Not quite right, but is the best we can do...
               int n = ptr->__length();
               *this = Array_obj<ELEM_>::__new(n);
               for(int i=0;i<n;i++)
                  mPtr->__unsafe_set(i,ptr->__GetItem(i));
            }
         }
         else
            mPtr = arr;
      }
   }

   Array( const Dynamic &inRHS ) : super(0) { setDynamic(inRHS); }
   Array( const cpp::ArrayBase &inRHS ) : super(0) { setDynamic(inRHS); }
   inline Array(const ::cpp::Variant &inVariant) : super(0)
   {
      setDynamic(inVariant.asObject());
   }

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


   Array &operator=( const cpp::Variant &inRHS )
   {
      if (inRHS.type!=cpp::Variant::typeObject)
         setDynamic( null() );
      else
         setDynamic(inRHS.valObject);
      return *this;
   }


   Array &operator=( const null &inNull )
   {
      mPtr = 0;
      return *this;
   }


   #if (HXCPP_API_LEVEL >= 330)
   inline bool operator==(const cpp::VirtualArray &varray) const { return varray==*this; }
   inline bool operator!=(const cpp::VirtualArray &varray) const { return varray!=*this; }
   #endif


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


template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::__newConstWrapper(ELEM_ *inData,int inSize)
{
   Array_obj<ELEM_> temp(0,0);
   Array_obj<ELEM_> *result = (Array_obj<ELEM_> *)hx::InternalCreateConstBuffer(&temp,sizeof(temp));
   result->setUnmanagedData(inData, inSize);
   return result;
}


template<typename ELEM_>
Array<ELEM_> Array_obj<ELEM_>::fromData(const ELEM_ *inData,int inCount)
{
   Array<ELEM_> result = new Array_obj(inCount,inCount);
   if (inCount)
       result->memcpy(0, inData, inCount);
   return result;
}



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
   ::memcpy(result->GetBase(),GetBase(),length*sizeof(ELEM_));
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

// Static externs 
template<typename ARRAY>
inline ARRAY _hx_array_set_size_exact(ARRAY inArray, int inLen)
{
   return inArray->__SetSizeExact(inLen);
}

template<typename ARRAY1,typename ARRAY2>
inline int _hx_array_memcmp(ARRAY1 inArray1, ARRAY2 inArray2)
{
   return inArray1->memcmp(inArray2);
}

template<typename ARRAY,typename VALUE>
inline typename ARRAY::Elem _hx_array_unsafe_set(ARRAY inArray, int inIndex, VALUE inValue)
{
   return inArray->__unsafe_set(inIndex, inValue);
}


template<typename ARRAY>
inline typename ARRAY::Elem _hx_array_unsafe_get(ARRAY inArray, int inIndex)
{
   return inArray->__unsafe_get(inIndex);
}



// Include again, for functions that required Array definition
#ifdef HX_VARRAY_DEFINED
#include "cpp/VirtualArray.h"
#endif


#if HXCPP_API_LEVEL >= 330
template<typename ELEM_>
cpp::VirtualArray Array_obj<ELEM_>::map(Dynamic inFunc)
{
   cpp::VirtualArray result = cpp::VirtualArray_obj::__new(length,0);
   for(int i=0;i<length;i++)
      result->__unsafe_set(i,inFunc(__unsafe_get(i)));
   return result;
}

#else
template<typename ELEM_>
Dynamic Array_obj<ELEM_>::map(Dynamic inFunc)
{
   Array_obj<Dynamic> *result = new Array_obj<Dynamic>(length,0);
   for(int i=0;i<length;i++)
      result->__unsafe_set(i,inFunc(__unsafe_get(i)));
   return result;
}
#endif



#endif
