#include <stdio.h>
#include <stdlib.h>

namespace hx
{


inline unsigned int HashCalcHash(int inKey) { return inKey; }
inline unsigned int HashCalcHash(const String &inKey) { return inKey.hash(); }
inline unsigned int HashCalcHash(const Dynamic &inKey)
{
   return __hxcpp_obj_hash(inKey);
}

inline void HashClear(int &ioValue) { }
inline void HashClear(Dynamic &ioValue) { ioValue=null(); }
inline void HashClear(String &ioValue) { ioValue=String(); }
inline void HashClear(Float &ioValue) {  }

template<typename T>
struct NeedsMarking { enum { Yes = 0 }; };
template<> struct NeedsMarking<Dynamic> { enum { Yes = 1 }; };
template<> struct NeedsMarking<String> { enum { Yes = 1 }; };

// An int element has key = hash
template<typename VALUE>
struct TIntElement
{
   typedef int   Key;
   typedef VALUE Value;

   enum { IgnoreHash = 1 };
   enum { WeakKeys = 0 };

   typedef TIntElement<Int>     IntValue;
   typedef TIntElement<Float>   FloatValue;
   typedef TIntElement<Dynamic> DynamicValue;
   typedef TIntElement<String>  StringValue;

public:
   inline void  setKey(int inKey, unsigned int )
   {
      key = inKey;
   }
   inline unsigned int getHash()   { return key; }

   Value               value;
   Key                 key;
   TIntElement<VALUE>  *next;
};


// An string element gets hash from string or calculates it
template<typename VALUE>
struct TStringElement
{
   typedef String Key;
   typedef VALUE  Value;

   enum { IgnoreHash = 0 };
   enum { WeakKeys = 0 };

   typedef TStringElement<Int>     IntValue;
   typedef TStringElement<Float>   FloatValue;
   typedef TStringElement<Dynamic> DynamicValue;
   typedef TStringElement<String>  StringValue;


public:
   inline void  setKey(String inKey, unsigned int inHash)
   {
      key = inKey;
      hash = inHash;
   }
   inline unsigned int getHash() { return hash; }

   Value                  value;
   Key                    key;
   unsigned int           hash;
   TStringElement<VALUE>  *next;
};



// An Dyanamic element must use the GC code get get a hash
template<typename VALUE,bool WEAK>
struct TDynamicElement
{
   typedef Dynamic Key;
   typedef VALUE   Value;

   enum { IgnoreHash = 0 };
   enum { WeakKeys = WEAK };

   typedef TDynamicElement<Int,WEAK>     IntValue;
   typedef TDynamicElement<Float,WEAK>   FloatValue;
   typedef TDynamicElement<Dynamic,WEAK> DynamicValue;
   typedef TDynamicElement<String,WEAK>  StringValue;

public:
   inline void  setKey(Dynamic inKey, unsigned int inHash)
   {
      key = inKey;
      hash = inHash;
   }
   inline unsigned int getHash() { return hash; }

   Value                        value;
   Key                          key;
   unsigned int                 hash;
   TDynamicElement<VALUE,WEAK>  *next;
};




enum HashStore
{
   hashInt,
   hashFloat,
   hashString,
   hashObject,
};
template<typename T> struct StoreOf{ enum {store=hashObject}; };
template<> struct StoreOf<int> { enum {store=hashInt}; };
template<> struct StoreOf< ::String> { enum {store=hashString}; };
template<> struct StoreOf<Float> { enum {store=hashFloat}; };

namespace
{
inline void CopyValue(Dynamic &outValue, const Dynamic &inValue) { outValue = inValue; }
inline void CopyValue(String &outValue, const String &inValue) { outValue = inValue; }
inline void CopyValue(String &outValue, Float inValue) {  }
inline void CopyValue(String &outValue, const Dynamic &inValue) { outValue = inValue; }
inline void CopyValue(int &outValue, int inValue) { outValue = inValue; }
inline void CopyValue(int &outValue, Float inValue) { outValue = inValue; }
inline void CopyValue(int &outValue, const Dynamic &inValue) { outValue = inValue; }
inline void CopyValue(int &outValue, const String &inValue) {  }
inline void CopyValue(Float &outValue, Float inValue) { outValue = inValue; }
inline void CopyValue(Float &outValue, const Dynamic &inValue) { outValue = inValue; }
inline void CopyValue(Float &outValue, const String &inValue) {  }
}

template<typename KEY>
struct HashBase : public Object
{
   HashStore store;

