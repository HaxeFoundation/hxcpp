#include <hxcpp.h>
#include <vector>

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
      mBase = (char *)( (!inAtomic) ?
        hx::NewGCBytes(0, alloc * inElementSize ) : hx::NewGCPrivate(0,alloc*inElementSize));
   }
   else
      mBase = 0;
   mAlloc = alloc;
}


void ArrayBase::EnsureSize(int inSize) const
{
   int s = inSize;
   if (s>length)
   {
      if (s>mAlloc)
      {
         bool wasUnamanaged = mAlloc<0;
         int newAlloc = s*3/2 + 10;
         int bytes = newAlloc * GetElementSize();
         if (mBase)
         {
            if (wasUnamanaged)
            {
               char *base=(char *)(AllocAtomic() ? hx::NewGCPrivate(0,bytes) : hx::NewGCBytes(0,bytes));
               memcpy(base,mBase,length*GetElementSize());
               mBase = base;
            }
            else
               mBase = (char *)hx::GCRealloc(mBase, bytes );
         }
         else if (AllocAtomic())
         {
            mBase = (char *)hx::NewGCPrivate(0,bytes);
         }
         else
         {
            mBase = (char *)hx::NewGCBytes(0,bytes);
         }
         mAlloc = newAlloc;
      }
      length = s;
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
      __SetSize(newSize);

   const char *src = inSourceArray->mBase + inSourceElement*srcSize;
   char *dest = mBase + inDestElement*srcSize;
   int len = inElementCount*srcSize;
   if (src+len < dest || dest+len<src)
      memcpy(dest,src,len);
   else
      memmove(dest,src,len);
}



String ArrayBase::__ToString() const { return HX_CSTRING("Array"); }
String ArrayBase::toString()
{
   // Byte-array (not bool!)
   if (IsByteArray())
   {
      return String( (const char *) mBase, length);
   }

   return HX_CSTRING("[") + join(HX_CSTRING(",")) + HX_CSTRING("]");
}

void ArrayBase::__SetSize(int inSize)
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

 
void ArrayBase::__SetSizeExact(int inSize)
{
   if (inSize!=length || inSize!=mAlloc)
   {
      int bytes = inSize * GetElementSize();
      if (mBase)
      {
         bool wasUnamanaged = mAlloc<0;

         if (wasUnamanaged)
         {
            char *base=(char *)(AllocAtomic() ? hx::NewGCPrivate(0,bytes) : hx::NewGCBytes(0,bytes));
            memcpy(base,mBase,std::min(length,inSize)*GetElementSize());
            mBase = base;
         }
         else
            mBase = (char *)hx::GCRealloc(mBase, bytes );
      }
      else if (AllocAtomic())
      {
         mBase = (char *)hx::NewGCPrivate(0,bytes);
      }
      else
      {
         mBase = (char *)hx::NewGCBytes(0,bytes);
      }
      mAlloc = length = inSize;
   }
}

 




void ArrayBase::Insert(int inPos)
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

   outResult->__SetSize(inLen);
   int s = GetElementSize();
   memcpy(outResult->mBase, mBase+inPos*s, s*inLen);
   memmove(mBase+inPos*s, mBase + (inPos+inLen)*s, (length-(inPos+inLen))*s);
   __SetSize(length-inLen);
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
      outResult->__SetSize(0);
   else
   {
      outResult->__SetSize(n);
      int s = GetElementSize();
      memcpy(outResult->mBase, mBase+inPos*s, n*s);
   }
}

void ArrayBase::RemoveElement(int inPos)
{
   if (inPos<length)
   {
      int s = GetElementSize();
      memmove(mBase + inPos*s, mBase+inPos*s + s, (length-inPos-1)*s );
      __SetSize(length-1);
   }

}

void ArrayBase::Concat(ArrayBase *outResult,const char *inSecond,int inLen)
{
   char *ptr =  outResult->GetBase();
   int n = length * GetElementSize();
   memcpy(ptr,mBase,n);
   ptr += n;
   memcpy(ptr,inSecond,inLen*GetElementSize());

}


String ArrayBase::join(String inSeparator)
{
   int len = 0;
   for(int i=0;i<length;i++)
   {
      len += ItemString(i).length;
   }
   if (length) len += (length-1) * inSeparator.length;

   HX_CHAR *buf = hx::NewString(len);

   int pos = 0;
   bool separated = inSeparator.length>0;
   for(int i=0;i<length;i++)
   {
      String s = ItemString(i);
      memcpy(buf+pos,s.__s,s.length*sizeof(HX_CHAR));
      pos += s.length;
      if (separated && (i+1<length) )
      {
         memcpy(buf+pos,inSeparator.__s,inSeparator.length*sizeof(HX_CHAR));
         pos += inSeparator.length;
      }
   }
   buf[len] = '\0';

   return String(buf,len);
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

#define DEFINE_ARRAY_FUNC(func,array_list,dynamic_arg_list,arg_list,ARG_C) \
struct ArrayBase_##func : public hx::Object \
{ \
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
      return mThis->__##func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      return mThis->__##func(arg_list); return Dynamic(); \
   } \
}; \
Dynamic ArrayBase::func##_dyn()  { return new ArrayBase_##func(this);  }


