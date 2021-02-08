#ifndef CPP_POINTER_H
#define CPP_POINTER_H

namespace cpp
{

struct AutoCast
{
   void *value;

   explicit inline AutoCast(void *inValue) : value(inValue) { }
};


struct RawAutoCast
{
   void *value;

   explicit inline RawAutoCast(void *inValue) : value(inValue) { }

   template<typename T>
   operator T*() const { return (T*)value; }
};


Dynamic CreateDynamicPointer(void *inValue);

enum DynamicHandlerOp
{
   dhoGetClassName,
   dhoToString,
   dhoFromDynamic,
   dhoToDynamic,
   dhoIs,
};
typedef void (*DynamicHandlerFunc)(DynamicHandlerOp op, void *ioValue, int inSize, void *outResult);
Dynamic CreateDynamicStruct(const void *inValue, int inSize, DynamicHandlerFunc inFunc);

template<typename T> class Reference;



struct StructHandlerDynamicParams
{
   StructHandlerDynamicParams(hx::Object *data,const char *inName) :
       outProcessed(false), inName(inName), inData(data) { }
   bool outProcessed;
   hx::Object *inData;
   const char *inName;
};


class DefaultStructHandler
{
   public:
      static inline const char *getName() { return "unknown"; }
      static inline String toString( const void *inValue ) { return HX_CSTRING("Struct"); }
      static inline void handler(DynamicHandlerOp op, void *ioValue, int inSize, void *outResult)
      {
         if (op==dhoToString)
            *(String *)outResult = toString(ioValue);
         else if (op==dhoGetClassName)
            *(const char **)outResult = getName();
         else if (op==dhoToDynamic)
         {
            // Handle outsize..
            *(hx::Object **)outResult = 0;
         }
         else if (op==dhoFromDynamic)
         {
            StructHandlerDynamicParams *params = (StructHandlerDynamicParams *)outResult;
            hx::Object *ptr= params->inData;
            void *data = (void *)ptr->__GetHandle();
            int len = ptr->__length();
            if (data && len>=inSize && ptr->__CStr()==params->inName)
            {
               memcpy(ioValue,data,inSize);
               params->outProcessed = true;
            }
         }
         else if (op==dhoIs)
         {
            StructHandlerDynamicParams *params = (StructHandlerDynamicParams *)outResult;
            hx::Object *ptr= params->inData;
            void *data = (void *)ptr->__GetHandle();
            int len = ptr->__length();
            params->outProcessed = data && len>=inSize && ptr->__CStr()==params->inName;
         }
      }
};


class EnumHandler
{
   public:
      static inline const char *getName() { return "enum"; }
      static inline String toString( const void *inValue ) {
         int val = inValue ? *(int *)inValue : 0;
         return HX_CSTRING("enum(") + String(val) + HX_CSTRING(")");
      }

      static inline void handler(DynamicHandlerOp op, void *ioValue, int inSize, void *outResult)
      {
         if (op==dhoToString)
            *(String *)outResult = toString(ioValue);
         else if (op==dhoGetClassName)
            *(const char **)outResult = getName();
         else if (op==dhoFromDynamic)
         {
            StructHandlerDynamicParams *params = (StructHandlerDynamicParams *)outResult;
            if (params->inData->__GetType()==vtInt)
            {
               *(int *)ioValue =  params->inData->__ToInt();
               params->outProcessed = true;
            }
            else
               DefaultStructHandler::handler(op,ioValue, inSize, outResult);
         }
         else
            DefaultStructHandler::handler(op,ioValue, inSize, outResult);
      }
};



template<typename T, typename HANDLER = DefaultStructHandler >
class Struct
{
public:
   T value;
   // This allows 'StaticCast' to be used from arrays
   typedef Dynamic Ptr;