   HashBase(int inStore)
   {
      store = (HashStore)inStore;
   }


   virtual bool query(KEY inKey,int &outValue) = 0;
   virtual bool query(KEY inKey,::String &outValue) = 0;
   virtual bool query(KEY inKey,Float &outValue) = 0;
   virtual bool query(KEY inKey,Dynamic &outValue) = 0;

   virtual HashBase<KEY> * set(KEY inKey, const int &inValue) = 0;
   virtual HashBase<KEY> * set(KEY inKey, const ::String &inValue) = 0;
   virtual HashBase<KEY> * set(KEY inKey, const Float &inValue) = 0;
   virtual HashBase<KEY> * set(KEY inKey, const Dynamic &inValue) = 0;

   virtual HashBase<KEY> *convertStore(HashStore inStore) = 0;

   virtual bool remove(KEY inKey) = 0;
   virtual bool exists(KEY inKey) = 0;
   virtual Array<KEY> keys() = 0;
   virtual Dynamic values() = 0;

   virtual void updateAfterGc() = 0;
};

extern void RegisterWeakHash(HashBase<Dynamic> *);
inline void RegisterWeakHash(HashBase<Int> *) { };
inline void RegisterWeakHash(HashBase< ::String> *) { };



template<typename ELEMENT>
struct Hash : public HashBase< typename ELEMENT::Key >
{
   typedef typename ELEMENT::Key   Key;
   typedef typename ELEMENT::Value Value;

   typedef ELEMENT Element;
   enum { IgnoreHash = Element::IgnoreHash };


   int       size;
   int       mask;
   int       bucketCount;
   ELEMENT   **bucket;


   Hash() : HashBase<Key>( StoreOf<Value>::store )
   {
      bucket = 0;
      size = 0;
      mask = 0;
      bucketCount = 0;
      if (ELEMENT::WeakKeys)
         RegisterWeakHash(this);
   }
   inline int getSize() { return size; }

   template<typename T>
   bool TIsWeakRefValid(T &) { return true; }
   bool TIsWeakRefValid(Dynamic &key) { return IsWeakRefValid(key.mPtr); }


   void updateAfterGc()
   {
      if (Element::WeakKeys)
      {
         for(int b=0;b<bucketCount;b++)
         {
            Element **headPtr = &bucket[b];
            while(*headPtr)
            {
               Element &el = **headPtr;
               if (!TIsWeakRefValid(el.key))
               {
                  *headPtr = el.next;
                  size--;
               }
               else
                  headPtr = &el.next;
            }
         }
      }
   }

   void rebucket(int inNewCount)
   {
      mask = inNewCount-1;
      //printf("expand %d -> %d\n",bucketCount, inNewCount);
      bucket = (Element **)InternalRealloc(bucket,inNewCount*sizeof(ELEMENT *));
      //for(int b=bucketCount;b<inNewCount;b++)
      //   bucket[b] = 0;

      for(int b=0;b<bucketCount;b++)
      {
         Element **head = &bucket[b];
         while(*head)
         {
            Element &e = **head;
            int newBucket = e.getHash()&mask;
            if ( newBucket != b )
            {
               *head = e.next;
               e.next = bucket[newBucket];
               bucket[newBucket] = &e;;
            }
            else
               head = &e.next;
         }
      }

      bucketCount = inNewCount;
   }

   void reserve(int inSize)
   {
      if (inSize<8)
          inSize = 8;

      expandBuckets(inSize);
   }

   void compact()
   {
      int newSize = bucketCount>>1;
      // printf("compact -> %d\n", newSize);
      mask = newSize-1;
      for(int b=newSize; b<bucketCount; b++)
      {
         Element *head = bucket[b];
         if (head)
         {
            Element *oldHead = bucket[b-newSize];
            bucket[b-newSize] = head;

            if (oldHead)
            {
               // Append to last element
               Element **lastPtr = &(head->next);
               while(*lastPtr)
                  lastPtr = & (*lastPtr)->next;
               *lastPtr = oldHead;
            }
            bucket[b] = 0;
         }
      }
      bucketCount = newSize;
      bucket = (Element **)InternalRealloc(bucket, sizeof(ELEMENT *)*bucketCount );
   }

