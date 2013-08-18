#ifndef HX_QUICKVEC_INCLUDED

#include <stdlib.h>

namespace hx
{

template<typename T>
struct QuickVec
{
   QuickVec() : mPtr(0), mAlloc(0), mSize(0) { } 
   inline void push(const T &inT)
   {
      if (mSize+1>=mAlloc)
      {
         mAlloc = 10 + (mSize*3/2);
         mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
      }
      mPtr[mSize++]=inT;
   }
   void setSize(int inSize)
   {
      if (inSize>mAlloc)
      {
         mAlloc = inSize;
         mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
      }
      mSize = inSize;
   }
   inline T pop()
   {
      return mPtr[--mSize];
   }
   inline void qerase(int inPos)
   {
      --mSize;
      mPtr[inPos] = mPtr[mSize];
   }
   inline void erase(int inPos)
   {
      --mSize;
      if (mSize>inPos)
         memmove(mPtr+inPos, mPtr+inPos+1, (mSize-inPos)*sizeof(T));
   }
   void zero() { memset(mPtr,0,mSize*sizeof(T) ); }

   inline void qerase_val(T inVal)
   {
      for(int i=0;i<mSize;i++)
         if (mPtr[i]==inVal)
         {
            --mSize;
            mPtr[i] = mPtr[mSize];
            return;
         }
   }

   inline bool some_left() { return mSize; }
   inline bool empty() const { return !mSize; }
   inline void clear() { mSize = 0; }
   inline int next()
   {
      if (mSize+1>=mAlloc)
      {
         mAlloc = 10 + (mSize*3/2);
         mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
      }
      return mSize++;
   }
   inline int size() const { return mSize; }
   inline T &operator[](int inIndex) { return mPtr[inIndex]; }

   int mAlloc;
   int mSize;
   T *mPtr;
};


template<typename T>
class QuickDeque
{
    struct Slab
    {
       T mElems[1024];
    };

    QuickVec<Slab *> mSpare;
    QuickVec<Slab *> mActive;

    int  mHeadPos;
    int  mTailPos;
    Slab *mHead;
    Slab *mTail;

public:

   QuickDeque()
   {
      mHead = mTail = 0;
      mHeadPos = 1024;
      mTailPos = 1024;
   }
   ~QuickDeque()
   {
      for(int i=0;i<mSpare.size();i++)
         delete mSpare[i];
      for(int i=0;i<mActive.size();i++)
         delete mActive[i];
      delete mHead;
      if (mTail!=mHead)
         delete mTail;
   }
   inline void push(T inObj)
   {
      if (mHeadPos<1024)
      {
         mHead->mElems[mHeadPos++] = inObj;
         return;
      }
      if (mHead != mTail)
         mActive.push(mHead);
      mHead = mSpare.empty() ? new Slab : mSpare.pop();
      mHead->mElems[0] = inObj;
      mHeadPos = 1;
   }
   inline bool some_left() { return mHead!=mTail || mHeadPos!=mTailPos; }
   inline T pop()
   {
      if (mTailPos<1024)
         return mTail->mElems[mTailPos++];
      if (mTail)
         mSpare.push(mTail);
      if (mActive.empty())
      {
         mTail = mHead;
      }
      else
      {
         mTail = mActive[0];
         mActive.erase(0);
      }
      mTailPos = 1;
      return mTail->mElems[0];
   }
};

} // end namespace hx

#endif
