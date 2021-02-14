#ifndef HX_QUICKSEARCHVEC_INCLUDED
#define HX_QUICKSEARCHVEC_INCLUDED

#include <stdlib.h>
#include <algorithm>
#include <hx/QuickVec.h>

namespace hx
{

template<typename T>
struct QuickSearchVec
{
	
   int mHashCount;
   int mHashShift;
   
   
   QuickVec<T>** mPtr;
   QuickVec<T> mElements;
   T mMin;
   T mMax;
   
   QuickSearchVec(int hashCount = 4096, int hashShift = 2, int reserveSize = 1024) : mHashCount(hashCount), mHashShift(hashShift)
   { 
   	
   	 mMin = 0; mMax = 0;
   	 mPtr = (QuickVec<T>**)malloc(sizeof(QuickVec<T>*) * hashCount);
   	 
   	 mElements.reserve(reserveSize);
   	 for (int i = 0; i < hashCount; i++)
   	 {
   	 	mPtr[i] = new QuickVec<T>();
   	  mPtr[i]->reserve(reserveSize >> 4);
   	 }
   	 
   	 	 
   }
   
   inline void setMinMax()
   {
   	  mMin = 0;
   	  mMax = 0;
   	  
   	  for(int i=0;i<mElements.mSize;i++)
      {
      	if (mMin == 0 || mElements.mPtr[i] < mMin) mMin = mElements.mPtr[i];
      	if (mMax == 0 || mElements.mPtr[i] > mMax) mMax = mElements.mPtr[i];
      }
   }
    
   ~QuickSearchVec()
   {  
   	  for (int i = 0; i < mHashCount; i++) 
   	    delete mPtr[i];
   	  
      if (mPtr)
        free(mPtr);
   }
   
   inline bool has(T inVal)
   {  
   	  if (inVal >= mMin && inVal <= mMax)
   	  {
   	   unsigned int hash = ((size_t)inVal>>mHashShift)%mHashCount;
   	   return mPtr[hash]->indexOf(inVal) != -1;
   	  }
   	  return false;
   }
   
   inline void qerase(int inPos)
   {  
   	
      T elem = mElements[inPos];
      
   	  unsigned int hash = ((size_t)elem>>mHashShift)%mHashCount;
      mPtr[hash]->qerase_val(elem);
   	  mElements.qerase(inPos);
   	  
   }
   
   inline bool qerase_val(T inVal)
   {
   	  unsigned int hash = ((size_t)inVal>>mHashShift)%mHashCount;
      mPtr[hash]->qerase_val(inVal);
   	  return mElements.qerase_val(inVal);
   	  
   	 
   }   
   
   inline T pop()
   {
      
      T elem = mElements.pop();
      
   	  unsigned int hash = ((size_t)elem>>mHashShift)%mHashCount;
      mPtr[hash]->qerase_val(elem);
      
      
      return elem;
   }  

   inline void push(const T &inV)
   {
   	 
   	 unsigned int hash = ((size_t)inV>>mHashShift)%mHashCount;
   	 
   	 mPtr[hash]->push(inV);
   	 mElements.push(inV);

     if (mMin == 0 || inV < mMin) mMin = inV;
     if (mMax == 0 || inV > mMax) mMax = inV;
   	 
   }
   
   inline int size() const { return mElements.size(); }
   inline T &operator[](int inIndex) { return mElements[inIndex]; }
   inline const T &operator[](int inIndex) const { return mElements[inIndex]; }

private:
   QuickSearchVec(const QuickSearchVec<T> &);
   void operator =(const QuickSearchVec<T> &);

   
};



} // end namespace hx

#endif