   inline Struct( ) {  }
   inline Struct( const T &inRHS ) : value(inRHS) {  }
   inline Struct( const null &) { value = T(); }
   inline Struct( const Reference<T> &);
   inline Struct( const Dynamic &inRHS) { fromDynamic(inRHS.mPtr); }

   inline Struct<T,HANDLER> &operator=( const T &inRHS ) { value = inRHS; return *this; }
   inline Struct<T,HANDLER> &operator=( const null & ) { value = T(); return *this; }
   inline Struct<T,HANDLER> &operator=( const Dynamic &inRHS ) { return *this = Struct<T,HANDLER>(inRHS); }

   operator Dynamic() const
   {
      hx::Object *result = 0;
      HANDLER::handler(dhoToDynamic, (void *)&value, sizeof(T), &result );
      if (result)
         return result;
      return CreateDynamicStruct( &value, sizeof(T), HANDLER::handler);
   }
   operator String() const { return HANDLER::toString(&value); }

   #if (HXCPP_API_LEVEL >= 330)
   inline Struct( const hx::Val &inRHS) { fromDynamic(inRHS.asObject()); }
   operator hx::Val() const { return operator Dynamic(); }
   #endif

   bool operator==(const Struct<T,HANDLER> &inRHS) const { return value==inRHS.value; }
   bool operator==(const null &inRHS) const { return false; }
   bool operator!=(const null &inRHS) const { return true; }

   // Haxe uses -> notation
   inline T *operator->() { return &value; }

   T &get() { return value; }

   static inline bool is( const Dynamic &inRHS)
   {
      hx::Object *ptr = inRHS.mPtr;
      if (!ptr)
         return false;
      StructHandlerDynamicParams convert(ptr, ptr->__CStr());
      HANDLER::handler(dhoIs, 0, sizeof(T), &convert );
      return convert.outProcessed;
   }


   inline void fromDynamic( hx::Object *ptr)
   {
      if (!ptr)
      {
         value = T();
         return;
      }
      StructHandlerDynamicParams convert(ptr, ptr->__CStr());
      HANDLER::handler(dhoFromDynamic, &value, sizeof(T), &convert );
      if (!convert.outProcessed)
      {
         hx::NullReference("DynamicData", true);
         return;
      }
   }

   inline operator T& () { return value; }
};




template<typename T>
class Pointer
{
public:
   typedef T elementType;

   T *ptr;

   inline Pointer( ) : ptr(0) { }
   inline Pointer( const Pointer &inRHS ) : ptr(inRHS.ptr) {  }
   inline Pointer( const Dynamic &inRHS) { ptr = inRHS==null()?0: (T*)inRHS->__GetHandle(); }
   inline Pointer( const null &inRHS ) : ptr(0) { }
   inline Pointer( const cpp::Variant &inVariant ) {
      hx::Object *obj = inVariant.asObject();
      ptr = obj  ? (T*)inVariant.valObject->__GetHandle() : 0;
   }

   template<typename O>
   inline Pointer( const O *inValue ) : ptr( (T*) inValue) { }
   //inline Pointer( T *inValue ) : ptr(inValue) { }
   inline Pointer( AutoCast inValue ) : ptr( (T*)inValue.value) { }

   template<typename H>
   inline Pointer( const Struct<T,H> &structVal ) : ptr( &structVal.value ) { }

   template<typename O>
   inline void setRaw(const O *inValue ) { ptr =  (T*) inValue; }
   

   inline Pointer operator=( const Pointer &inRHS ) { return ptr = inRHS.ptr; }
   inline Dynamic operator=( Dynamic &inValue )
   {
      ptr = inValue==null() ? 0 : (T*) inValue->__GetHandle();
      return inValue;
   }
   inline Dynamic operator=( null &inValue ) { ptr=0; return inValue; }

   template<typename O>
   inline Pointer operator=( const Pointer<O> &inValue ) { ptr = (T*) inValue.ptr; return *this; }

   template<typename O>
   inline Pointer operator=( const O *inValue ) { ptr = (T*) inValue; return *this; }