   bool remove(Key inKey)
   {
      if (!bucket)
         return false;
      unsigned int hash = HashCalcHash(inKey);
      Element **head = bucket + (hash&mask);
      while(*head)
      {
         Element &el = **head;
         if ( (IgnoreHash || el.getHash()==hash) && el.key==inKey)
         {
            *head = el.next;
            size--;
            if (bucketCount>8 && size < (bucketCount>>1) )
               compact();
            return true;
         }
         head = &el.next;
      }
      return false;
   }

   ELEMENT *allocElement()
   {
      ELEMENT *result = (ELEMENT *)InternalNew( sizeof(ELEMENT), false );
      size++;
      expandBuckets(size);
      return result;
   }

   inline void expandBuckets(int inSize)
   {
      // Trades memory vs bucket occupancy - more memory is used for elements anyhow, so not too critical
      enum { LOG_ELEMS_PER_BUCKET = 1 };
      if ( inSize > (bucketCount<<LOG_ELEMS_PER_BUCKET) )
      {
         int newCount = bucketCount;
         if (newCount==0)
            newCount = 2;
         else
            while( inSize > (newCount<<LOG_ELEMS_PER_BUCKET) )
               newCount<<=1;
         if (newCount!=bucketCount)
            rebucket(newCount);
      }
   }

   Element *find(int inHash, Key inKey)
   {
      if (!bucket) return 0;
      Element *head = bucket[inHash & mask];
      while(head)
      {
         if ( (IgnoreHash || head->getHash()==inHash) && head->key==inKey)
            return head;
         head = head->next;
      }
      return 0;
   }

   bool exists(Key inKey) { return find( HashCalcHash(inKey), inKey ); }


   HashBase<Key> *convertStore(HashStore inStore)
   {
      switch(inStore)
      {
         case hashInt:
            return TConvertStore< typename ELEMENT::IntValue >();
         case hashFloat:
            return TConvertStore< typename ELEMENT::FloatValue >();
         case hashString:
            return TConvertStore< typename ELEMENT::StringValue >();
         case hashObject:
            return TConvertStore< typename ELEMENT::DynamicValue >();
      }
      return 0;
   }


   template<typename OUT_VALUE>
   bool TQuery(Key inKey,OUT_VALUE &outValue)
   {
      Element *result = find( HashCalcHash(inKey), inKey );
      if (!result)
         return false;
      CopyValue(outValue,result->value);
      return true;
   }

   bool query(Key inKey,int &outValue) { return TQuery(inKey,outValue); }
   bool query(Key inKey,::String &outValue) { return TQuery(inKey,outValue); }
   bool query(Key inKey,Float &outValue) { return TQuery(inKey,outValue); }
   bool query(Key inKey,Dynamic &outValue) { return TQuery(inKey,outValue); }


   Value get(Key inKey)
   {
      Element *result = find( HashCalcHash(inKey), inKey );
      if (result)
         return result->value;
      return 0;
   }


   template<typename SET>
   void TSet(Key inKey, const SET &inValue)
   {
      unsigned int hash = HashCalcHash(inKey);
      Element *el = find(hash,inKey);
      if (el)
      {
         CopyValue(el->value,inValue);
         return;
      }
      el = allocElement();
      el->setKey(inKey,hash);
      CopyValue(el->value,inValue);
      el->next = bucket[hash&mask];
      bucket[hash&mask] = el;
   }

   HashBase<Key> * set(Key inKey, const int &inValue) { TSet(inKey, inValue); return this; }
   HashBase<Key> * set(Key inKey, const ::String &inValue)  { TSet(inKey, inValue); return this; }
   HashBase<Key> * set(Key inKey, const Float &inValue)  { TSet(inKey, inValue); return this; }
   HashBase<Key> * set(Key inKey, const Dynamic &inValue)  { TSet(inKey, inValue); return this; }


   template<typename F>
   void iterateAddr(F &inFunc)
   {
      for(int b=0;b<bucketCount;b++)
      {
         Element **el = &bucket[b];
         while(*el)
         {
            inFunc(el);
            el = &(*el)->next;
         }
      }
   }


   template<typename F>
   void iterate(F &inFunc)
   {
      for(int b=0;b<bucketCount;b++)
      {
         Element *el = bucket[b];
         while(el)
         {
            inFunc(*el);
            el = el->next;
         }
      }
   }

   // Convert
   template<typename NEW>
   struct Converter
   {
      NEW *result;

      Converter(NEW *inResult) : result(inResult) { }

      void operator()(typename Hash::Element &elem)
      {
         result->set(elem.key,elem.value);
      }
   };


