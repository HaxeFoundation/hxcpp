#ifndef HX_INDEX_REF_H
#define HX_INDEX_REF_H

namespace hx
{

// --- IndexRef ----------------------------------------------------------
//
// Like hxFieldRef, but for array[] syntax.  A bit easier because we know
// the array type.

template<typename T>
class IndexRef
{
   typedef typename T::Obj Obj;
   Obj *mObj;
   int mIdx;
public: 
   typedef typename Obj::__array_access Return;
   IndexRef(const T &inObj,int inIdx) : mObj(inObj.mPtr), mIdx(inIdx) { }

   inline operator Return() const { return mObj->__get(mIdx); }
   inline void operator=(Return inVal)  { mObj->__set(mIdx,inVal); }
};

template<typename T>
inline IndexRef<T> IndexRefNew(const T &inObj, int inIdx)
{
   return IndexRef<T>(inObj, inIdx);
}

}

#endif