   template<typename H>
   inline Pointer operator=( const Struct<T,H> &structVal ) { ptr = &structVal.value; return *this; }



   inline AutoCast reinterpret() { return AutoCast(ptr); }
   inline RawAutoCast rawCast() { return RawAutoCast(ptr); }

   inline bool operator==( const null &inValue ) const { return ptr==0; }
   inline bool operator!=( const null &inValue ) const { return ptr!=0; }

   // Allow '->' syntax
   inline Pointer *operator->() { return this; }
 	inline Pointer inc() { return ++ptr; }
	inline Pointer dec() { return --ptr; }
	inline Pointer add(int inInt) { return ptr+inInt; }
	inline Pointer sub(int inInt) { return ptr-inInt; }
 	inline Pointer incBy(int inDiff) { ptr+=inDiff; return ptr; }
 	inline Pointer decBy(int inDiff) { ptr-=inDiff; return ptr; }
 	inline T &postIncRef() { return *ptr++; }
 	inline T &postIncVal() { return *ptr++; }

   inline T &at(int inIndex) { return ptr[inIndex]; }
   inline void setAt(int inIndex, const T &test) { ptr[inIndex] = test; }

   inline T &__get(int inIndex) { return ptr[inIndex]; }
   inline T &__set(int inIndex, const T &inValue) { T *p = ptr+inIndex; *p = inValue; return *p; }

   inline T &get_value() { return *ptr; }
   inline T &get_ref() { return *ptr; }
   inline T &set_ref(const T &inValue) { return *ptr = inValue;  }

   operator Dynamic () const { return CreateDynamicPointer((void *)ptr); }
   #if (HXCPP_API_LEVEL >= 330)
   operator cpp::Variant () const { return CreateDynamicPointer((void *)ptr); }
   #endif

   operator T * () { return ptr; }
   T * get_raw() { return ptr; }
   const T * get_constRaw() { return ptr; }

   inline void destroy() { delete ptr; }
   inline void destroyArray() { delete [] ptr; }

   inline bool lt(Pointer inOther) { return ptr < inOther.ptr; }
   inline bool gt(Pointer inOther) { return ptr > inOther.ptr; }
   inline bool leq(Pointer inOther) { return ptr <= inOther.ptr; }
   inline bool geq(Pointer inOther) { return ptr >= inOther.ptr; }

};





template<>
class Pointer<void>
{
public:
   enum { elementSize = 0 };

   void *ptr;

   inline Pointer( ) : ptr(0) { }
   inline Pointer( const Pointer &inRHS ) : ptr(inRHS.ptr) {  }
   inline Pointer( const Dynamic &inRHS) { ptr = inRHS==null()?0: (void*)inRHS->__GetHandle(); }
   inline Pointer( const null &inRHS ) : ptr(0) { }

   template<typename O>
   inline Pointer( const O *inValue ) : ptr( (void*) inValue) { }
   //inline Pointer( T *inValue ) : ptr(inValue) { }
   inline Pointer( AutoCast inValue ) : ptr( (void*)inValue.value) { }

   inline Pointer operator=( const Pointer &inRHS ) { return ptr = inRHS.ptr; }
   inline Dynamic operator=( Dynamic &inValue )
   {
      ptr = inValue==null() ? 0 : (void*) inValue->__GetHandle();
      return inValue;
   }
   inline Dynamic operator=( null &inValue ) { ptr=0; return inValue; }
   inline AutoCast reinterpret() { return AutoCast(ptr); }
   inline RawAutoCast rawCast() { return RawAutoCast(ptr); }

   inline bool operator==( const null &inValue ) const { return ptr==0; }
   inline bool operator!=( const null &inValue ) const { return ptr!=0; }

