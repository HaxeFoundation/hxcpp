#include <hxcpp.h>
#include <vector>
#include <cpp/Pointer.h>

#ifdef HXCPP_TELEMETRY
extern void __hxt_new_array(void* obj, int size);
#endif

using namespace hx;


// -------- ArrayBase -------------------------------------

namespace hx
{

Array<Dynamic> ArrayBase::__new(int inSize,int inReserve)
 { return  Array<Dynamic>(new Array_obj<Dynamic>(inSize,inReserve)); }

ArrayBase::ArrayBase(int inSize,int inReserve,int inElementSize,bool inAtomic)
{
   length = inSize;
   int alloc = inSize < inReserve ? inReserve : inSize;
   if (alloc)
   {
      mBase = (char *)hx::InternalNew(alloc * inElementSize,false);
      HX_OBJ_WB_GET(this,mBase);
#ifdef HXCPP_TELEMETRY
      __hxt_new_array(mBase, alloc * inElementSize);
#endif
   }
   else
      mBase = 0;
   mAlloc = alloc;
   mArrayConvertId = inAtomic ? inElementSize :
               inElementSize==sizeof(String) ? aciStringArray : aciObjectArray;
}



void ArrayBase::reserve(int inSize) const
{
   if (mAlloc<inSize)
   {
      int elemSize = GetElementSize();
      int bytes = inSize * elemSize;

      if (mBase)
      {
         bool wasUnamanaged = mAlloc<0;
         if (wasUnamanaged)
         {
            char *base=(char *)hx::InternalNew(bytes,false);
            memcpy(base,mBase,length*elemSize);
            mBase = base;
         }
         else
            mBase = (char *)hx::InternalRealloc(length*elemSize,mBase, bytes );
      }
      else
      {
         mBase = (char *)hx::InternalNew(bytes,false);
         #ifdef HXCPP_TELEMETRY
         __hxt_new_array(mBase, bytes);
         #endif
      }

      mAlloc = inSize;
      HX_OBJ_WB_GET(const_cast<ArrayBase *>(this),mBase);
   }
}


void ArrayBase::Realloc(int inSize) const
{
   // Try to detect "push" case vs resizing to big array size explicitly by looking at gap
   bool pushCase = (inSize<=mAlloc + 16);
   if (!pushCase)
   {
      reserve(inSize);
   }
   else if (pushCase)
   {
      int newAlloc = inSize;
      unsigned int elemSize = GetElementSize();
      unsigned int minBytes = inSize*elemSize + 8;
      unsigned int roundup = 64;
      while(roundup<minBytes)
         roundup<<=1;

      if (roundup>64)
      {
         int half = 3*(roundup>>2);
         if (minBytes<half)
            roundup = half;
      }
      unsigned int bytes = roundup-8;

      if (mBase)
      {
         bool wasUnamanaged = mAlloc<0;
         if (wasUnamanaged)
         {
            char *base=(char *)hx::InternalNew(bytes,false);
            memcpy(base,mBase,length*elemSize);
            mBase = base;
         }
         else
         {
            mBase = (char *)hx::InternalRealloc(length*elemSize,mBase, bytes, true);
            int o = bytes;
            bytes = hx::ObjectSizeSafe(mBase);
         }
      }
      else
      {
         mBase = (char *)hx::InternalNew(bytes,false);
         #ifdef HXCPP_TELEMETRY
         __hxt_new_array(mBase, bytes);
         #endif
      }

      mAlloc = bytes/elemSize;
      HX_OBJ_WB_GET(const_cast<ArrayBase *>(this),mBase);
   }
}

// Set numeric values to 0, pointers to null, bools to false
void ArrayBase::zero(Dynamic inFirst, Dynamic inCount)
{
   int first = inFirst==null() ? 0 : inFirst->__ToInt();
   if (first<0)
      first+=length;
   if (first<0 || first>=length)
      return;

   int count = length;
   if (inCount!=null())
      count = inCount->__ToInt();
   if (count<0)
      count += length;
   if (count<0)
      return;

   if (first+count > length)
      count = length - first;

   int size = GetElementSize();
   memset(mBase + first*size, 0, count*size);
}

int ArrayBase::Memcmp(ArrayBase *inOther)
{
   int bytesA = length * GetElementSize();
   int bytesB = inOther->length * inOther->GetElementSize();
   int common = bytesA<bytesB ? bytesA : bytesB;
   int result = memcmp(mBase, inOther->mBase, common);
   if (result)
      return result;
   return bytesA - bytesB;
}

void ArrayBase::Blit(int inDestElement, ArrayBase *inSourceArray, int inSourceElement, int inElementCount)
{
   int srcSize = inSourceArray->GetElementSize();
   int srcElems = inSourceArray->length;
   if (inDestElement<0 || inSourceElement<0 || inSourceElement+inElementCount>srcElems)
      hx::Throw( HX_CSTRING("blit out of bounds") );
   if (srcSize!=GetElementSize())
      hx::Throw( HX_CSTRING("blit array mismatch") );

   int newSize = inDestElement + inElementCount;
   if (newSize>length)
      resize(newSize);

   const char *src = inSourceArray->mBase + inSourceElement*srcSize;
   char *dest = mBase + inDestElement*srcSize;
   int len = inElementCount*srcSize;
   if (src+len < dest || dest+len<src)
      memcpy(dest,src,len);
   else
      memmove(dest,src,len);

   HX_OBJ_WB_PESSIMISTIC_GET(this);
}


#if (HXCPP_API_LEVEL>330)
int ArrayBase::__Compare(const hx::Object *inRHS) const
{
   if (inRHS==this)
      return 0;
   if (inRHS->__GetType()!=vtArray)
      return -1;
   ArrayCommon *common = (ArrayCommon *)inRHS;
   hx::Object *implementation = common->__GetRealObject();
   return implementation<this ? -1 : implementation!=this;
}
#endif


String ArrayBase::__ToString() const { return HX_CSTRING("Array"); }
String ArrayBase::toString()
{
   // Byte-array (not bool!)
   if (IsByteArray())
   {
      return String( (const char *) mBase, length);
   }

   return HX_CSTRING("[") + __join(HX_CSTRING(",")) + HX_CSTRING("]");
}


void ArrayBase::__SetSizeExact(int inSize)
{
   if (inSize==0)
   {
      InternalReleaseMem(mBase);
      mBase = 0;
      mAlloc = length = 0;
   }
   else if (inSize!=length || inSize!=mAlloc)
   {
      int elemSize = GetElementSize();
      int bytes = inSize * elemSize;
      if (mBase)
      {
         bool wasUnamanaged = mAlloc<0;

         if (wasUnamanaged)
         {
            char *base=(char *)(AllocAtomic() ? hx::NewGCPrivate(0,bytes) : hx::NewGCBytes(0,bytes));
            memcpy(base,mBase,std::min(length,inSize)*elemSize);
            mBase = base;
         }
         else
            mBase = (char *)hx::InternalRealloc(length*elemSize,mBase, bytes );
      }
      else if (AllocAtomic())
      {
         mBase = (char *)hx::NewGCPrivate(0,bytes);
#ifdef HXCPP_TELEMETRY
         __hxt_new_array(mBase, bytes);
#endif
      }
      else
      {
         mBase = (char *)hx::NewGCBytes(0,bytes);
#ifdef HXCPP_TELEMETRY
         __hxt_new_array(mBase, bytes);
#endif
      }
      mAlloc = length = inSize;
      HX_OBJ_WB_GET(this,mBase);
   }
}


Dynamic ArrayBase::__unsafe_get(const Dynamic &i)
{
   return __GetItem(i);
}


Dynamic ArrayBase::__unsafe_set(const Dynamic &i, const Dynamic &val)
{
   return __SetItem(i,val);
}



void ArrayBase::Insert(int inPos)
{
   if (inPos>=length)
      resize(length+1);
   else
   {
      resize(length+1);
      int s = GetElementSize();
      memmove(mBase + inPos*s + s, mBase+inPos*s, (length-inPos-1)*s );
   }
}

void ArrayBase::Splice(ArrayBase *outResult,int inPos,int inLen)
{
   if (inPos>=length)
   {
      return;
   }
   else if (inPos<0)
   {
      inPos += length;
      if (inPos<0)
         inPos =0;
   }
   if (inLen<0)
      return;
   if (inPos+inLen>length)
      inLen = length - inPos;

   int s = GetElementSize();
   if (outResult)
   {
      outResult->resize(inLen);
      memcpy(outResult->mBase, mBase+inPos*s, s*inLen);
      // todo - only needed if we have dirty pointer elements
      HX_OBJ_WB_PESSIMISTIC_GET(outResult);
   }
   memmove(mBase+inPos*s, mBase + (inPos+inLen)*s, (length-(inPos+inLen))*s);
   resize(length-inLen);
}

void ArrayBase::Slice(ArrayBase *outResult,int inPos,int inEnd)
{
   if (inPos<0)
   {
      inPos += length;
      if (inPos<0)
         inPos =0;
   }
   if (inEnd<0)
      inEnd += length;
   if (inEnd>length)
      inEnd = length;
   int n = inEnd - inPos;
   if (n<=0)
      outResult->resize(0);
   else
   {
      outResult->resize(n);
      int s = GetElementSize();
      memcpy(outResult->mBase, mBase+inPos*s, n*s);
      // todo - only needed if we have dirty pointer elements
      HX_OBJ_WB_PESSIMISTIC_GET(outResult);
   }
}

void ArrayBase::RemoveElement(int inPos)
{
   if (inPos<length)
   {
      int s = GetElementSize();
      memmove(mBase + inPos*s, mBase+inPos*s + s, (length-inPos-1)*s );
      resize(length-1);
   }

}

void ArrayBase::Concat(ArrayBase *outResult,const char *inSecond,int inLen)
{
   char *ptr =  outResult->GetBase();
   int n = length * GetElementSize();
   memcpy(ptr,mBase,n);
   ptr += n;
   memcpy(ptr,inSecond,inLen*GetElementSize());
   HX_OBJ_WB_PESSIMISTIC_GET(this);
}


String ArrayBase::joinArray(Array_obj<String> *inArray, String inSeparator)
{
   int length = inArray->length;
   if (length==0)
      return HX_CSTRING("");

   int len = 0;
   bool isWChar = false;
   for(int i=0;i<length;i++)
   {
      String strI = inArray->__unsafe_get(i);
      if (strI.raw_ptr())
      {
         len += strI.length;
         #ifdef HX_SMART_STRINGS
         if (strI.isUTF16Encoded())
            isWChar = true;
         #endif
      }
      else
         len += 4;
   }

   len += (length-1) * inSeparator.length;
   #ifdef HX_SMART_STRINGS
   bool sepIsWide = inSeparator.isUTF16Encoded();
   if (isWChar || sepIsWide)
   {
      char16_t *buf = String::allocChar16Ptr(len);
      int pos = 0;
      bool separated = inSeparator.length>0;

      for(int i=0;i<length;i++)
      {
         String strI = inArray->__unsafe_get(i);
         if (!strI.raw_ptr())
         {
            memcpy(buf+pos,u"null",8);
            pos+=4;
         }
         else if(strI.length==0)
         {
            // ignore
         }
         else if (strI.isUTF16Encoded())
         {
            memcpy(buf+pos,strI.raw_wptr(),strI.length*sizeof(char16_t));
            pos += strI.length;
         }
         else
         {
            const char *ptr = strI.raw_ptr();
            for(int c=0;c<strI.length;c++)
               buf[pos++] = ptr[c];
         }

         if (separated && (i+1<length) )
         {
            if (sepIsWide)
            {
               memcpy(buf+pos,inSeparator.raw_ptr(),inSeparator.length*sizeof(char16_t));
               pos += inSeparator.length;
            }
            else
            {
               const char *ptr = inSeparator.raw_ptr();
               for(int c=0;c<inSeparator.length;c++)
                  buf[pos++] = ptr[c];
            }
         }
      }
      buf[len] = '\0';

      String result(buf,len);
      return result;
   }
   #endif
   {
      char *buf = hx::NewString(len);

      int pos = 0;
      bool separated = inSeparator.length>0;
      for(int i=0;i<length;i++)
      {
         String strI = inArray->__unsafe_get(i);
         if (!strI.raw_ptr())
         {
            memcpy(buf+pos,"null",4);
            pos+=4;
         }
         else
         {
            memcpy(buf+pos,strI.raw_ptr(),strI.length*sizeof(char));
            pos += strI.length;
         }
         if (separated && (i+1<length) )
         {
            memcpy(buf+pos,inSeparator.raw_ptr(),inSeparator.length*sizeof(char));
            pos += inSeparator.length;
         }
      }
      buf[len] = '\0';

      return String(buf,len);
   }
}


int _hx_toString_depth = 0;
String ArrayBase::joinArray(ArrayBase *inBase, String inSeparator)
{
   if (_hx_toString_depth >= 5)
      return HX_CSTRING("...");
   int length = inBase->length;
   if (length==0)
      return HX_CSTRING("");

   Array<String> stringArray = Array_obj<String>::__new(length, length);
   _hx_toString_depth++;
   try
   {
      for(int i=0;i<length;i++)
         stringArray->__unsafe_set(i, inBase->ItemString(i));
      _hx_toString_depth--;
   }
   catch (...)
   {
      _hx_toString_depth--;
      throw;
   }

   return stringArray->join(inSeparator);
}

template<typename T>
struct ArrayBaseSorter
{
   ArrayBaseSorter(T *inArray, Dynamic inFunc)
   {
      mFunc = inFunc;
      mArray = inArray;
   }