#define DEFINE_ARRAY_FUNC0(func) DEFINE_ARRAY_FUNC(func,HX_ARR_LIST0,HX_DYNAMIC_ARG_LIST0,HX_ARG_LIST0,0)
#define DEFINE_ARRAY_FUNC1(func) DEFINE_ARRAY_FUNC(func,HX_ARR_LIST1,HX_DYNAMIC_ARG_LIST1,HX_ARG_LIST1,1)
#define DEFINE_ARRAY_FUNC2(func) DEFINE_ARRAY_FUNC(func,HX_ARR_LIST2,HX_DYNAMIC_ARG_LIST2,HX_ARG_LIST2,2)
#define DEFINE_ARRAY_FUNC3(func) DEFINE_ARRAY_FUNC(func,HX_ARR_LIST3,HX_DYNAMIC_ARG_LIST3,HX_ARG_LIST3,3)
#define DEFINE_ARRAY_FUNC4(func) DEFINE_ARRAY_FUNC(func,HX_ARR_LIST4,HX_DYNAMIC_ARG_LIST4,HX_ARG_LIST4,4)


DEFINE_ARRAY_FUNC1(concat);
DEFINE_ARRAY_FUNC2(insert);
DEFINE_ARRAY_FUNC0(iterator);
DEFINE_ARRAY_FUNC1(join);
DEFINE_ARRAY_FUNC0(pop);
DEFINE_ARRAY_FUNC0(copy);
DEFINE_ARRAY_FUNC1(push);
DEFINE_ARRAY_FUNC1(remove);
DEFINE_ARRAY_FUNC2(indexOf);
DEFINE_ARRAY_FUNC2(lastIndexOf);
DEFINE_ARRAY_FUNC0(reverse);
DEFINE_ARRAY_FUNC0(shift);
DEFINE_ARRAY_FUNC2(slice);
DEFINE_ARRAY_FUNC2(splice);
DEFINE_ARRAY_FUNC1(sort);
DEFINE_ARRAY_FUNC0(toString);
DEFINE_ARRAY_FUNC1(unshift);
DEFINE_ARRAY_FUNC1(map);
DEFINE_ARRAY_FUNC1(filter);
DEFINE_ARRAY_FUNC1(__SetSize);
DEFINE_ARRAY_FUNC1(__SetSizeExact);
DEFINE_ARRAY_FUNC1(__unsafe_get);
DEFINE_ARRAY_FUNC2(__unsafe_set);
DEFINE_ARRAY_FUNC4(blit);
DEFINE_ARRAY_FUNC2(zero);
DEFINE_ARRAY_FUNC1(memcmp);

Dynamic ArrayBase::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   if (inString==HX_CSTRING("length")) return Dynamic((int)size());
   if (inString==HX_CSTRING("concat")) return concat_dyn();
   if (inString==HX_CSTRING("insert")) return insert_dyn();
   if (inString==HX_CSTRING("copy")) return copy_dyn();
   if (inString==HX_CSTRING("iterator")) return iterator_dyn();
   if (inString==HX_CSTRING("join")) return join_dyn();
   if (inString==HX_CSTRING("pop")) return pop_dyn();
   if (inString==HX_CSTRING("push")) return push_dyn();
   if (inString==HX_CSTRING("remove")) return remove_dyn();
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
   return null();
}


static String sArrayFields[] = {
   HX_CSTRING("length"),
   HX_CSTRING("concat"),
   HX_CSTRING("insert"),
   HX_CSTRING("iterator"),
   HX_CSTRING("join"),
   HX_CSTRING("copy"),
   HX_CSTRING("pop"),
   HX_CSTRING("push"),
   HX_CSTRING("remove"),
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
   String(null())
};



// TODO;
hx::Class ArrayBase::__mClass;

Dynamic ArrayCreateEmpty() { return new Array<Dynamic>(0,0); }
Dynamic ArrayCreateArgs(DynamicArray inArgs)
{
   return inArgs->__copy();
}

void ArrayBase::__boot()
{
   Static(__mClass) = hx::RegisterClass(HX_CSTRING("Array"),TCanCast<ArrayBase>,sNone,sArrayFields,
                                    ArrayCreateEmpty,ArrayCreateArgs,0,0);
}




// -------- ArrayIterator -------------------------------------

} // End namespace hx


namespace cpp
{
HX_DEFINE_DYNAMIC_FUNC0(IteratorBase,hasNext,return)
HX_DEFINE_DYNAMIC_FUNC0(IteratorBase,_dynamicNext,return)

Dynamic IteratorBase::next_dyn()
{
   return hx::CreateMemberFunction0(this,__IteratorBase_dynamicNext);
}

Dynamic IteratorBase::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   if (inString==HX_CSTRING("hasNext")) return hasNext_dyn();
   if (inString==HX_CSTRING("next")) return _dynamicNext_dyn();
   return null();
}
}