   template<typename NEW_ELEM>
   HashBase<Key> *TConvertStore()
   {
      Hash<NEW_ELEM> *result = new Hash<NEW_ELEM>();

      result->reserve(getSize()*3/2);

      Converter< Hash<NEW_ELEM> > converter(result);

      iterate(converter);

      return result;
   }



   // Keys ...
   struct KeyBuilder
   {
      Array<Key> array;

      KeyBuilder(int inReserve = 0)
      {
         array = Array<Key>(0,inReserve);
      }
      void operator()(typename Hash::Element &elem)
      {
         array->push(elem.key);
      }
   };
   Array<Key> keys()
   {
      KeyBuilder builder(getSize());
      iterate(builder);
      return builder.array;;
   }


   // Values...
   struct ValueBuilder
   {
      Array<Value> array;
      ValueBuilder(int inReserve = 0)
      {
         array = Array<Value>(0,inReserve);
      }

      void operator()(typename Hash::Element &elem)
      {
         array->push(elem.value);
      }
   };
   Dynamic values()
   {
      ValueBuilder builder(getSize());
      iterate(builder);
      return builder.array;;
   }


   // Strings ...
   struct StringBuilder
   {
      Array<String> array;

      StringBuilder(int inReserve = 0)
      {
         array = Array<String>(0,inReserve*4+1);
         array->push(HX_CSTRING("{ "));
      }
      void operator()(typename Hash::Element &elem)
      {
         if (array->length>1)
            array->push(HX_CSTRING(", "));
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

   String toString()
   {
      StringBuilder builder(getSize());
      iterate(builder);
      return builder.toString();
   }


   // Mark ...
   struct HashMarker
   {
      hx::MarkContext *__inCtx;
      HashMarker(hx::MarkContext *ctx) : __inCtx(ctx) { }
      void operator()(typename Hash::Element &inElem)
      {
         HX_MARK_ARRAY(&inElem);
         if (!Hash::Element::WeakKeys)
         {
            HX_MARK_MEMBER(inElem.key);
         }
         HX_MARK_MEMBER(inElem.value);
      }
   };

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_ARRAY(bucket);

      HashMarker marker(__inCtx);
      iterate(marker);
   }

#ifdef HXCPP_VISIT_ALLOCS
   // Vist ...
   struct HashVisitor
   {
      hx::VisitContext *__inCtx;
      HashVisitor(hx::VisitContext *ctx) : __inCtx(ctx) { }
      void operator()(typename Hash::Element **inElem)
      {
         if ( (NeedsMarking<Key>::Yes && !ELEMENT::WeakKeys) || NeedsMarking<Value>::Yes)
         {
             typename Hash::Element &el = **inElem;
             HX_VISIT_MEMBER(el.key);
             HX_VISIT_MEMBER(el.value);
         }
         HX_VISIT_ARRAY(*inElem);
      }
   };

   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_ARRAY(bucket);
      HashVisitor vistor(__inCtx);
      iterateAddr(vistor);
   }
#endif
};


template<typename ELEMENT>
struct TinyHash : public HashBase< typename ELEMENT::Key >
{
   typedef typename ELEMENT::Key   Key;
   typedef typename ELEMENT::Value Value;

   typedef ELEMENT Element;

   enum { START_SIZE = 4 };
   enum { MAX_SIZE = 8 };

   struct KeyValue
   {
      Key   key;
      Value value;
   };

   int      count;
   int      alloc;
   KeyValue *element;

   TinyHash() : HashBase<Key>( StoreOf<Value>::store )
   {
      count = 0;
      alloc = START_SIZE;
      element = (KeyValue *)InternalRealloc(element, sizeof(KeyValue)*(alloc));
      if (ELEMENT::WeakKeys)
         RegisterWeakHash(this);
   }

   template<typename T>
   bool TIsWeakRefValid(T &) { return true; }
   bool TIsWeakRefValid(Dynamic &key) { return IsWeakRefValid(key.mPtr); }


   void updateAfterGc()
   {
      if (ELEMENT::WeakKeys)
      {
         for(int i=0;i<count;i++)
         {
            if (!TIsWeakRefValid(element[i].key))
            {
               element[i] = element[count-1];
               count--;
               i--;
            }
         }
      }
   }


   inline int find(const Key &inKey)
   {
      for(int i=0;i<count;i++)
         if (inKey == element[i].key)
            return i;
      return -1;
   }