   bool operator()(int inA, int inB)
      { return mFunc(mArray[inA], mArray[inB])->__ToInt() < 0; }

   Dynamic mFunc;
   T*      mArray;
};

template<typename T,typename STORE>
void TArraySortLen(T *inArray, int inLength, Dynamic inSorter)
{
   std::vector<STORE> index(inLength);
   for(int i=0;i<inLength;i++)
      index[i] = (STORE)i;

   std::stable_sort(index.begin(), index.end(), ArrayBaseSorter<T>(inArray,inSorter) );

   // Put the results back ...
   for(int i=0;i<inLength;i++)
   {
      int from = index[i];
      while(from < i)
         from = index[from];
      if (from!=i)
      {
         std::swap(inArray[i],inArray[from]);
         index[i] = from;
      }
   }
}

template<typename T>
void TArraySort(T *inArray, int inLength, Dynamic inSorter)
{
   if (inLength<2)
      return;
   if (inLength<=256)
      TArraySortLen<T,unsigned char >(inArray, inLength, inSorter);
   else if (inLength<=65536)
      TArraySortLen<T,unsigned short >(inArray, inLength, inSorter);
   else
      TArraySortLen<T,unsigned int >(inArray, inLength, inSorter);
}

void ArrayBase::safeSort(Dynamic inSorter, bool inIsString)
{
   if (inIsString)
      TArraySort((String *)mBase, length,inSorter);
   else
      TArraySort((Dynamic *)mBase, length,inSorter);
}



#ifdef HXCPP_VISIT_ALLOCS
#define ARRAY_VISIT_FUNC \
    void __Visit(hx::VisitContext *__inCtx) { HX_VISIT_MEMBER(mThis); }
#else
#define ARRAY_VISIT_FUNC
#endif

#define DEFINE_ARRAY_FUNC(ret,func,array_list,dynamic_arg_list,arg_list,ARG_C) \
struct ArrayBase_##func : public hx::Object \
{ \
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdClosure }; \
   bool __IsFunction() const { return true; } \
   ArrayBase *mThis; \
   ArrayBase_##func(ArrayBase *inThis) : mThis(inThis) { } \
   String toString() const{ return HX_CSTRING(#func) ; } \
   String __ToString() const{ return HX_CSTRING(#func) ; } \
   int __GetType() const { return vtFunction; } \
   void *__GetHandle() const { return mThis; } \
   int __ArgCount() const { return ARG_C; } \
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(mThis); } \
   ARRAY_VISIT_FUNC \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      ret mThis->__##func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      ret mThis->__##func(arg_list); return Dynamic(); \
   } \
   int __Compare(const hx::Object *inRHS) const \
   { \
      if (!dynamic_cast<const ArrayBase_##func *>(inRHS)) return -1; \
      return (mThis==inRHS->__GetHandle() ? 0 : -1); \
   } \
}; \
Dynamic ArrayBase::func##_dyn()  { return new ArrayBase_##func(this);  }


#define DEFINE_ARRAY_FUNC0(ret,func) DEFINE_ARRAY_FUNC(ret,func,HX_ARR_LIST0,HX_DYNAMIC_ARG_LIST0,HX_ARG_LIST0,0)
#define DEFINE_ARRAY_FUNC1(ret,func) DEFINE_ARRAY_FUNC(ret,func,HX_ARR_LIST1,HX_DYNAMIC_ARG_LIST1,HX_ARG_LIST1,1)
#define DEFINE_ARRAY_FUNC2(ret,func) DEFINE_ARRAY_FUNC(ret,func,HX_ARR_LIST2,HX_DYNAMIC_ARG_LIST2,HX_ARG_LIST2,2)
#define DEFINE_ARRAY_FUNC3(ret,func) DEFINE_ARRAY_FUNC(ret,func,HX_ARR_LIST3,HX_DYNAMIC_ARG_LIST3,HX_ARG_LIST3,3)
#define DEFINE_ARRAY_FUNC4(ret,func) DEFINE_ARRAY_FUNC(ret,func,HX_ARR_LIST4,HX_DYNAMIC_ARG_LIST4,HX_ARG_LIST4,4)


#if (HXCPP_API_LEVEL>=330)
DEFINE_ARRAY_FUNC1(,__SetSize);
DEFINE_ARRAY_FUNC1(,__SetSizeExact);
DEFINE_ARRAY_FUNC2(,insert);
DEFINE_ARRAY_FUNC0(,reverse);
DEFINE_ARRAY_FUNC1(,sort);
DEFINE_ARRAY_FUNC1(,unshift);
DEFINE_ARRAY_FUNC4(,blit);
DEFINE_ARRAY_FUNC2(,zero);
DEFINE_ARRAY_FUNC1(,resize);
#else
DEFINE_ARRAY_FUNC1(return,__SetSize);
DEFINE_ARRAY_FUNC1(return,__SetSizeExact);
DEFINE_ARRAY_FUNC2(return,insert);
DEFINE_ARRAY_FUNC0(return,reverse);
DEFINE_ARRAY_FUNC1(return,sort);
DEFINE_ARRAY_FUNC1(return,unshift);
DEFINE_ARRAY_FUNC4(return,blit);
DEFINE_ARRAY_FUNC2(return,zero);
DEFINE_ARRAY_FUNC1(return,resize);
#endif

DEFINE_ARRAY_FUNC1(return,concat);
DEFINE_ARRAY_FUNC0(return,iterator);
DEFINE_ARRAY_FUNC0(return,keyValueIterator);
DEFINE_ARRAY_FUNC1(return,join);
DEFINE_ARRAY_FUNC0(return,pop);
DEFINE_ARRAY_FUNC0(return,copy);
DEFINE_ARRAY_FUNC1(return,push);
DEFINE_ARRAY_FUNC1(return,contains);
DEFINE_ARRAY_FUNC1(return,remove);
DEFINE_ARRAY_FUNC1(return,removeAt);
DEFINE_ARRAY_FUNC2(return,indexOf);
DEFINE_ARRAY_FUNC2(return,lastIndexOf);
DEFINE_ARRAY_FUNC0(return,shift);
DEFINE_ARRAY_FUNC2(return,slice);
DEFINE_ARRAY_FUNC2(return,splice);
DEFINE_ARRAY_FUNC0(return,toString);
DEFINE_ARRAY_FUNC1(return,map);
DEFINE_ARRAY_FUNC1(return,filter);
DEFINE_ARRAY_FUNC1(return,__unsafe_get);
DEFINE_ARRAY_FUNC2(return,__unsafe_set);
DEFINE_ARRAY_FUNC1(return,memcmp);




hx::Val ArrayBase::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   if (inString==HX_CSTRING("length")) return (int)size();
   if (inString==HX_CSTRING("concat")) return concat_dyn();
   if (inString==HX_CSTRING("insert")) return insert_dyn();
   if (inString==HX_CSTRING("copy")) return copy_dyn();
   if (inString==HX_CSTRING("iterator")) return iterator_dyn();
   if (inString==HX_CSTRING("keyValueIterator")) return keyValueIterator_dyn();
   if (inString==HX_CSTRING("join")) return join_dyn();
   if (inString==HX_CSTRING("pop")) return pop_dyn();
   if (inString==HX_CSTRING("push")) return push_dyn();
   if (inString==HX_CSTRING("contains")) return contains_dyn();
   if (inString==HX_CSTRING("remove")) return remove_dyn();
   if (inString==HX_CSTRING("removeAt")) return removeAt_dyn();
   if (inString==HX_CSTRING("indexOf")) return indexOf_dyn();
   if (inString==HX_CSTRING("lastIndexOf")) return lastIndexOf_dyn();
   if (inString==HX_CSTRING("reverse")) return reverse_dyn();
   if (inString==HX_CSTRING("shift")) return shift_dyn();
   if (inString==HX_CSTRING("splice")) return splice_dyn();
   if (inString==HX_CSTRING("slice")) return slice_dyn();
   if (inString==HX_CSTRING("sort")) return sort_dyn();
   if (inString==HX_CSTRING("toString")) return toString_dyn();
   if (inString==HX_CSTRING("unshift")) return unshift_dyn();
   if (inString==HX_CSTRING("filter")) return filter_dyn();
   if (inString==HX_CSTRING("map")) return map_dyn();
   if (inString==HX_CSTRING("__SetSize")) return __SetSize_dyn();
   if (inString==HX_CSTRING("__SetSizeExact")) return __SetSizeExact_dyn();
   if (inString==HX_CSTRING("__unsafe_get")) return __unsafe_get_dyn();
   if (inString==HX_CSTRING("__unsafe_set")) return __unsafe_set_dyn();
   if (inString==HX_CSTRING("blit")) return blit_dyn();
   if (inString==HX_CSTRING("zero")) return zero_dyn();
   if (inString==HX_CSTRING("memcmp")) return memcmp_dyn();
   if (inString==HX_CSTRING("_hx_storeType")) return (int)getStoreType();
   if (inString==HX_CSTRING("_hx_elementSize")) return (int)GetElementSize();
   if (inString==HX_CSTRING("_hx_pointer")) return cpp::CreateDynamicPointer((void *)mBase);
   if (inString==HX_CSTRING("resize")) return resize_dyn();
   return null();
}


static String sArrayFields[] = {
   HX_CSTRING("length"),
   HX_CSTRING("concat"),
   HX_CSTRING("insert"),
   HX_CSTRING("iterator"),
   HX_CSTRING("keyValueIterator"),
   HX_CSTRING("join"),
   HX_CSTRING("copy"),
   HX_CSTRING("pop"),
   HX_CSTRING("push"),
   HX_CSTRING("contains"),
   HX_CSTRING("remove"),
   HX_CSTRING("removeAt"),
   HX_CSTRING("indexOf"),
   HX_CSTRING("lastIndexOf"),
   HX_CSTRING("reverse"),
   HX_CSTRING("shift"),
   HX_CSTRING("slice"),
   HX_CSTRING("splice"),
   HX_CSTRING("sort"),
   HX_CSTRING("toString"),
   HX_CSTRING("unshift"),
   HX_CSTRING("filter"),
   HX_CSTRING("map"),
   HX_CSTRING("resize"),
   String(null())
};



// TODO;
hx::Class ArrayBase::__mClass;

Dynamic ArrayCreateEmpty() { return new Array<Dynamic>(0,0); }
Dynamic ArrayCreateArgs(DynamicArray inArgs)
{
   return inArgs->__copy();
}

static bool ArrayCanCast(hx::Object *inInstance)
{
   return inInstance->__GetClass().mPtr == ArrayBase::__mClass.mPtr;
}

void ArrayBase::__boot()
{
   Static(__mClass) = hx::_hx_RegisterClass(HX_CSTRING("Array"),ArrayCanCast,sNone,sArrayFields,
                                    ArrayCreateEmpty,ArrayCreateArgs,0,0);
}



bool DynamicEq(const Dynamic &a, const Dynamic &b)
{
   // ? return hx::IsInstanceEq(a,b);
   return hx::IsEq(a,b);
}



// -------- ArrayIterator -------------------------------------

} // End namespace hx


namespace cpp
{
HX_DEFINE_DYNAMIC_FUNC0(IteratorBase,hasNext,return)
HX_DEFINE_DYNAMIC_FUNC0(IteratorBase,_dynamicNext,return)

Dynamic IteratorBase::next_dyn()
{
   return hx::CreateMemberFunction0("next",this,__IteratorBase_dynamicNext);
}

hx::Val IteratorBase::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   if (inString==HX_CSTRING("hasNext")) return hasNext_dyn();
   if (inString==HX_CSTRING("next")) return _dynamicNext_dyn();
   return null();
}
}


