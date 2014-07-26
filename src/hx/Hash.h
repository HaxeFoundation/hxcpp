#include <stdio.h>
#include <stdlib.h>

namespace hx
{


inline unsigned int HashCalcHash(int inKey) { return inKey; }
inline void HashClear(int &ioValue) { }
inline void HashClear(Dynamic &ioValue) { ioValue=null(); }


template<typename KEY, typename VALUE>
struct TElement
{
   typedef KEY   Key;
   typedef VALUE Value;

   enum { IgnoreHash = 0 };

private:
   unsigned int hash;

public:
   inline void  setKey(KEY inKey, unsigned int inHash)
   {
      key = inKey;
      hash = inHash;
   }
   inline unsigned int getHash()   { return hash; }

   Value        value;
   Key          key;
   int          next;
};

// An int element has key = hash
template<typename VALUE>
struct TIntElement
{
   typedef int   Key;
   typedef VALUE Value;

   enum { IgnoreHash = 1 };
public:
   inline void  setKey(int inKey, unsigned int )
   {
      key = inKey;
   }
   inline unsigned int getHash()   { return key; }

   Value        value;
   Key          key;
   int          next;
};





template<typename ELEMENT>
struct Hash
{
   typedef typename ELEMENT::Key   Key;
   typedef typename ELEMENT::Value Value;

   //typedef TElement<Key,Value> Element;
   //typedef TIntElement<Value> Element;
   typedef ELEMENT Element;

   enum { IgnoreHash = Element::IgnoreHash };

   int     size;
   int     holes;
   int     mask;
   int     alloc;
   int     bucketCount;
   Element *element;
   int     *bucket;
   int     firstHole;

   Hash()
   {
      element = 0;
      bucket = 0;
      size = alloc = holes = 0;
      mask = 0;
      bucketCount = 0;
      firstHole = -1;
   }

   void destroy()
   {
      if (bucket)
      {
         free(bucket);
         bucket = 0;
      }
      if (element)
      {
         free(element);
         element = 0;
      }
   }

   ~Hash()
   {
      destroy();
   }

   int getSize() { return size-holes; }

   void rebucket(int inNewCount)
   {
      mask = inNewCount-1;
      //printf("expand -> %d\n", inNewCount);
      bucket = (int *)realloc(bucket,inNewCount*sizeof(int));
      for(int b=bucketCount;b<inNewCount;b++)
         bucket[b] = -1;

      for(int b=0;b<bucketCount;b++)
      {
         int *head = &bucket[b];
         while(*head >= 0)
         {
            Element &e = element[*head];
            int newBucket = e.getHash()&mask;
            if ( newBucket != b )
            {
               int slot = *head;
               *head = e.next;
               e.next = bucket[newBucket];
               bucket[newBucket] = slot;
            }
            else
               head = &e.next;
         }
      }

      bucketCount = inNewCount;
   }

   void compact()
   {
      int newSize = bucketCount>>1;
      //printf("compact -> %d\n", newSize);
      mask = newSize-1;
      for(int b=newSize; b<bucketCount; b++)
      {
         int head = bucket[b];
         if (head>=0)
         {
            int oldHead = bucket[b-newSize];
            bucket[b-newSize] = head;

            int *lastPtr = &element[head].next;
            while(*lastPtr >= 0)
               lastPtr = &element[ *lastPtr ].next;
            *lastPtr = oldHead;
         }
      }
      bucketCount = newSize;
      bucket = (int *)realloc(bucket, sizeof(int)*bucketCount );
   }

   bool remove(Key inKey)
   {
      if (!bucket)
         return false;
      unsigned int hash = HashCalcHash(inKey);
      int *head = bucket + (hash&mask);
      while(*head>=0)
      {
         Element &el = element[*head];
         if ( (IgnoreHash || el.getHash()==hash) && el.key==inKey)
         {
            int slot = *head;
            *head = el.next;
            HashClear(el.value);
            el.next = firstHole;
            firstHole = slot;
            holes++;
            if (bucketCount>8 && size-holes < bucketCount )
               compact();
            return true;
         }
         head = &el.next;
      }
      return false;
   }