   bool exists(Key inKey) { return find(inKey)>=0; }

   
   bool remove(Key inKey)
   {
      int idx = find(inKey);
      if (idx<0)
         return false;
      element[idx] = element[count-1];
      count--;
      return true;
   }


   template<typename OUT_VALUE>
   bool TQuery(Key inKey,OUT_VALUE &outValue)
   {
      int idx = find(inKey);
      if (idx<0)
         return false;
      CopyValue(outValue,element[idx].value);
      return true;
   }

   bool query(Key inKey,int &outValue) { return TQuery(inKey,outValue); }
   bool query(Key inKey,::String &outValue) { return TQuery(inKey,outValue); }
   bool query(Key inKey,Float &outValue) { return TQuery(inKey,outValue); }
   bool query(Key inKey,Dynamic &outValue) { return TQuery(inKey,outValue); }


   Value get(Key inKey)
   {
      int idx = find(inKey);
      if (idx>=0)
         return element[idx].value;
      return 0;
   }


   template<typename SET>
   HashBase<Key> *TSet(Key inKey, const SET &inValue)
   {
      int idx = find(inKey);
      if (idx>=0)
      {
         CopyValue(element[idx].value,inValue);
         return this;
      }
      if (count>=MAX_SIZE)
      {
         HashBase<Key> *result = convertStore( HashBase<Key>::store );
         result->set(inKey,inValue);
         return result;
      }
      if (count>=alloc)
      {
         alloc *= 2;
         element = (KeyValue *)InternalRealloc(element, sizeof(KeyValue)*alloc);
      }

      idx = count++;
      element[idx].key = inKey;
      CopyValue(element[idx].value,inValue);
      return this;
   }

   HashBase<Key> *set(Key inKey, const int &inValue) { return TSet(inKey, inValue); }
   HashBase<Key> *set(Key inKey, const ::String &inValue)  { return TSet(inKey, inValue); }
   HashBase<Key> *set(Key inKey, const Float &inValue)  { return TSet(inKey, inValue); }
   HashBase<Key> *set(Key inKey, const Dynamic &inValue)  { return TSet(inKey, inValue); }

   Array<Key> keys()
   {
      Array<Key> result(0,count);
      for(int i=0;i<count;i++)
         result->push(element[i].key);
      return result;
   }


   Dynamic values()
   {
      Array<Value> result(0,count);
      for(int i=0;i<count;i++)
         result->push(element[i].value);
      return result;
   }

   String toString()
   {
      Array<String> strings(0,count*4+1);
      strings->push(HX_CSTRING("{ "));
      for(int i=0;i<count;i++)
      {
         KeyValue &elem = element[i];
         if (i>0)
            strings->push(HX_CSTRING(", "));
         strings->push(String(elem.key));
         strings->push(HX_CSTRING(" => "));
         strings->push(String(elem.value));
      }
      strings->push(HX_CSTRING("}"));
      return strings->join(HX_CSTRING(""));
   }


   template<typename NEW_ELEM>
   HashBase<Key> *TConvertStore()
   {
      Hash<NEW_ELEM> *result = new Hash<NEW_ELEM>();

      result->reserve(count*3/2+1);

      for(int i=0;i<count;i++)
         result->set( element[i].key, element[i].value );

      return result;
   }


   HashBase<Key> *convertStore(HashStore inStore)
   {
      switch(inStore)
      {
         case hashInt:
            return TConvertStore< typename ELEMENT::IntValue >();
         case hashFloat:
            return TConvertStore< typename ELEMENT::FloatValue >();
         case hashString:
            return TConvertStore< typename ELEMENT::StringValue >();
         case hashObject:
            return TConvertStore< typename ELEMENT::DynamicValue >();
      }
      return 0;
   }



   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_ARRAY(element);
      if ( (NeedsMarking<Key>::Yes && !ELEMENT::WeakKeys) || NeedsMarking<Value>::Yes)
      {
         for(int i=0;i<count;i++)
         {
            HX_MARK_MEMBER(element[i].key);
            HX_MARK_MEMBER(element[i].value);
         }
      }
   }

#ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_ARRAY(element);
      if (NeedsMarking<Key>::Yes || NeedsMarking<Value>::Yes)
      {
         for(int i=0;i<count;i++)
         {
            HX_VISIT_MEMBER(element[i].key);
            HX_VISIT_MEMBER(element[i].value);
         }
      }
   }
#endif
};

} // end namespace hx

