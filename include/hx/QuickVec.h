#ifndef HX_QUICKVEC_INCLUDED
#define HX_QUICKVEC_INCLUDED

#include <stdlib.h>
#include <algorithm>

namespace hx
{

template<typename T>
struct QuickVec
{
   int mAlloc;
   int mSize;
   T *mPtr;

   QuickVec() : mPtr(0), mAlloc(0), mSize(0) { } 
   ~QuickVec()
   {
      if (mPtr)
         free(mPtr);
   }

   inline void push(const T &inT)
   {
      if (mSize+1>mAlloc)
      {
         mAlloc = 10 + (mSize*3/2);
         mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
      }
      mPtr[mSize]=inT;
      mSize++;
   }
   void swap(QuickVec<T> &inOther)
   {
      std::swap(mAlloc, inOther.mAlloc);
      std::swap(mSize, inOther.mSize);
      std::swap(mPtr, inOther.mPtr);
   }
   T *setSize(int inSize)
   {
      if (inSize>mAlloc)
      {
         mAlloc = inSize;
         mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
      }
      mSize = inSize;
      return mPtr;
   }
   // Can push this many without realloc
   bool hasExtraCapacity(int inN)
   {
      return mSize+inN<=mAlloc;
   }

   bool safeReserveExtra(int inN)
   {
      int want = mSize + inN;
      if (want>mAlloc)
      {
         int wantAlloc = 10 + (mSize*3/2);
         if (wantAlloc<want)
            wantAlloc = want;
         T *newBuffer = (T *)malloc( sizeof(T)*wantAlloc );
         if (!newBuffer)
            return false;
         mAlloc = wantAlloc;
         if (mPtr)
         {
            memcpy(newBuffer, mPtr, mSize*sizeof(T));
            free(mPtr);
         }
         mPtr = newBuffer;
      }
      return true;
   }
   inline void pop_back() { --mSize; }
   inline T &back() { return mPtr[mSize-1]; }
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

   inline bool qerase_val(T inVal)
   {
      for(int i=0;i<mSize;i++)
         if (mPtr[i]==inVal)
         {
            --mSize;
            mPtr[i] = mPtr[mSize];
            return true;
         }
      return false;
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
   inline const T &operator[](int inIndex) const { return mPtr[inIndex]; }

private:
   QuickVec(const QuickVec<T> &);
   void operator =(const QuickVec<T> &);
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

private:
   QuickDeque(const QuickDeque<T> &);
   void operator=(const QuickDeque<T> &);
};

} // end namespace hx

#endif
