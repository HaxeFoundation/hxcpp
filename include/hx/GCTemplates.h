#ifndef HX_GC_TEMPLATES_H
#define HX_GC_TEMPLATES_H



namespace hx
{


template<typename T> inline void MarkMember(T &outT HX_MARK_ADD_PARAMS) { }

template<typename T> inline void MarkMember(hx::ObjectPtr<T> &outT HX_MARK_ADD_PARAMS)
{
	HX_MARK_OBJECT(outT.mPtr);
}
template<> inline void MarkMember(Dynamic &outT HX_MARK_ADD_PARAMS)
{
	HX_MARK_OBJECT(outT.mPtr);
}
template<typename T> inline void MarkMember(Array<T> &outT HX_MARK_ADD_PARAMS)
{
	HX_MARK_OBJECT(outT.mPtr);
}
template<> inline void MarkMember<int>(int &outT HX_MARK_ADD_PARAMS) {  }
template<> inline void MarkMember<bool>(bool &outT HX_MARK_ADD_PARAMS) {  }
template<> inline void MarkMember<double>(double &outT HX_MARK_ADD_PARAMS) {  }
template<> inline void MarkMember<String>(String &outT HX_MARK_ADD_PARAMS)
{
   HX_MARK_STRING(outT.__s);
}
template<> inline void MarkMember<Void>(Void &outT HX_MARK_ADD_PARAMS) {  }


// Template used to register and initialise the statics in the one call.
template<typename T> inline T &Static(T &inPtr) { hx::RegisterObject((hx::Object **)&inPtr); return inPtr; }

// Make sure we get the "__s" pointer
template<> inline String &Static<String>(String &inString)
   { hx::RegisterString(&inString.__s); return inString; }

// Do nothing
template<> inline int &Static<int>(int &inPtr) { return inPtr; }
template<> inline bool &Static<bool>(bool &inPtr) { return inPtr; }
template<> inline double &Static<double>(double &inPtr) { return inPtr; }
template<> inline null &Static<null>(null &inPtr) { return inPtr; }


} // end namespace hx




#endif
