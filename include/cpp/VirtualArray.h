namespace cpp
{

// This file is included twice - either side of the Array definition
#ifndef HX_VARRAY_DEFINED
#define HX_VARRAY_DEFINED


class VirtualArray : public hx::ObjectPtr<VirtualArray_obj>
{
   typedef hx::ObjectPtr<VirtualArray_obj> super;
public:
   typedef Dynamic Elem;

   inline VirtualArray() : super(0) { }
   inline VirtualArray(VirtualArray_obj *inObj) : super(inObj) { }
   inline VirtualArray(const null &inNull) : super(0) { }
   inline VirtualArray(const VirtualArray &inOther) : super( inOther.mPtr ) {  }

   // Build from foreign array
   template<typename SOURCE_> inline VirtualArray( const Array<SOURCE_> &inRHS );


   inline VirtualArray( const Dynamic &inRHS ) : super(0) { setDynamic(inRHS); }
   inline VirtualArray( const cpp::ArrayBase &inRHS ) : super(0) { setDynamic(inRHS); }
   inline VirtualArray(const ::cpp::Variant &inVariant) { setDynamic(inVariant.asObject()); }




   inline VirtualArray &operator=(const null &inNull) { mPtr = 0; return *this; }
   inline VirtualArray &operator=(Ptr inRHS) { mPtr = inRHS; return *this; }
   inline VirtualArray &operator=(const VirtualArray &inRHS) { mPtr = inRHS.mPtr; return *this; }

   inline void setDynamic( const Dynamic &inRHS );

   template<typename T>
   inline VirtualArray Add(const T &inVal);


   inline bool operator==(const Dynamic &value) const { return value==*this; }
   template<typename SOURCE_> inline bool operator==( const Array<SOURCE_> &inRHS );

   inline bool operator!=(Dynamic value) const { return value!=*this; }
   template<typename SOURCE_> inline bool operator!=( const Array<SOURCE_> &inRHS ) { return inRHS!=*this; }

};






class HXCPP_EXTERN_CLASS_ATTRIBUTES VirtualArray_obj : public hx::ArrayCommon
{
   typedef hx::ArrayStore ArrayStore;
   typedef hx::ArrayBase ArrayBase;

public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdVirtualArray };

   typedef hx::Object super;
   ArrayStore  store;
   ArrayBase   *base;

   VirtualArray_obj(ArrayBase *inBase=0, bool inFixed=false) : base(inBase)
   {
      mArrayConvertId = hx::aciVirtualArray;
      store = inFixed && inBase ? hx::arrayFixed : base ? base->getStoreType() : hx::arrayEmpty;
      #ifdef HXCPP_GC_GENERATIONAL
      if (base)
         HX_OBJ_WB_GET(this,base);
      #endif
   }

   VirtualArray_obj(ArrayStore inStore)
   {
      mArrayConvertId = hx::aciVirtualArray;
      store = inStore;
   }

   hx::Object *__GetRealObject() { return base?(hx::Object *)base:(hx::Object *)this; }

   inline static VirtualArray __new(int inSize=0,int inReserve=0)
   {
      VirtualArray result = new VirtualArray_obj(hx::arrayEmpty);
      if (inSize>0)
         result->__SetSizeExact(inSize);
      if (inReserve>0)
         result->reserve(inReserve);
      return result;
   }

   #if (HXCPP_API_LEVEL>330)
   int __Compare(const hx::Object *inRHS) const;
   #endif


   inline int get_length() const
   {
      return base ? base->length : 0;
   }

   inline void checkBase() const
   {
      #ifdef HXCPP_CHECK_POINTER
      if (store==hx::arrayNull)
      {
         hx::NullReference("Array", true);
         // The handler might have fixed up the null value
         if (store==hx::arrayNull) hx::NullReference("Array", false);
      }
      #endif
   }

   void EnsureStorage(const Dynamic &inValue)
   {
      if (!inValue.mPtr)
      {
         EnsureNullStorage();
      }
      else switch(inValue->__GetType())
      {
         case vtBool: EnsureBoolStorage(); break;
         case vtInt: EnsureIntStorage(); break;
         case vtFloat: EnsureFloatStorage(); break;
         case vtString: EnsureStringStorage(); break;
         default: EnsureObjectStorage();
      }
   }


   void EnsureStorage(const cpp::Variant &inValue)
   {
      switch(inValue.type)
      {
         case Variant::typeObject:
               if (!inValue.valObject)
                  EnsureNullStorage();
               else
                  EnsureObjectStorage();
               break;
         case Variant::typeString: EnsureStringStorage(); break;
         case Variant::typeDouble: EnsureFloatStorage(); break;
         case Variant::typeInt: EnsureIntStorage(); break;
         case Variant::typeBool: EnsureBoolStorage(); break;
         case Variant::typeInt64: EnsureObjectStorage(); break;
      }
   }


   void MakeIntArray();
   void MakeObjectArray();
   void MakeFloatArray();
   void MakeBoolArray();
   void MakeStringArray();

   void EnsureStorage(const VirtualArray &inValue) { EnsureObjectStorage(); }
   void EnsureStorage(const unsigned char &inValue) { EnsureIntStorage(); }
   void EnsureStorage(const bool &inValue) { EnsureBoolStorage(); }
   void EnsureStorage(const String &inValue) { EnsureStringStorage(); }
   void EnsureStorage(const double &inValue) { EnsureFloatStorage(); }
   void EnsureStorage(const float &inValue) { EnsureFloatStorage(); }
   void EnsureStorage(const int &inValue) { EnsureIntStorage(); }
   void EnsureStorage(const cpp::Int64 &inValue) { EnsureObjectStorage(); }
   void EnsureStorage(const cpp::UInt64 &inValue) { EnsureObjectStorage(); }
   void EnsureStorage(const null &inValue) { EnsureNullStorage(); }
   template<typename T>
   void EnsureStorage(const T &inValue) { EnsureObjectStorage(); }

   inline void EnsureBoolStorage()
   {
      switch(store)
      {
         case hx::arrayNull:
         case hx::arrayObject:
         case hx::arrayFixed:
         case hx::arrayBool:
            return;
         case hx::arrayEmpty:
            MakeBoolArray();
            break;
         case hx::arrayInt:
         case hx::arrayFloat:
         case hx::arrayString:
            MakeObjectArray();
            break;
      }
   }
   inline void EnsureStringStorage()
   {
      switch(store)
      {
         case hx::arrayNull:
         case hx::arrayObject:
         case hx::arrayFixed:
         case hx::arrayString:
            return;
         case hx::arrayEmpty:
            MakeStringArray();
            break;
         case hx::arrayInt:
         case hx::arrayFloat:
         case hx::arrayBool:
            MakeObjectArray();
            break;
      }
   }
   inline void EnsureFloatStorage()
   {
      switch(store)
      {
         case hx::arrayNull:
         case hx::arrayFloat:
         case hx::arrayObject:
         case hx::arrayFixed:
            return;
         case hx::arrayInt:
         case hx::arrayEmpty:
            MakeFloatArray();
            break;
         case hx::arrayBool:
         case hx::arrayString:
            MakeObjectArray();
            break;
      }
   }

   inline void EnsureIntStorage()
   {
      switch(store)
      {
         case hx::arrayNull:
         case hx::arrayInt:
         case hx::arrayFloat:
         case hx::arrayObject:
         case hx::arrayFixed:
            return;
         case hx::arrayEmpty:
            MakeIntArray();
            break;
         case hx::arrayBool:
         case hx::arrayString:
            MakeObjectArray();
            break;
      }
   }
   inline void EnsureObjectStorage()
   {
      switch(store)
      {
         case hx::arrayNull:
         case hx::arrayObject:
         case hx::arrayFixed:
            return;
         case hx::arrayEmpty:
         case hx::arrayInt:
         case hx::arrayFloat:
         case hx::arrayBool:
         case hx::arrayString:
            MakeObjectArray();
            break;
      }
   }
   inline void EnsureNullStorage()
   {
      switch(store)
      {
         case hx::arrayNull:
         case hx::arrayObject:
         case hx::arrayFixed:
         case hx::arrayString:
            return;
         case hx::arrayEmpty:
         case hx::arrayInt:
         case hx::arrayFloat:
         case hx::arrayBool:
            MakeObjectArray();
            break;
      }
   }

   template<typename F> void fixType();
   template<typename F> F castArray();

   void EnsureBase();
   void CreateEmptyArray(int inLen);

   void EnsureArrayStorage(ArrayStore inValue);
   void EnsureArrayStorage(VirtualArray inValue);

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_OBJECT(base);
   }
   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      if (base)
        __inCtx->visitObject( (hx::Object **)&base);
   }
   #endif


   // Used by cpp.ArrayBase
   inline int getElementSize() const { return base ? base->GetElementSize() : 0; }
   inline int getByteCount() const { return base ? base->getByteCount() : 0; }
   inline char * getBase() const { return base ? base->GetBase() : 0; }
   hx::Val __SetField(const String &inString,const hx::Val &inValue ,hx::PropertyAccess inCallProp) { return null(); }

   static hx::Class &__SGetClass() { return hx::ArrayBase::__mClass; }
   hx::Class __GetClass() const;
   String toString();
   String __ToString() const { return const_cast<VirtualArray_obj *>(this)->toString(); }

   void setData(void *inData, int inElements) { EnsureBase(); base->setData(inData, inElements); }
   void setUnmanagedData(void *inData, int inElements) { EnsureBase(); base->setUnmanagedData(inData, inElements); }

   int __GetType() const { return vtArray; }

   inline size_t size() const { checkBase(); return store==hx::arrayEmpty ? 0 : base->length; }
   inline int __length() const { checkBase(); return store==hx::arrayEmpty ? 0 : (int)base->length; }

   String ItemString(int inI) { checkBase(); return store==hx::arrayEmpty ? null() : base->ItemString(inI); }

   const char * __CStr() const { return store==hx::arrayEmpty ? "[]" : store==hx::arrayNull ? "null" : base->__CStr(); }
   inline const char *GetBase() const { return base ? base->GetBase() : 0; }
   inline char *GetBase() { return base ? base->GetBase() : 0; }

   int GetElementSize() const { checkBase(); return store==hx::arrayEmpty ? 0 : base->GetElementSize(); }

   inline void reserve(int inSize) const
   {
      if (base)
         base->reserve(inSize);
   }

   inline int capacity()
   {
      if (base)
         return base->capacity();
      return 0;
   }

   void resize(int inLen)
   {
      if (!base)
         CreateEmptyArray(inLen);
      else
         base->resize(inLen);
   }
   void __SetSize(int inLen) { resize(inLen); }

   VirtualArray __SetSizeExact(int inLen=0)
   {
      if (!base && inLen)
         CreateEmptyArray(inLen);
      else if (base)
         base->__SetSizeExact(inLen);
      return this;
   }

   void safeSort(Dynamic sorter, bool isString) { checkBase(); if (store!=hx::arrayEmpty) base->safeSort(sorter,isString); }

   inline void __unsafeStringReference(String inString) { if (base) base->__unsafeStringReference(inString); }


   Dynamic __GetItem(int inIndex) const;
   Dynamic __SetItem(int inIndex,Dynamic inValue);
   hx::Val __Field(const String &inString, hx::PropertyAccess inCallProp);

   template<typename T>
   inline const T &set(int inIdx, const T &inVal)
   {
      if (store!=hx::arrayFixed)
      {
         if (inIdx>(store==hx::arrayEmpty ? 0 : (int)base->length) )
            EnsureObjectStorage();
         else
            EnsureStorage(inVal);
      }
      base->set(inIdx, inVal);
      return inVal;
   }


   template<typename T>
   inline int push(const T &inVal)
   {
      if (store!=hx::arrayFixed) EnsureStorage(inVal);
      return base->__push(Dynamic(inVal));
   }


   template<typename T>
   inline VirtualArray_obj *Add(const T &inVal)
   {
      if (store!=hx::arrayFixed) EnsureStorage(inVal);
      base->__push(Dynamic(inVal));
      return this;
   }

   inline Dynamic pop() { checkBase(); return store==hx::arrayEmpty ? null() : base->__pop(); }

   inline bool contains(Dynamic inValue)
   {
      checkBase();
      if (store==hx::arrayEmpty)
         return false;
      EnsureStorage(inValue);
      return base->__contains(inValue);
   }

   inline bool remove(Dynamic inValue)
   {
      checkBase();
      if (store==hx::arrayEmpty)
         return false;
      EnsureStorage(inValue);
      return base->__remove(inValue);
   }

   inline bool removeAt(int inIndex) { checkBase(); return (store!=hx::arrayEmpty) && base->__removeAt(inIndex); }

   int indexOf(Dynamic inValue, Dynamic fromIndex = null())
   {
      checkBase();
      if (store==hx::arrayEmpty)
         return -1;
      EnsureStorage(inValue);
      return (int)base->__indexOf(inValue,fromIndex);
   }
   int lastIndexOf(Dynamic inValue, Dynamic fromIndex = null())
   {
      checkBase();
      if (store==hx::arrayEmpty)
         return -1;
      EnsureStorage(inValue);
      return (int)base->__lastIndexOf(inValue,fromIndex);
   }

   Dynamic shift() { checkBase(); return store==hx::arrayEmpty ? null() : base->__shift(); }

   VirtualArray concat( VirtualArray inTail )
   {
      inTail->checkBase();
      EnsureArrayStorage(inTail);
      if (inTail->__length()<1)
         return copy();
      return new VirtualArray_obj( base->__concat(inTail), store==hx::arrayFixed );
   }
   VirtualArray copy( )
   {
      checkBase();
      if (store==hx::arrayEmpty)
         return new VirtualArray_obj(hx::arrayEmpty);

      return new VirtualArray_obj(base->__copy(), store==hx::arrayFixed);
   }
   VirtualArray slice(int inPos, Dynamic end = null())
   {
      checkBase();
      if (store==hx::arrayEmpty)
         return new VirtualArray_obj(hx::arrayEmpty);
      return new VirtualArray_obj(base->__slice(inPos,end), store==hx::arrayFixed);
   }
   VirtualArray splice(int inPos, int len);
   VirtualArray map(Dynamic inFunc);
   VirtualArray filter(Dynamic inFunc);

   template<typename T>
   inline VirtualArray init(int inIndex, const T &inVal)
   {
      if (store!=hx::arrayFixed) EnsureStorage(inVal);
      __SetItem(inIndex,inVal);
      return this;
   } 

   inline Dynamic __unsafe_set(int inIndex, const Dynamic &val)  { return __SetItem(inIndex,val); } 
   inline Dynamic __unsafe_get(int inIndex)  { return __GetItem(inIndex); } 


   template<typename T>
   inline void insert(int inPos, const T &inValue)
   {
      if (store!=hx::arrayFixed)
      {
         if (inPos>(store==hx::arrayEmpty ? 0 : (int)base->length) )
            EnsureObjectStorage();
         else
            EnsureStorage(inValue);
      }

      base->__insert(inPos,inValue);
   }

   template<typename T>
   inline void unshift(const T& inValue)
   {
      if (store!=hx::arrayFixed) EnsureStorage(inValue);
      base->__unshift(inValue);
   }

   inline void reverse() { checkBase(); if (store!=hx::arrayEmpty) base->__reverse(); }

   inline void qsort(Dynamic inSorter) { checkBase(); if (base) base->__qsort(inSorter); }

   inline void sort(Dynamic inSorter) { checkBase(); if (base) base->__sort(inSorter); }

   Dynamic iterator() { checkBase(); return  !base ? getEmptyIterator() :  base->__iterator(); }
   static Dynamic getEmptyIterator();

   Dynamic keyValueIterator() { checkBase(); return  !base ? getEmptyIterator() :  base->__keyValueIterator(); }

   bool IsByteArray() const { checkBase(); return store!=hx::arrayEmpty && base->IsByteArray(); }

   void zero(Dynamic inFirst, Dynamic inCount) { checkBase(); if (store!=hx::arrayEmpty) base->zero(inFirst,inCount); }

   inline int memcmp(VirtualArray inOther)
   {
      checkBase();
      if (store==hx::arrayEmpty)
         return inOther->__length() == 0;
      return base->__memcmp(inOther);
   }
   inline void blit(int inDestElement,  cpp::VirtualArray inSourceArray, int inSourceElement, int inElementCount)
   {
      inSourceArray->checkBase();
      EnsureArrayStorage(inSourceArray);
      if (base)
         base->__blit(inDestElement, inSourceArray, inSourceElement, inElementCount);
   }

   String join(String inSeparator) { checkBase(); if (!base) return HX_CSTRING(""); return base->__join(inSeparator); }


   Dynamic __get(int inIndex) const { checkBase(); if (store==hx::arrayEmpty) return null(); return base->__GetItem(inIndex); }

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
};


