#ifndef HX_GC_TYPE_INFERENCE_H
#define HX_GC_TYPE_INFERENCE_H


// These templates allow you to call MarkMember(x) or VisitMember(x) and the
// compiler will direct the call to the correct function

namespace hx
{


template<typename T> inline void MarkMember(T &outT,hx::MarkContext *__inCtx) { }

template<typename T> inline void MarkMember(hx::ObjectPtr<T> &outT,hx::MarkContext *__inCtx)
{
   HX_MARK_OBJECT(outT.mPtr);
}
template<> inline void MarkMember(Dynamic &outT,hx::MarkContext *__inCtx)
{
   HX_MARK_OBJECT(outT.mPtr);
}
template<typename T> inline void MarkMember(Array<T> &outT,hx::MarkContext *__inCtx)
{
   HX_MARK_OBJECT(outT.mPtr);
}
template<> inline void MarkMember<hx::Object *>(hx::Object *&outT,hx::MarkContext *__inCtx)
{
   HX_MARK_OBJECT(outT);
}
template<> inline void MarkMember<cpp::Variant>(cpp::Variant &outT,hx::MarkContext *__inCtx)
{
   outT.mark(__inCtx);
}
template<typename T> inline void MarkMember(hx::Native<T> &outT,hx::MarkContext *__inCtx)
{
   if (outT.ptr)
   {
      hx::Object *ptr = outT.ptr->__GetRealObject();
      HX_MARK_OBJECT(ptr);
   }
}

template<> inline void MarkMember<int>(int &outT,hx::MarkContext *__inCtx) {  }
template<> inline void MarkMember<bool>(bool &outT,hx::MarkContext *__inCtx) {  }
template<> inline void MarkMember<double>(double &outT,hx::MarkContext *__inCtx) {  }
template<> inline void MarkMember<float>(float &outT,hx::MarkContext *__inCtx) {  }
template<> inline void MarkMember<String>(String &outT,hx::MarkContext *__inCtx)
{
   HX_MARK_STRING(outT.__s);
}
template<> inline void MarkMember<Void>(Void &outT,hx::MarkContext *__inCtx) {  }




template<typename T> inline void MarkMemberArray(T *,int, hx::MarkContext *__inCtx)
{
   //*(int *)0=0;
}
template<> inline void MarkMemberArray<String>(String *ioStrings,int inLen,hx::MarkContext *__inCtx)
{
   hx::MarkStringArray(ioStrings,inLen,__inCtx);
}
template<typename T> inline void MarkMemberArray(hx::ObjectPtr<T> *inObjects, int inLen, hx::MarkContext *__inCtx)
{
   hx::MarkObjectArray( (hx::Object **)inObjects ,inLen,__inCtx);
}
template<> inline void MarkMemberArray(Dynamic *outT,int inLen, hx::MarkContext *__inCtx)
{
   hx::MarkObjectArray( (hx::Object **)outT ,inLen,__inCtx);
}
template<> inline void MarkMemberArray(hx::Object **outT,int inLen, hx::MarkContext *__inCtx)
{
   hx::MarkObjectArray( outT ,inLen,__inCtx);
}
template<typename T> inline void MarkMemberArray(Array<T> *outT,int inLen,hx::MarkContext *__inCtx)
{
   hx::MarkObjectArray( (hx::Object **)outT ,inLen,__inCtx);
}




#ifdef HXCPP_VISIT_ALLOCS
template<typename T> inline void VisitMember(T &outT,hx::VisitContext *__inCtx) { }

template<typename T> inline void VisitMember(hx::ObjectPtr<T> &outT,hx::VisitContext *__inCtx)
{
   HX_VISIT_OBJECT(outT.mPtr);
}
template<> inline void VisitMember(Dynamic &outT,hx::VisitContext *__inCtx)
{
   HX_VISIT_OBJECT(outT.mPtr);
}
template<> inline void VisitMember<hx::Object *>(hx::Object *&outT,hx::VisitContext *__inCtx)
{
   HX_VISIT_OBJECT(outT);
}
template<typename T> inline void VisitMember(Array<T> &outT,hx::VisitContext *__inCtx)
{
   HX_VISIT_OBJECT(outT.mPtr);
}
template<> inline void VisitMember(cpp::Variant &outT,hx::VisitContext *__inCtx)
{
   outT.visit(__inCtx);
}
template<typename T> inline void VisitMember(hx::Native<T> &outT,hx::VisitContext *__inCtx)
{
   if (outT.ptr)
   {
      hx::Object *ptr0 = outT.ptr->__GetRealObject();
      if (ptr0)
      {
         hx::Object *ptr1 = ptr0;
         HX_VISIT_OBJECT(ptr1);
         size_t delta = ( (char *)ptr1 - (char *)ptr0 );
         if (delta)
            outT.ptr = (T)( (char *)outT.ptr + delta );
      }
   }
}


template<> inline void VisitMember<int>(int &outT,hx::VisitContext *__inCtx) {  }
template<> inline void VisitMember<bool>(bool &outT,hx::VisitContext *__inCtx) {  }
template<> inline void VisitMember<double>(double &outT,hx::VisitContext *__inCtx) {  }
template<> inline void VisitMember<float>(float &outT,hx::VisitContext *__inCtx) {  }
template<> inline void VisitMember<String>(String &outT,hx::VisitContext *__inCtx)
{
   HX_VISIT_STRING(outT.__s);
}
template<> inline void VisitMember<Void>(Void &outT,hx::VisitContext *__inCtx) {  }
#endif



// Template used to register and initialise the statics in the one call.
//  Do nothing...
template<typename T> inline T &Static(T &t) {  return t; }


} // end namespace hx




#endif
