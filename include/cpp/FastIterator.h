#ifndef INCLUDED_cpp_FastIterator
#define INCLUDED_cpp_FastIterator

namespace cpp
{

class HXCPP_EXTERN_CLASS_ATTRIBUTES IteratorBase : public hx::Object
{
public:
   hx::Val __Field(const String &inString ,hx::PropertyAccess inCallProp);
   virtual bool hasNext() = 0;
   virtual Dynamic _dynamicNext() = 0;

   Dynamic hasNext_dyn( );
   Dynamic next_dyn( );
   Dynamic _dynamicNext_dyn( );
};


template<typename T>
class HXCPP_EXTERN_CLASS_ATTRIBUTES FastIterator_obj : public IteratorBase
{
public:
   virtual bool hasNext() = 0;
   virtual T next() = 0;

   virtual Dynamic _dynamicNext() { return next(); }
};



template<typename T>
class HXCPP_EXTERN_CLASS_ATTRIBUTES DynamicIterator : public FastIterator_obj<T>
{
public:
   Dynamic mNext;
   Dynamic mHasNext;

   DynamicIterator(Dynamic inValue)
   {
      mNext = inValue->__Field(HX_CSTRING("next"), HX_PROP_ALWAYS);
      mHasNext = inValue->__Field(HX_CSTRING("hasNext"), HX_PROP_ALWAYS);
   }

   bool hasNext() { return mHasNext(); }
   T next() { return mNext(); }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER_NAME(mNext,"mNext");
      HX_MARK_MEMBER_NAME(mHasNext,"mHasNext");
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER_NAME(mNext,"mNext");
      HX_VISIT_MEMBER_NAME(mHasNext,"mHasNext");
   }
   #endif

};


template<typename T>
FastIterator_obj<T> *CreateFastIterator(Dynamic inValue)
{
   FastIterator_obj<T> *result = dynamic_cast< FastIterator_obj<T> *>(inValue.GetPtr());
   if (result) return result;
   return new DynamicIterator<T>(inValue);
}

template<typename T>
class HXCPP_EXTERN_CLASS_ATTRIBUTES StringIterator : public cpp::FastIterator_obj<T>
{
public:
   String value;
   int    pos;

   StringIterator(const String &inValue) : value(inValue), pos(0) { }

   bool hasNext() { return pos<value.length; }
   void __Mark(hx::MarkContext *__inCtx)
   {
      cpp::FastIterator_obj<T>::__Mark(__inCtx);
      HX_MARK_MEMBER_NAME(value,"value");
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      cpp::FastIterator_obj<T>::__Visit(__inCtx);
      HX_VISIT_MEMBER_NAME(value,"value");
   }
   #endif
};

}









#endif