//typedef hx::ObjectPtr< VirtualArray_obj > VirtualArray;



#else // !HX_VARRAY_DEFINED


// Build dynamic array from foreign array
template<typename SOURCE_>
VirtualArray::VirtualArray( const Array<SOURCE_> &inRHS )
   : super( new VirtualArray_obj( inRHS.mPtr, true) )
{
}


template<typename T>
inline VirtualArray VirtualArray::Add(const T &inVal)
{
   mPtr->push(inVal);
   return *this;
}


inline void VirtualArray::setDynamic( const Dynamic &inRHS )
{
   hx::Object *ptr = inRHS.GetPtr(); 
   if (ptr)
   {
      if (ptr->__GetClass().mPtr == super::__SGetClass().mPtr )
      {
         cpp::VirtualArray_obj *varray = dynamic_cast<cpp::VirtualArray_obj *>(ptr);
         if (varray)
            mPtr = varray;
         else
            mPtr = new VirtualArray_obj(dynamic_cast<cpp::ArrayBase_obj *>(ptr), true);
      }
   }
}



template<typename F>
void VirtualArray_obj::fixType()
{
   if (store==hx::arrayFixed)
      return;

   store = hx::arrayFixed;
   if (base && base->length>0)
   {
      Array<F> fixedArray = Dynamic(base);
      base = fixedArray.mPtr;
   }
   else
   {
      base = new Array_obj<F>(0,0);
   }
   #ifdef HXCPP_GC_GENERATIONAL
   if (base)
      HX_OBJ_WB_GET(this,base);
   #endif
}

