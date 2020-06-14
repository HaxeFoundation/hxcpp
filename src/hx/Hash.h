#include <stdio.h>
#include <stdlib.h>

#ifdef HXCPP_TELEMETRY
extern void __hxt_new_hash(void* obj, int size);
#endif

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
   enum { ManageKeys = 0 };

   typedef TIntElement<int>     IntValue;
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
   enum { ManageKeys = 1 };

   typedef TStringElement<int>     IntValue;
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


struct TWeakStringSet
{
   typedef null Value;
   typedef String Key;

   enum { IgnoreHash = 0 };
   enum { WeakKeys = 1 };
   enum { ManageKeys = 1 };

   typedef TWeakStringSet  IntValue;
   typedef TWeakStringSet  FloatValue;
   typedef TWeakStringSet  DynamicValue;
   typedef TWeakStringSet  StringValue;


public:
   inline void  setKey(String inKey, unsigned int inHash)
   {
      key = inKey;
      hash = inHash;
   }
   inline unsigned int getHash() { return hash; }

   Key                key;
   unsigned int       hash;
   Value              value;
   TWeakStringSet     *next;
};


struct TNonGcStringSet
{
   typedef null Value;
   typedef String Key;

   enum { IgnoreHash = 0 };
   enum { WeakKeys = 1 };
   enum { ManageKeys = 0 };

   typedef TNonGcStringSet  IntValue;
   typedef TNonGcStringSet  FloatValue;
   typedef TNonGcStringSet  DynamicValue;
   typedef TNonGcStringSet  StringValue;


public:
   inline void  setKey(String inKey, unsigned int inHash)
   {
      key = inKey;
      hash = inHash;
   }
   inline unsigned int getHash() { return hash; }

   Key                key;
   unsigned int       hash;
   Value              value;
   TNonGcStringSet    *next;
};




// An Dyanamic element must use the GC code to get a hash
template<typename VALUE,bool WEAK>
struct TDynamicElement
{
   typedef Dynamic Key;
   typedef VALUE   Value;

   enum { IgnoreHash = 0 };
   enum { WeakKeys = WEAK };
   enum { ManageKeys = 1 };

   typedef TDynamicElement<int,WEAK>     IntValue;
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
   hashNull,
};
template<typename T> struct StoreOf{ enum {store=hashObject}; };
template<> struct StoreOf<int> { enum {store=hashInt}; };
template<> struct StoreOf< ::String> { enum {store=hashString}; };
template<> struct StoreOf<Float> { enum {store=hashFloat}; };
template<> struct StoreOf<null> { enum {store=hashNull}; };

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
inline void CopyValue(null &, const null &) {  }
template<typename T> inline void CopyValue(T &outValue, const null &) {  }
template<typename T> inline void CopyValue(null &, const T &) {  }
}


struct HashRoot : public Object
{
   HashStore store;

    HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdHash };

   virtual void updateAfterGc() = 0;
};

template<typename KEY>
struct HashBase : public HashRoot
{
   HashBase(int inStore)
   {
      store = (HashStore)inStore;
   }


   virtual bool query(KEY inKey,int &outValue) = 0;
   virtual bool query(KEY inKey,::String &outValue) = 0;
   virtual bool query(KEY inKey,Float &outValue) = 0;
   virtual bool query(KEY inKey,Dynamic &outValue) = 0;

   virtual void set(KEY inKey, const int &inValue) = 0;
   virtual void set(KEY inKey, const ::String &inValue) = 0;
   virtual void set(KEY inKey, const Float &inValue) = 0;
   virtual void set(KEY inKey, const Dynamic &inValue) = 0;

   virtual void clear() = 0;

   virtual HashBase<KEY> *convertStore(HashStore inStore) = 0;

   virtual bool remove(KEY inKey) = 0;
   virtual bool exists(KEY inKey) = 0;
   virtual Array<KEY> keys() = 0;
   virtual Dynamic values() = 0;

   virtual ::String toStringRaw() { return toString(); }
};

extern void RegisterWeakHash(HashBase<Dynamic> *);
extern void RegisterWeakHash(HashBase< ::String> *);

inline void RegisterWeakHash(HashBase<int> *) { };


template<typename T>
struct ArrayValueOf{ typedef T Value; };
template<> struct ArrayValueOf<null> { typedef Dynamic Value; };

