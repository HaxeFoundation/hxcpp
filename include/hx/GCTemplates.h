#ifndef HX_GC_TEMPLATES_H
#define HX_GC_TEMPLATES_H



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
template<> inline void MarkMember<int>(int &outT,hx::MarkContext *__inCtx) {  }
template<> inline void MarkMember<bool>(bool &outT,hx::MarkContext *__inCtx) {  }
template<> inline void MarkMember<double>(double &outT,hx::MarkContext *__inCtx) {  }
template<> inline void MarkMember<float>(float &outT,hx::MarkContext *__inCtx) {  }
template<> inline void MarkMember<String>(String &outT,hx::MarkContext *__inCtx)
{
   HX_MARK_STRING(outT.__s);
}
template<> inline void MarkMember<Void>(Void &outT,hx::MarkContext *__inCtx) {  }


// Template used to register and initialise the statics in the one call.
//  Do nothing...
template<typename T> inline T &Static(T &t) {  return t; }


} // end namespace hx




#endif