   // Allow '->' syntax
   inline Pointer *operator->() { return this; }
 	inline Pointer inc() { return ptr; }
	inline Pointer dec() { return ptr; }
	inline Pointer add(int inInt) { return ptr; }
	inline Pointer sub(int inInt) { return ptr; }
 	inline Pointer incBy(int inDiff) { return ptr; }
 	inline Pointer decBy(int inDiff) { return ptr; }
 	inline void postIncRef() {  }
 	inline void postIncVal() {  }

   inline void at(int inIndex) {  }

   inline void __get(int inIndex) { }

   template<typename O>
   inline void __set(int inIndex, O inValue) { }

   inline void get_value() {  }
   inline void get_ref() {  }
   template<typename O> inline void set_ref(O val) {  }

   operator Dynamic () const { return CreateDynamicPointer(ptr); }
   //operator hx::Val () const { return CreateDynamicPointer((void *)ptr); }
   operator void * () { return ptr; }
   void * get_raw() { return ptr; }
   const void * get_constRaw() { return ptr; }

   inline void destroy() {  }
   inline void destroyArray() {  }

   inline bool lt(Pointer inOther) { return ptr < inOther.ptr; }
   inline bool gt(Pointer inOther) { return ptr > inOther.ptr; }
   inline bool leq(Pointer inOther) { return ptr <= inOther.ptr; }
   inline bool geq(Pointer inOther) { return ptr >= inOther.ptr; }

};




template<typename T>
inline bool operator == (const null &, Pointer<T> inPtr) { return inPtr.ptr==0; }
template<typename T>
inline bool operator != (const null &, Pointer<T> inPtr) { return inPtr.ptr!=0; }



template<typename T>
class Reference : public Pointer<T>
{
public:
   using Pointer<T>::ptr;


   inline Reference( const T &inRHS ) : Pointer<T>(&inRHS) {  }
   inline Reference( T &inRHS ) : Pointer<T>(&inRHS) {  }

   inline Reference( ) : Pointer<T>((T*)0) { }
   inline Reference( const Reference &inRHS ) : Pointer<T>(inRHS.ptr) {  }
   inline Reference( const Dynamic &inRHS) { ptr = inRHS==null()?0: (T*)inRHS->__GetHandle(); }
   inline Reference( const null &inRHS ) : Pointer<T>((T*)0) { }
   inline Reference( const T *inValue ) : Pointer<T>( (T*) inValue) { }
   //inline Reference( T *inValue ) : Pointer(inValue) { }
   inline Reference( AutoCast inValue ) : Pointer<T>( (T*)inValue.value) { }

   template<typename OTHER>
   inline Reference( const Reference<OTHER> &inOther )
   {
      // Allow reinterpret or not?
      ptr = (T*)inOther.ptr;
   }

   template<typename H>
   inline Reference( const Struct<T,H> &structVal ) : Pointer<T>( &structVal.value ) { }

   inline Reference operator=( const Reference &inRHS ) { return ptr = inRHS.ptr; }


   inline T *operator->() const { return ptr; }
   
   inline operator T &() { return *ptr; }

};

template<typename T,typename H>
Struct<T,H>::Struct( const Reference<T> &ref ) : value(*ref.ptr) { };



template<typename T>
class Function
{
public:
   T *call;

   inline Function( ) { }
   inline Function( const Function &inRHS ) : call(inRHS.call) {  }
   inline Function( const Dynamic &inRHS) { call = inRHS==null()?0: (T*)inRHS->__GetHandle(); }
   inline Function( const null &inRHS ) { call = 0; }
   inline Function( T *inValue ) : call((T*)(inValue)) { }
   //inline Function( T *inValue ) : call(inValue) { }
   inline Function( AutoCast inValue ) : call( (T*)inValue.value) { }
   inline Function( const hx::AnyCast &inValue ) : call( (T*)inValue.mPtr) { }

   template<typename FROM>
   inline static Function __new(FROM from)
   {
      return Function(from);
   }