template<typename ELEMENT>
struct Hash : public HashBase< typename ELEMENT::Key >
{
   typedef typename ELEMENT::Key   Key;
   typedef typename ELEMENT::Value Value;
   typedef typename ArrayValueOf<Value>::Value ArrayValue;

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
      if (ELEMENT::WeakKeys && Element::ManageKeys)
         RegisterWeakHash(this);
   }
   inline int getSize() { return size; }

   template<typename T>
   bool TIsWeakRefValid(T &) { return true; }
   bool TIsWeakRefValid(Dynamic &key) { return IsWeakRefValid(key.mPtr); }
   bool TIsWeakRefValid(String &key) { return IsWeakRefValid(key.raw_ptr()); }


   void updateAfterGc()
   {
      if (Element::WeakKeys && Element::ManageKeys)
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
#ifdef HXCPP_TELEMETRY
      bool is_new = bucket==0;
#endif
      mask = inNewCount-1;
      //printf("expand %d -> %d\n",bucketCount, inNewCount);
      bucket = (Element **)InternalRealloc(bucketCount*sizeof(ELEMENT *), bucket,inNewCount*sizeof(ELEMENT *));
      HX_OBJ_WB_GET(this, bucket);
      //for(int b=bucketCount;b<inNewCount;b++)
      //   bucket[b] = 0;

#ifdef HXCPP_TELEMETRY
      if (is_new) __hxt_new_hash(bucket, inNewCount*sizeof(ELEMENT *));
#endif


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
               bucket[newBucket] = &e;
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
      int origSize = bucketCount;
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
      bucket = (Element **)InternalRealloc(origSize*sizeof(ELEMENT *),bucket, sizeof(ELEMENT *)*bucketCount );
      HX_OBJ_WB_GET(this, bucket);
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
         case hashNull:
             ;
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


   template<typename Finder>
   bool findEquivalentKey(Key &outKey, int inHash, const Finder &inFinder)
   {
      if (!bucket) return false;
      Element *head = bucket[inHash & mask];
      while(head)
      {
         if ( (IgnoreHash || head->getHash()==inHash) && inFinder==head->key)
         {
            outKey = head->key;
            return true;
         }
         head = head->next;
      }
      return false;
   }

   static inline bool IsNursery(const void *inPtr)
   {
      return inPtr && !(((unsigned char *)inPtr)[ HX_ENDIAN_MARK_ID_BYTE]);
   }

   template<typename SET>
   void TSet(Key inKey, const SET &inValue)
   {
      unsigned int hash = HashCalcHash(inKey);
      Element *el = find(hash,inKey);
      if (el)
      {
         CopyValue(el->value,inValue);
         if (hx::ContainsPointers<Value>())
            HX_OBJ_WB_GET(this,hx::PointerOf(el->value));
         return;
      }
      el = allocElement();
      el->setKey(inKey,hash);
      CopyValue(el->value,inValue);
      el->next = bucket[hash&mask];
      bucket[hash&mask] = el;

      #ifdef HXCPP_GC_GENERATIONAL
      unsigned char &mark =  ((unsigned char *)(this))[ HX_ENDIAN_MARK_ID_BYTE];
      if (mark == hx::gByteMarkID)
      {
         // Look for nursery objects...
         if ( IsNursery(el) || IsNursery(hx::PointerOf(el->key)) ||
             (hx::ContainsPointers<Value>() && IsNursery(hx::PointerOf(el->value)) ) )
         {
            mark|=HX_GC_REMEMBERED;
            (HX_CTX_GET)->pushReferrer(this);
         }
      }
      #endif
   }

   void set(Key inKey, const int &inValue) { TSet(inKey, inValue); }
   void set(Key inKey, const ::String &inValue)  { TSet(inKey, inValue); }
   void set(Key inKey, const Float &inValue)  { TSet(inKey, inValue); }
   void set(Key inKey, const Dynamic &inValue)  { TSet(inKey, inValue); }
   void set(Key inKey, const null &inValue)  { TSet(inKey, inValue);  }

   void clear()
   {
      bucket = 0;
      size = 0;
      mask = 0;
      bucketCount = 0;
   }


   template<typename F>
   void iterate(F &inFunc)
   {
      for(int b=0;b<bucketCount;b++)
      {
         Element *el = bucket[b];
         while(el)
         {
            inFunc(el);
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

      void operator()(typename Hash::Element *elem)
      {
         result->set(elem->key,elem->value);
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
      void operator()(typename Hash::Element *elem)
      {
         array->push(elem->key);
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
      Array<ArrayValue> array;
      ValueBuilder(int inReserve = 0)
      {
         array = Array<ArrayValue>(0,inReserve);
      }

      void operator()(typename Hash::Element *elem)
      {
         array->push(elem->value);
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
      bool raw;

      StringBuilder(int inReserve = 0,bool inRaw=false)
      {
         raw = inRaw;
         array = Array<String>(0,inReserve*4+1);
         if (!raw)
            array->push(HX_CSTRING("{ "));
      }
      void operator()(typename Hash::Element *elem)
      {
         if (array->length>1)
            array->push(HX_CSTRING(", "));
         array->push(String(elem->key));
         array->push(HX_CSTRING(" => "));
         array->push(String(elem->value));
      }
      ::String toString()
      {
         if (!raw)
            array->push(HX_CSTRING(" }"));
         return array->length==0 ? String() : array->join(HX_CSTRING(""));
      }
   };

   String toString()
   {
      StringBuilder builder(getSize());
      iterate(builder);
      return builder.toString();
   }


   String toStringRaw()
   {
      StringBuilder builder(getSize(),true);
      iterate(builder);
      return builder.toString();
   }


   // Mark ...
   struct HashMarker
   {
      hx::MarkContext *__inCtx;
      HashMarker(hx::MarkContext *ctx) : __inCtx(ctx) { }
      void operator()(typename Hash::Element *inElem)
      {
         HX_MARK_ARRAY(inElem);
         if (!Hash::Element::WeakKeys)
         {
            HX_MARK_MEMBER(inElem->key);
         }
         HX_MARK_MEMBER(inElem->value);
      }
   };

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_ARRAY(bucket);

      HashMarker marker(__inCtx);
      iterate(marker);
   }

#ifdef HXCPP_VISIT_ALLOCS

   void __Visit(hx::VisitContext *__inCtx)
   {
      //printf(" visit hash %p\n", this);
      HX_VISIT_ARRAY(bucket);
      for(int b=0;b<bucketCount;b++)
      {
         HX_VISIT_ARRAY(bucket[b]);
         Element *el = bucket[b];
         while(el)
         {
            HX_VISIT_MEMBER(el->key);
            HX_VISIT_MEMBER(el->value);
            HX_VISIT_ARRAY(el->next);
            el = el->next;
         }
      }
   }
#endif
};


} // end namespace hx