template<typename ARRAY >
ARRAY VirtualArray_obj::castArray()
{
   if (store==hx::arrayFixed)
      return Dynamic(base);

   store = hx::arrayFixed;
   if (base && base->length>0)
   {
      ARRAY fixedArray = Dynamic(base);
      base = fixedArray.mPtr;
      #ifdef HXCPP_GC_GENERATIONAL
      if (base)
         HX_OBJ_WB_GET(this,base);
      #endif
      return fixedArray;
   }
   else
   {
      ARRAY fixedArray(0,0);
      base = fixedArray.mPtr;
      #ifdef HXCPP_GC_GENERATIONAL
      if (base)
         HX_OBJ_WB_GET(this,base);
      #endif
      return fixedArray;
   }
}

template<typename SOURCE_>
inline bool VirtualArray::operator==( const Array<SOURCE_> &inRHS )
{
   if (!mPtr)
      return inRHS.mPtr;
   return mPtr->castArray< Array<SOURCE_> >() == inRHS;
}

} // end namespace cpp

HXCPP_EXTERN_CLASS_ATTRIBUTES Dynamic _hx_reslove_virtual_array(cpp::VirtualArray inArray);



namespace hx
{
// For type inference when marking
template<> inline void MarkMember(cpp::VirtualArray &outT,hx::MarkContext *__inCtx)
  { HX_MARK_OBJECT(outT.mPtr); }

#ifdef HXCPP_VISIT_ALLOCS
template<> inline void VisitMember(cpp::VirtualArray &outT,hx::VisitContext *__inCtx)
{
   HX_VISIT_OBJECT(outT.mPtr);
}
#endif

} // end namespace hx

namespace cpp
{
#endif // HX_VARRAY_DEFINED
}