   int allocElement()
   {
      if (firstHole>=0)
      {
         int result = firstHole;
         firstHole = element[firstHole].next;
         holes--;
         return result;
      }
      int newSize = size + 1;
      if (newSize>=alloc)
      {
         alloc = (newSize+10)*3/2;
         element = (Element *)realloc(element, sizeof(Element)*alloc);
      }
      if ( newSize > (bucketCount>>1) )
      {
         int newCount = bucketCount;
         if (newCount==0)
            newCount = 8;
         else
            while( newSize > (newCount>>1) )
               newCount<<=1;
         rebucket(newCount);
      }

      return size++;
   }

   Element *find(int inHash, Key inKey)
   {
      if (!bucket) return 0;
      int slot = bucket[inHash & mask];
      while(slot>=0)
      {
         Element &el = element[slot];
         if ( (IgnoreHash || el.getHash()==inHash) && el.key==inKey)
            return &el;
         slot = el.next;
      }
      return 0;
   }

   bool exists(Key inKey) { return find( HashCalcHash(inKey), inKey ); }

   template<typename OUT>
   bool query(Key inKey,OUT &outValue)
   {
      Element *result = find( HashCalcHash(inKey), inKey );
      if (!result)
         return false;
      outValue = result->value;
      return true;
   }


   Value get(Key inKey)
   {
      Element *result = find( HashCalcHash(inKey), inKey );
      if (result)
         return result->value;
      return 0;
   }

   void set(Key inKey, const Value &inValue)
   {
      unsigned int hash = HashCalcHash(inKey);
      Element *el = find(hash,inKey);
      if (el)
      {
         el->value = inValue;
         return;
      }
      int slot = allocElement();
      el = element+slot;
      el->setKey(inKey,hash);
      el->value = inValue;
      el->next = bucket[hash&mask];
      bucket[hash&mask] = slot;
   }

   template<typename F>
   void iterate(F &inFunc)
   {
      for(int b=0;b<bucketCount;b++)
      {
         int head = bucket[b];
         while(head>=0)
         {
            Element &el = element[head];

            inFunc(el);

            head = el.next;
         }
      }
   }

};



template<typename Hash>
struct HashMarker
{
   hx::MarkContext *__inCtx;

   HashMarker(hx::MarkContext *ctx) : __inCtx(ctx) { }

   void operator()(typename Hash::Element &inElem)
   {
      HX_MARK_MEMBER(inElem.key);
      HX_MARK_MEMBER(inElem.value);
   }
};


template<typename Hash>
struct HashVisitor
{
   hx::VisitContext *__inCtx;

   HashVisitor(hx::VisitContext *ctx) : __inCtx(ctx) { }

   void operator()(typename Hash::Element &inElem)
   {
      HX_VISIT_MEMBER(inElem.key);
      HX_VISIT_MEMBER(inElem.value);
   }
};

template<typename Key>
struct KeyBuilder
{
   Array<Key> array;

   KeyBuilder(int inReserve = 0)
   {
      array = Array<Key>(0,inReserve);
   }
   template<typename ELEM>
   void operator()(ELEM &elem)
   {
      array->push(elem.key);
   }
};
template<typename Value>
struct ValueBuilder
{
   Array<Value> array;

   ValueBuilder(int inReserve = 0)
   {
      array = Array<Value>(0,inReserve);
   }
   template<typename ELEM>
   void operator()(ELEM &elem)
   {
      array->push(elem.value);
   }
};

struct StringBuilder
{
   Array<String> array;

   StringBuilder(int inReserve = 0)
   {
      array = Array<String>(0,inReserve*4+1);
      array->push(HX_CSTRING("{ "));
   }
   template<typename ELEM>
   void operator()(ELEM &elem)
   {
      if (array->length>1)
         array->push(HX_CSTRING(","));
      array->push(String(elem.key));
      array->push(HX_CSTRING(" => "));
      array->push(String(elem.value));
   }

   ::String toString()
   {
      array->push(HX_CSTRING("}"));
      return array->join(HX_CSTRING(""));
   }
};

template<typename T>
struct NeedsMarking { enum { Yes = 0 }; };
template<> struct NeedsMarking<Dynamic> { enum { Yes = 1 }; };
template<> struct NeedsMarking<String> { enum { Yes = 1 }; };

}



