#ifndef HX_QUICKVECMINMAX_INCLUDED
#define HX_QUICKVECMINMAX_INCLUDED

#include <stdlib.h>
#include <algorithm>

namespace hx
{

template<typename T>
struct QuickVecMinMax
{
   int mAlloc;
   int mSize;
   T *mPtr;
   T mMin;
   T mMax;

   QuickVecMinMax() : mPtr(0), mAlloc(0), mSize(0), mMin(0), mMax(0) { } 
   ~QuickVecMinMax()
   {
      if (mPtr)
         free(mPtr);
   }

   inline void push(const T &inV)
   {
      if (mSize+1>mAlloc)
      {
         mAlloc = 10 + (mSize*3/2);
         mPtr = (T *)realloc(mPtr,sizeof(T)*mAlloc);
      }
      mPtr[mSize]=inV;
      if (mMin == 0 || inV < mMin) mMin = inV;
      if (mMax == 0 || inV > mMax) mMax = inV;
      mSize++;
   }
   
   void swap(QuickVecMinMax<T> &inOther)
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
      if (mPtr[inPos] == mMin) mMin++;
      if (mPtr[inPos] == mMax) mMax--;
      mPtr[inPos] = mPtr[mSize];
      
   }
   
   inline void set_min_max()
   {   
   	  mMin = 0;
   	  mMax = 0;
   	  
   	  for(int i=0;i<mSize;i++)
      {
      	if (mMin == 0 || mPtr[i] < mMin) mMin = mPtr[i];
      	if (mMax == 0 || mPtr[i] > mMax) mMax = mPtr[i];
      }

   }
      
   inline bool has(T inVal)
   {   
   	  if (inVal >= mMin && inVal <= mMax)
   	  	for(int i=0;i<mSize;i++)
         	if (mPtr[i]==inVal)
           	 return true;

      return false;
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
   QuickVecMinMax(const QuickVecMinMax<T> &);
   void operator =(const QuickVecMinMax<T> &);
};


} // end namespace hx

#endif