   inline Function operator=( const Function &inRHS ) { return call = inRHS.call; }
   inline Dynamic operator=( Dynamic &inValue )
   {
      call = inValue==null() ? 0 : (T*) inValue->__GetHandle();
      return inValue;
   }
   inline Dynamic operator=( null &inValue ) { call=0; return inValue; }
   inline bool operator==( const null &inValue ) const { return call==0; }
   inline bool operator!=( const null &inValue ) const { return call!=0; }


   operator Dynamic () const { return CreateDynamicPointer((void *)call); }
   //operator hx::Val () const { return CreateDynamicPointer((void *)call); }
   operator T * () { return call; }
   operator void * () { return (void *)call; }

   inline T &get_call() { return *call; }

   inline bool lt(Function inOther) { return call < inOther.call; }
   inline bool gt(Function inOther) { return call > inOther.call; }
   inline bool leq(Function inOther) { return call <= inOther.call; }
   inline bool geq(Function inOther) { return call >= inOther.call; }

};


template<typename T>
inline bool operator == (const null &, Function<T> inPtr) { return inPtr.call==0; }
template<typename T>
inline bool operator != (const null &, Function<T> inPtr) { return inPtr.call!=0; }



class Function_obj
{
public:

	inline static AutoCast getProcAddress(String inLib, String inPrim)
   {
      return AutoCast(__hxcpp_get_proc_address(inLib, inPrim,false));
   }


   template<typename T>
	inline static AutoCast fromStaticFunction(T *inFunction)
   {
      return AutoCast(inFunction);
   }
};


class Pointer_obj
{
public:
   template<typename T>
	inline static AutoCast arrayElem(::Array<T> array, int inIndex)  { return AutoCast(&array[inIndex]); }
	inline static AutoCast arrayElem(Dynamic inVal, int inIndex)
   {
      if (inVal==null() || !inVal->__IsArray())
         return AutoCast(0);
      hx::ArrayBase *base = (hx::ArrayBase *)inVal.GetPtr();
      return AutoCast(base->GetBase() + inIndex*base->GetElementSize());
   }

   template<typename T>
	inline static AutoCast ofArray(::Array<T> array)  { return AutoCast(&array[0]); }
	inline static AutoCast ofArray(Dynamic inVal)
   {
      if (inVal==null() || !inVal->__IsArray())
         return AutoCast(0);
      hx::ArrayBase *base = (hx::ArrayBase *)inVal.GetPtr();
      return AutoCast(base->GetBase());
   }



   template<typename T>
	inline static Pointer<T> addressOf(T &value)  { return Pointer<T>(&value); }

   template<typename T>
	inline static Pointer<void> endOf(hx::ObjectPtr<T> value)  { return (void *)(value.mPtr+1); }

   template<typename T>
	inline static Pointer<T> fromPointer(T *value)  { return Pointer<T>(value); }
   template<typename T>
	inline static Pointer<T> fromPointer(const T *value)  { return Pointer<T>(value); }

   template<typename T>
	inline static Pointer<T> fromRaw(T *value)  { return Pointer<T>(value); }
   template<typename T>
	inline static Pointer<T> fromRaw(const T *value)  { return Pointer<T>(value); }
	inline static Pointer<void> fromRaw(const AutoCast &inAutoCast)  { return Pointer<void>(inAutoCast.value); }
	inline static Pointer<void> fromRaw(const RawAutoCast &inAutoCast)  { return Pointer<void>(inAutoCast.value); }


   inline static AutoCast fromHandle(Dynamic inValue, String inKind)
   {
      if (inValue==null() || (inKind!=null() && inKind!=__hxcpp_get_kind(inValue)))
         return AutoCast(0);
      return AutoCast(inValue->__GetHandle());
   }
};


class Reference_obj
{
public:

};



} // end namespace cpp

namespace hx
{
template <typename T>
T *StarOf(T &x) { return &x; }
}


#endif