#ifdef HX_VARRAY_DEFINED
// -------- VirtualArray -------------------------------------

namespace cpp
{



#define DEFINE_VARRAY_FUNC(ret, func,array_list,dynamic_arg_list,arg_list,ARG_C) \
struct VirtualArray_##func : public hx::Object \
{ \
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdClosure }; \
   bool __IsFunction() const { return true; } \
   VirtualArray mThis; \
   VirtualArray_##func(VirtualArray inThis) : mThis(inThis) { \
      HX_OBJ_WB_NEW_MARKED_OBJECT(this); \
   } \
   String toString() const{ return HX_CSTRING(#func) ; } \
   String __ToString() const{ return HX_CSTRING(#func) ; } \
   int __GetType() const { return vtFunction; } \
   void *__GetHandle() const { return mThis.mPtr; } \
   int __ArgCount() const { return ARG_C; } \
   void __Mark(hx::MarkContext *__inCtx) { HX_MARK_MEMBER(mThis); } \
   ARRAY_VISIT_FUNC \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      ret mThis->func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      ret mThis->func(arg_list); return Dynamic(); \
   } \
}; \
Dynamic VirtualArray_obj::func##_dyn()  { return new VirtualArray_##func(this);  }


#define DEFINE_VARRAY_FUNC0(ret,func) DEFINE_VARRAY_FUNC(ret,func,HX_ARR_LIST0,HX_DYNAMIC_ARG_LIST0,HX_ARG_LIST0,0)
#define DEFINE_VARRAY_FUNC1(ret,func) DEFINE_VARRAY_FUNC(ret,func,HX_ARR_LIST1,HX_DYNAMIC_ARG_LIST1,HX_ARG_LIST1,1)
#define DEFINE_VARRAY_FUNC2(ret,func) DEFINE_VARRAY_FUNC(ret,func,HX_ARR_LIST2,HX_DYNAMIC_ARG_LIST2,HX_ARG_LIST2,2)
#define DEFINE_VARRAY_FUNC3(ret,func) DEFINE_VARRAY_FUNC(ret,func,HX_ARR_LIST3,HX_DYNAMIC_ARG_LIST3,HX_ARG_LIST3,3)
#define DEFINE_VARRAY_FUNC4(ret,func) DEFINE_VARRAY_FUNC(ret,func,HX_ARR_LIST4,HX_DYNAMIC_ARG_LIST4,HX_ARG_LIST4,4)


DEFINE_VARRAY_FUNC1(return,concat);
DEFINE_VARRAY_FUNC2(,insert);
DEFINE_VARRAY_FUNC0(return,iterator);
DEFINE_VARRAY_FUNC0(return,keyValueIterator);
DEFINE_VARRAY_FUNC1(return,join);
DEFINE_VARRAY_FUNC0(return,pop);
DEFINE_VARRAY_FUNC0(return,copy);
DEFINE_VARRAY_FUNC1(return,push);
DEFINE_VARRAY_FUNC1(return,contains);
DEFINE_VARRAY_FUNC1(return,remove);
DEFINE_VARRAY_FUNC1(return,removeAt);
DEFINE_VARRAY_FUNC2(return,indexOf);
DEFINE_VARRAY_FUNC2(return,lastIndexOf);
DEFINE_VARRAY_FUNC0(,reverse);
DEFINE_VARRAY_FUNC0(return,shift);
DEFINE_VARRAY_FUNC2(return,slice);
DEFINE_VARRAY_FUNC2(return,splice);
DEFINE_VARRAY_FUNC1(,sort);
DEFINE_VARRAY_FUNC0(return,toString);
DEFINE_VARRAY_FUNC1(,unshift);
DEFINE_VARRAY_FUNC1(return,map);
DEFINE_VARRAY_FUNC1(return,filter);
DEFINE_VARRAY_FUNC1(,__SetSize);
DEFINE_VARRAY_FUNC1(,__SetSizeExact);
DEFINE_VARRAY_FUNC2(,zero);
DEFINE_VARRAY_FUNC1(,memcmp);
DEFINE_VARRAY_FUNC1(return,__unsafe_get);
DEFINE_VARRAY_FUNC2(return,__unsafe_set);
DEFINE_VARRAY_FUNC4(,blit);
DEFINE_VARRAY_FUNC1(,resize);




#if (HXCPP_API_LEVEL>330)
int VirtualArray_obj::__Compare(const hx::Object *inRHS) const
{
   if (inRHS->__GetType()!=vtArray)
      return -1;
   ArrayCommon *common = (ArrayCommon *)inRHS;
   hx::Object *a = const_cast<VirtualArray_obj *>(this)->__GetRealObject();
   hx::Object *b = common->__GetRealObject();
   return a<b ? -1 : a>b;
}
#endif

Dynamic VirtualArray_obj::__GetItem(int inIndex) const
{
   checkBase();
   if (store==hx::arrayEmpty || inIndex<0 || inIndex>=get_length()) return
      null();
   return base->__GetItem(inIndex);
}

Dynamic VirtualArray_obj::__SetItem(int inIndex,Dynamic inValue)
{
   checkBase();

   if (store!=hx::arrayFixed)
   {
      if (inIndex>(store==hx::arrayEmpty ? 0 : (int)base->length) )
         EnsureObjectStorage();
      else
         EnsureStorage(inValue);
   }

   base->__SetItem(inIndex,inValue);
   return inValue;
}

hx::Val VirtualArray_obj::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   if (inString==HX_CSTRING("length")) return (int)get_length();
   if (inString==HX_CSTRING("concat")) return concat_dyn();
   if (inString==HX_CSTRING("insert")) return insert_dyn();
   if (inString==HX_CSTRING("copy")) return copy_dyn();
   if (inString==HX_CSTRING("iterator")) return iterator_dyn();
   if (inString==HX_CSTRING("keyValueIterator")) return keyValueIterator_dyn();
   if (inString==HX_CSTRING("join")) return join_dyn();
   if (inString==HX_CSTRING("pop")) return pop_dyn();
   if (inString==HX_CSTRING("push")) return push_dyn();
   if (inString==HX_CSTRING("contains")) return contains_dyn();
   if (inString==HX_CSTRING("remove")) return remove_dyn();
   if (inString==HX_CSTRING("removeAt")) return removeAt_dyn();
   if (inString==HX_CSTRING("indexOf")) return indexOf_dyn();
   if (inString==HX_CSTRING("lastIndexOf")) return lastIndexOf_dyn();
   if (inString==HX_CSTRING("reverse")) return reverse_dyn();
   if (inString==HX_CSTRING("shift")) return shift_dyn();
   if (inString==HX_CSTRING("splice")) return splice_dyn();
   if (inString==HX_CSTRING("slice")) return slice_dyn();
   if (inString==HX_CSTRING("sort")) return sort_dyn();
   if (inString==HX_CSTRING("toString")) return toString_dyn();
   if (inString==HX_CSTRING("unshift")) return unshift_dyn();
   if (inString==HX_CSTRING("filter")) return filter_dyn();
   if (inString==HX_CSTRING("map")) return map_dyn();
   if (inString==HX_CSTRING("__SetSize")) return __SetSize_dyn();
   if (inString==HX_CSTRING("__SetSizeExact")) return __SetSizeExact_dyn();
   if (inString==HX_CSTRING("__unsafe_get")) return __unsafe_get_dyn();
   if (inString==HX_CSTRING("__unsafe_set")) return __unsafe_set_dyn();
   if (inString==HX_CSTRING("blit")) return blit_dyn();
   if (inString==HX_CSTRING("zero")) return zero_dyn();
   if (inString==HX_CSTRING("memcmp")) return memcmp_dyn();
   if (inString==HX_CSTRING("resize")) return resize_dyn();

   if (inString==HX_CSTRING("_hx_storeType"))
   {
      if (!base)
         return -1;
      return (int)base->getStoreType();
   }
   if (inString==HX_CSTRING("_hx_elementSize"))
   {
      if (!base)
         return 0;
      return (int)base->GetElementSize();
   }
   if (inString==HX_CSTRING("_hx_pointer"))
   {
      if (!base)
         return cpp::CreateDynamicPointer((void *)0);
      else
         return cpp::CreateDynamicPointer((void *)base->GetBase());
   }


   return null();

}


hx::Class VirtualArray_obj::__GetClass() const { return ArrayBase::__mClass; }
String VirtualArray_obj::toString()
{
   if (!base)
   {
      if (store==arrayEmpty)
         return HX_CSTRING("[]");
      return HX_CSTRING("null");
   }
   return base->toString();

}

void VirtualArray_obj::EnsureArrayStorage(ArrayStore inStore)
{
   switch(inStore)
   {
      case arrayFixed:
      case arrayNull:
         // These should not happen
         break;

      case arrayEmpty:
         EnsureBase();
         break;

      case arrayBool:  EnsureBoolStorage(); break;
      case arrayInt:  EnsureIntStorage(); break;
      case arrayFloat:  EnsureFloatStorage(); break;
      case arrayString:  EnsureStringStorage(); break;
      case arrayObject:  EnsureObjectStorage(); break;
   }
}

void VirtualArray_obj::EnsureArrayStorage(VirtualArray inValue)
{
   if (store!=arrayFixed)
   {
      if (inValue->store==arrayFixed)
      {
         EnsureArrayStorage(inValue->base->getStoreType());
         store = arrayFixed;
      }
      else
      {
         EnsureArrayStorage(inValue->store);
      }
   }
}

void VirtualArray_obj::MakeIntArray()
{
   if (store==arrayEmpty && base )
   {
      int len = base->length;
      base = new Array_obj<int>(len,len);
   }
   else if (!base)
      base = new Array_obj<int>(0,0);
   else
   {
      Array<int> result = Dynamic(base);
      base = result.mPtr;
   }
   store = arrayInt;
   HX_OBJ_WB_GET(this,base);
}


void VirtualArray_obj::MakeObjectArray()
{
   if (store==arrayEmpty && base )
   {
      // Actually, ok already.
   }
   else if (!base)
   {
      base = new Array_obj<Dynamic>(0,0);
      HX_OBJ_WB_GET(this,base);
   }
   else
   {
      Array<Dynamic> result = Dynamic(base);
      base = result.mPtr;
      HX_OBJ_WB_GET(this,base);
   }
   store = arrayObject;
}


void VirtualArray_obj::MakeStringArray()
{
   if (store==arrayEmpty && base )
   {
      int len = base->length;
      base = new Array_obj<String>(len,len);
   }
   else if (!base)
      base = new Array_obj<String>(0,0);
   else
   {
      Array<String> result = Dynamic(base);
      base = result.mPtr;
   }
   store = arrayString;
   HX_OBJ_WB_GET(this,base);
}


void VirtualArray_obj::MakeBoolArray()
{
   if (store==arrayEmpty && base )
   {
      int len = base->length;
      base = new Array_obj<bool>(len,len);
   }
   else if (!base)
      base = new Array_obj<bool>(0,0);
   else
   {
      Array<bool> result = Dynamic(base);
      base = result.mPtr;
   }
   store = arrayBool;
   HX_OBJ_WB_GET(this,base);
}


void VirtualArray_obj::MakeFloatArray()
{
   if (store==arrayEmpty && base )
   {
      int len = base->length;
      base = new Array_obj<Float>(len,len);
   }
   else if (!base)
      base = new Array_obj<Float>(0,0);
   else
   {
      Array<Float> result = Dynamic(base);
      base = result.mPtr;
   }
   store = arrayFloat;
   HX_OBJ_WB_GET(this,base);
}

void VirtualArray_obj::CreateEmptyArray(int inLen)
{
   base = new Array_obj<Dynamic>(inLen,inLen);
   HX_OBJ_WB_GET(this,base);
}

void VirtualArray_obj::EnsureBase()
{
   if (!base)
   {
      base = new Array_obj<unsigned char>(0,0);
      store = arrayInt;
      HX_OBJ_WB_GET(this,base);
   }
}

VirtualArray VirtualArray_obj::splice(int inPos, int len)
{
   if ( !base )
      return new VirtualArray_obj();

   Dynamic cut = base->__splice(inPos, len);

   VirtualArray result = new VirtualArray_obj( dynamic_cast<cpp::ArrayBase_obj *>(cut.mPtr), false);
   result->store = store;
   return result;
}

VirtualArray VirtualArray_obj::map(Dynamic inFunc)
{
   VirtualArray result = new VirtualArray_obj( );
   int len = get_length();
   for(int i=0;i<len;i++)
      result->push( inFunc(  base->__GetItem(i)  ) );
   return result;
}

VirtualArray VirtualArray_obj::filter(Dynamic inFunc)
{
   if ( !base )
      return new VirtualArray_obj();

   Dynamic filtered = base->__filter(inFunc);

   VirtualArray result = new VirtualArray_obj( dynamic_cast<cpp::ArrayBase_obj *>(filtered.mPtr), false);
   result->store = store;
   return result;
}

class EmptyIterator : public IteratorBase
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdArrayIterator }; \

   bool hasNext() { return false; }
   Dynamic _dynamicNext() { return null(); }
};



Dynamic VirtualArray_obj::getEmptyIterator()
{
   return new EmptyIterator();
}



} // End namespace cpp

Dynamic _hx_reslove_virtual_array(cpp::VirtualArray inArray)
{
   if (!inArray.mPtr)
      return Dynamic();
   if (inArray->store==hx::arrayFixed  || inArray->store==hx::arrayObject)
      return inArray->__GetRealObject();
   return inArray;
}

#endif
