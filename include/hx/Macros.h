#ifndef HX_MACROS_H
#define HX_MACROS_H




#define HX_DO_RTTI_BASE \
   bool __Is(hx::Object *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; } \


#define HX_DO_RTTI \
   HX_DO_RTTI_BASE \
   static hx::ObjectPtr<Class_obj> __mClass; \
   hx::ObjectPtr<Class_obj > __GetClass() const { return __mClass; } \
   static hx::ObjectPtr<Class_obj> &__SGetClass() { return __mClass; } \
   Dynamic __Field(const ::String &inString); \
   void __GetFields(Array< ::String> &outFields); \
   Dynamic __SetField(const ::String &inString,const Dynamic &inValue); \
   virtual int __GetType() const { return vtClass; } \
   inline operator super *() { return this; } 


#define HX_DO_ENUM_RTTI_INTERNAL \
   HX_DO_RTTI_BASE  \
   Dynamic __Field(const ::String &inString); \
   static int __FindIndex(::String inName); \
   static int __FindArgCount(::String inName);

#define HX_DO_ENUM_RTTI \
   HX_DO_ENUM_RTTI_INTERNAL \
   static hx::ObjectPtr<Class_obj> __mClass; \
   hx::ObjectPtr<Class_obj > __GetClass() const { return __mClass; } \
   static hx::ObjectPtr<Class_obj> &__SGetClass() { return __mClass; }


#define HX_DECLARE_IMPLEMENT_DYNAMIC  hx::FieldMap *__mDynamicFields; \
    hx::FieldMap *__GetFieldMap() { return __mDynamicFields; }


#define HX_INIT_IMPLEMENT_DYNAMIC __mDynamicFields = hx::FieldMapCreate();

#define HX_MARK_DYNAMIC hx::FieldMapMark(__mDynamicFields HX_MARK_ADD_ARG);

#define HX_CHECK_DYNAMIC_GET_FIELD(inName) \
   { Dynamic d;  if (hx::FieldMapGet(__mDynamicFields,inName,d)) return d; }

#define HX_CHECK_DYNAMIC_GET_INT_FIELD(inID) \
   { Dynamic d;  if (hx::FieldMapGet(__mDynamicFields,inID,d)) return d; }

#define HX_DYNAMIC_SET_FIELD(inName,inValue) hx::FieldMapSet(__mDynamicFields,inName,inValue) 

#define HX_APPEND_DYNAMIC_FIELDS(outFields) hx::FieldMapAppendFields(__mDynamicFields,outFields)

#define HX_ARR_LIST0
#define HX_ARR_LIST1 inArgs[0]
#define HX_ARR_LIST2 HX_ARR_LIST1,inArgs[1]
#define HX_ARR_LIST3 HX_ARR_LIST2,inArgs[2]
#define HX_ARR_LIST4 HX_ARR_LIST3,inArgs[3]
#define HX_ARR_LIST5 HX_ARR_LIST4,inArgs[4]
#define HX_ARR_LIST6 HX_ARR_LIST5,inArgs[5]
#define HX_ARR_LIST7 HX_ARR_LIST6,inArgs[6]
#define HX_ARR_LIST8 HX_ARR_LIST7,inArgs[8]
#define HX_ARR_LIST9 HX_ARR_LIST8,inArgs[9]
#define HX_ARR_LIST10 HX_ARR_LIST9,inArgs[10]
#define HX_ARR_LIST11 HX_ARR_LIST10,inArgs[11]
#define HX_ARR_LIST12 HX_ARR_LIST11,inArgs[12]

#define HX_DYNAMIC_ARG_LIST0
#define HX_DYNAMIC_ARG_LIST1 const Dynamic &inArg0
#define HX_DYNAMIC_ARG_LIST2 HX_DYNAMIC_ARG_LIST1,const Dynamic &inArg1
#define HX_DYNAMIC_ARG_LIST3 HX_DYNAMIC_ARG_LIST2,const Dynamic &inArg2
#define HX_DYNAMIC_ARG_LIST4 HX_DYNAMIC_ARG_LIST3,const Dynamic &inArg3
#define HX_DYNAMIC_ARG_LIST5 HX_DYNAMIC_ARG_LIST4,const Dynamic &inArg4
#define HX_DYNAMIC_ARG_LIST6 HX_DYNAMIC_ARG_LIST5,const Dynamic &inArg5
#define HX_DYNAMIC_ARG_LIST7 HX_DYNAMIC_ARG_LIST6,const Dynamic &inArg6
#define HX_DYNAMIC_ARG_LIST8 HX_DYNAMIC_ARG_LIST7,const Dynamic &inArg7
#define HX_DYNAMIC_ARG_LIST9 HX_DYNAMIC_ARG_LIST8,const Dynamic &inArg8
#define HX_DYNAMIC_ARG_LIST10 HX_DYNAMIC_ARG_LIST9,const Dynamic &inArg9
#define HX_DYNAMIC_ARG_LIST11 HX_DYNAMIC_ARG_LIST10,const Dynamic &inArg10
#define HX_DYNAMIC_ARG_LIST12 HX_DYNAMIC_ARG_LIST11,const Dynamic &inArg11

#define HX_ARG_LIST0
#define HX_ARG_LIST1 inArg0
#define HX_ARG_LIST2 HX_ARG_LIST1,inArg1
#define HX_ARG_LIST3 HX_ARG_LIST2,inArg2
#define HX_ARG_LIST4 HX_ARG_LIST3,inArg3
#define HX_ARG_LIST5 HX_ARG_LIST4,inArg4
#define HX_ARG_LIST6 HX_ARG_LIST5,inArg5
#define HX_ARG_LIST7 HX_ARG_LIST6,inArg6
#define HX_ARG_LIST8 HX_ARG_LIST7,inArg7
#define HX_ARG_LIST9 HX_ARG_LIST8,inArg8
#define HX_ARG_LIST10 HX_ARG_LIST9,inArg9
#define HX_ARG_LIST11 HX_ARG_LIST10,inArg10
#define HX_ARG_LIST12 HX_ARG_LIST11,inArg11


#define HX_DEFINE_DYNAMIC_FUNC(class,N,func,ret,array_list,dynamic_arg_list,arg_list) \
struct class##func : public hx::Object \
{ \
   int __GetType() const { return vtFunction; } \
   int __ArgCount() const { return N; } \
   hx::ObjectPtr<class> mThis; \
   class##func(class *inThis) : mThis(inThis) { } \
   ::String __ToString() const{ return HX_CSTRING(#func); } \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(mThis); } \
   void *__GetHandle() const { return mThis.GetPtr(); } \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      ret mThis->func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      ret mThis->func(arg_list); return Dynamic(); \
   } \
}; \
Dynamic class::func##_dyn() \
{\
   return new class##func(this); \
}


#define DELEGATE_0(ret,func) ret func() { return mDelegate->func(); }
#define CDELEGATE_0(ret,func) ret func() const { return mDelegate->func(); }
#define DELEGATE_1(ret,func,arg1) ret func(arg1 _a1) { return mDelegate->func(_a1); }
#define CDELEGATE_1(ret,func,arg1) ret func(arg1 _a1) const { return mDelegate->func(_a1); }
#define DELEGATE_2(ret,func,arg1,arg2) ret func(arg1 _a1,arg2 _a2) { return mDelegate->func(_a1,_a2); }







#define HX_DECLARE_DYNAMIC_FUNC(func,dynamic_arg_list) \
   Dynamic func##_dyn(dynamic_arg_list);

#define STATIC_HX_DECLARE_DYNAMIC_FUNC(func,dynamic_arg_list) \
   static Dynamic func##_dyn(dynamic_arg_list);


#define HX_DEFINE_DYNAMIC_FUNC0(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,0,func,ret,HX_ARR_LIST0,HX_DYNAMIC_ARG_LIST0,HX_ARG_LIST0)
#define HX_DEFINE_DYNAMIC_FUNC1(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,1,func,ret,HX_ARR_LIST1,HX_DYNAMIC_ARG_LIST1,HX_ARG_LIST1)
#define HX_DEFINE_DYNAMIC_FUNC2(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,2,func,ret,HX_ARR_LIST2,HX_DYNAMIC_ARG_LIST2,HX_ARG_LIST2)
#define HX_DEFINE_DYNAMIC_FUNC3(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,3,func,ret,HX_ARR_LIST3,HX_DYNAMIC_ARG_LIST3,HX_ARG_LIST3)
#define HX_DEFINE_DYNAMIC_FUNC4(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,4,func,ret,HX_ARR_LIST4,HX_DYNAMIC_ARG_LIST4,HX_ARG_LIST4)
#define HX_DEFINE_DYNAMIC_FUNC5(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,5,func,ret,HX_ARR_LIST5,HX_DYNAMIC_ARG_LIST5,HX_ARG_LIST5)
#define HX_DEFINE_DYNAMIC_FUNC6(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,6,func,ret,HX_ARR_LIST6,HX_DYNAMIC_ARG_LIST6,HX_ARG_LIST6)
#define HX_DEFINE_DYNAMIC_FUNC7(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,7,func,ret,HX_ARR_LIST7,HX_DYNAMIC_ARG_LIST7,HX_ARG_LIST7)
#define HX_DEFINE_DYNAMIC_FUNC8(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,8,func,ret,HX_ARR_LIST8,HX_DYNAMIC_ARG_LIST8,HX_ARG_LIST8)
#define HX_DEFINE_DYNAMIC_FUNC9(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,9,func,ret,HX_ARR_LIST9,HX_DYNAMIC_ARG_LIST9,HX_ARG_LIST9)
#define HX_DEFINE_DYNAMIC_FUNC10(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,10,func,ret,HX_ARR_LIST10,HX_DYNAMIC_ARG_LIST10,HX_ARG_LIST10)
#define HX_DEFINE_DYNAMIC_FUNC11(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,11,func,ret,HX_ARR_LIST11,HX_DYNAMIC_ARG_LIST11,HX_ARG_LIST11)
#define HX_DEFINE_DYNAMIC_FUNC12(class,func,ret) \
          HX_DEFINE_DYNAMIC_FUNC(class,12,func,ret,HX_ARR_LIST12,HX_DYNAMIC_ARG_LIST12,HX_ARG_LIST12)



#define STATIC_HX_DEFINE_DYNAMIC_FUNC(class,N,func,ret,array_list,dynamic_arg_list,arg_list) \
struct class##func : public hx::Object \
{ \
   class##func() { } \
   int __GetType() const { return vtFunction; } \
   int __ArgCount() const { return N; } \
   ::String __ToString() const{ return HX_CSTRING(#func) ; } \
   Dynamic __Run(const Array<Dynamic> &inArgs) \
   { \
      ret class::func(array_list); return Dynamic(); \
   } \
   Dynamic __run(dynamic_arg_list) \
   { \
      ret class::func(arg_list); return Dynamic(); \
   } \
}; \
Dynamic class::func##_dyn() \
{\
   return new class##func(); \
}



#define STATIC_HX_DEFINE_DYNAMIC_FUNC0(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,0,func,ret,HX_ARR_LIST0,HX_DYNAMIC_ARG_LIST0,HX_ARG_LIST0)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC1(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,1,func,ret,HX_ARR_LIST1,HX_DYNAMIC_ARG_LIST1,HX_ARG_LIST1)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC2(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,2,func,ret,HX_ARR_LIST2,HX_DYNAMIC_ARG_LIST2,HX_ARG_LIST2)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC3(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,3,func,ret,HX_ARR_LIST3,HX_DYNAMIC_ARG_LIST3,HX_ARG_LIST3)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC4(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,4,func,ret,HX_ARR_LIST4,HX_DYNAMIC_ARG_LIST4,HX_ARG_LIST4)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC5(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,5,func,ret,HX_ARR_LIST5,HX_DYNAMIC_ARG_LIST5,HX_ARG_LIST5)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC6(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,6,func,ret,HX_ARR_LIST6,HX_DYNAMIC_ARG_LIST6,HX_ARG_LIST6)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC7(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,7,func,ret,HX_ARR_LIST7,HX_DYNAMIC_ARG_LIST7,HX_ARG_LIST7)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC8(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,8,func,ret,HX_ARR_LIST8,HX_DYNAMIC_ARG_LIST8,HX_ARG_LIST8)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC9(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,9,func,ret,HX_ARR_LIST9,HX_DYNAMIC_ARG_LIST9,HX_ARG_LIST9)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC10(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,10,func,ret,HX_ARR_LIST10,HX_DYNAMIC_ARG_LIST10,HX_ARG_LIST10)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC11(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,11,func,ret,HX_ARR_LIST11,HX_DYNAMIC_ARG_LIST11,HX_ARG_LIST11)
#define STATIC_HX_DEFINE_DYNAMIC_FUNC12(class,func,ret) \
          STATIC_HX_DEFINE_DYNAMIC_FUNC(class,12,func,ret,HX_ARR_LIST12,HX_DYNAMIC_ARG_LIST12,HX_ARG_LIST12)



#define HX_DEFINE_CREATE_ENUM(enum_obj) \
static Dynamic Create##enum_obj(::String inName,hx::DynamicArray inArgs) \
{ \
   int idx =  enum_obj::__FindIndex(inName); \
   if (idx<0) throw HX_INVALID_CONSTRUCTOR; \
   int count =  enum_obj::__FindArgCount(inName); \
   int args = inArgs.GetPtr() ? inArgs.__length() : 0; \
   if (args!=count)  throw HX_INVALID_ARG_COUNT; \
   return hx::CreateEnum<enum_obj >(inName,idx,inArgs); \
}


#define HX_DECLARE_CLASS0(klass) \
	class klass##_obj; \
	typedef hx::ObjectPtr<klass##_obj> klass;
#define HX_DECLARE_CLASS1(ns1,klass) namespace ns1 { HX_DECLARE_CLASS0(klass) }
#define HX_DECLARE_CLASS2(ns2,ns1,klass) namespace ns2 { HX_DECLARE_CLASS1(ns1,klass) }
#define HX_DECLARE_CLASS3(ns3,ns2,ns1,klass) namespace ns3 { HX_DECLARE_CLASS2(ns2,ns1,klass) }
#define HX_DECLARE_CLASS4(ns4,ns3,ns2,ns1,klass) namespace ns4 { HX_DECLARE_CLASS3(ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS5(ns5,ns4,ns3,ns2,ns1,klass) namespace ns5 { HX_DECLARE_CLASS4(ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS6(ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns6 { HX_DECLARE_CLASS5(ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns7 { HX_DECLARE_CLASS6(ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns8 { HX_DECLARE_CLASS7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define HX_DECLARE_CLASS9(ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns9 { HX_DECLARE_CLASS8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }




#define HX_DYNAMIC_CALL(ret,func,array_args,dyn_arg_list,arg_list) \
   Dynamic __Run(const Array<Dynamic> &inArgs) { ret func( array_args ); return null();} \
   Dynamic __run(dyn_arg_list) { ret func( arg_list ); return null();}

#define HX_DYNAMIC_CALL0(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST0,HX_DYNAMIC_ARG_LIST0,HX_ARG_LIST0)
#define HX_DYNAMIC_CALL1(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST1,HX_DYNAMIC_ARG_LIST1,HX_ARG_LIST1)
#define HX_DYNAMIC_CALL2(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST2,HX_DYNAMIC_ARG_LIST2,HX_ARG_LIST2)
#define HX_DYNAMIC_CALL3(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST3,HX_DYNAMIC_ARG_LIST3,HX_ARG_LIST3)
#define HX_DYNAMIC_CALL4(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST4,HX_DYNAMIC_ARG_LIST4,HX_ARG_LIST4)
#define HX_DYNAMIC_CALL5(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST5,HX_DYNAMIC_ARG_LIST5,HX_ARG_LIST5)
#define HX_DYNAMIC_CALL6(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST6,HX_DYNAMIC_ARG_LIST6,HX_ARG_LIST6)
#define HX_DYNAMIC_CALL7(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST7,HX_DYNAMIC_ARG_LIST7,HX_ARG_LIST7)
#define HX_DYNAMIC_CALL8(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST8,HX_DYNAMIC_ARG_LIST8,HX_ARG_LIST8)
#define HX_DYNAMIC_CALL9(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST9,HX_DYNAMIC_ARG_LIST9,HX_ARG_LIST9)
#define HX_DYNAMIC_CALL10(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST10,HX_DYNAMIC_ARG_LIST10,HX_ARG_LIST10)
#define HX_DYNAMIC_CALL11(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST11,HX_DYNAMIC_ARG_LIST11,HX_ARG_LIST11)
#define HX_DYNAMIC_CALL12(ret,func) HX_DYNAMIC_CALL(ret,func,HX_ARR_LIST12,HX_DYNAMIC_ARG_LIST12,HX_ARG_LIST12)


#define HX_BEGIN_DEFAULT_FUNC(name,t0) \
	namespace { \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   hx::ObjectPtr<t0> __this; \
   name(hx::ObjectPtr<t0> __0 = null()) : __this(__0) {}


#define HX_END_DEFAULT_FUNC \
}


#define HX_BEGIN_LOCAL_FUNC0(name) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   name() {}

#define HX_BEGIN_LOCAL_FUNC1(name,t0,v0) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); } \
   name(t0 __0) : v0(__0) {}

#define HX_BEGIN_LOCAL_FUNC2(name,t0,v0,t1,v1) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); } \
   t0 v0; t1 v1;\
   name(t0 __0,t1 __1) : v0(__0), v1(__1) {}

#define HX_BEGIN_LOCAL_FUNC3(name,t0,v0,t1,v1,t2,v2) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2;\
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); } \
   name(t0 __0,t1 __1,t2 __2) : v0(__0), v1(__1), v2(__2) {}

#define HX_BEGIN_LOCAL_FUNC4(name,t0,v0,t1,v1,t2,v2,t3,v3) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3) : v0(__0), v1(__1), v2(__2), v3(__3) {}

#define HX_BEGIN_LOCAL_FUNC5(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4) {}

#define HX_BEGIN_LOCAL_FUNC6(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5) {}

#define HX_BEGIN_LOCAL_FUNC7(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); HX_MARK_MEMBER(v6);  } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6) {}


#define HX_BEGIN_LOCAL_FUNC8(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); HX_MARK_MEMBER(v6); HX_MARK_MEMBER(v7); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7) {}

#define HX_BEGIN_LOCAL_FUNC9(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); HX_MARK_MEMBER(v6); HX_MARK_MEMBER(v7); HX_MARK_MEMBER(v8); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8) {}

#define HX_BEGIN_LOCAL_FUNC10(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); HX_MARK_MEMBER(v6); HX_MARK_MEMBER(v7); HX_MARK_MEMBER(v8); HX_MARK_MEMBER(v9); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9) {}

#define HX_BEGIN_LOCAL_FUNC11(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9,t10,v10) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; t10 v10;  \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); HX_MARK_MEMBER(v6); HX_MARK_MEMBER(v7); HX_MARK_MEMBER(v8); HX_MARK_MEMBER(v9); HX_MARK_MEMBER(v10); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9, t10 __10) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9), v10(__10) {}

#define HX_BEGIN_LOCAL_FUNC12(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9,t10,v10,t11,v11) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; t10 v10; t11 v11; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); HX_MARK_MEMBER(v6); HX_MARK_MEMBER(v7); HX_MARK_MEMBER(v8); HX_MARK_MEMBER(v9); HX_MARK_MEMBER(v10); HX_MARK_MEMBER(v11); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9, t10 __10, t11 __11) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9), v10(__10), v11(__11) {}

#define HX_BEGIN_LOCAL_FUNC13(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9,t10,v10,t11,v11,t12,v12) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; t10 v10; t11 v11; t12 v12; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); HX_MARK_MEMBER(v6); HX_MARK_MEMBER(v7); HX_MARK_MEMBER(v8); HX_MARK_MEMBER(v9); HX_MARK_MEMBER(v10); HX_MARK_MEMBER(v11); HX_MARK_MEMBER(v12);  } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9, t10 __10, t11 __11, t12 __12) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9), v10(__10), v11(__11), v12(__12) {}


#define HX_BEGIN_LOCAL_FUNC14(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9,t10,v10,t11,v11,t12,v12,t13,v13) \
   struct name : public hx::Object { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; t10 v10; t11 v11; t12 v12; t13 v13; \
   void __Mark(HX_MARK_PARAMS) { HX_MARK_MEMBER(v0); HX_MARK_MEMBER(v1); HX_MARK_MEMBER(v2); HX_MARK_MEMBER(v3); HX_MARK_MEMBER(v4); HX_MARK_MEMBER(v5); HX_MARK_MEMBER(v6); HX_MARK_MEMBER(v7); HX_MARK_MEMBER(v8); HX_MARK_MEMBER(v9); HX_MARK_MEMBER(v10); HX_MARK_MEMBER(v11); HX_MARK_MEMBER(v12); HX_MARK_MEMBER(v13); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9, t10 __10, t11 __11, t12 __12, t13 __13) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9), v10(__10), v11(__11), v12(__12), v13(__13) {}


#define HX_END_LOCAL_FUNC0(ret) HX_DYNAMIC_CALL0(ret,run) };
#define HX_END_LOCAL_FUNC1(ret) HX_DYNAMIC_CALL1(ret,run) };
#define HX_END_LOCAL_FUNC2(ret) HX_DYNAMIC_CALL2(ret,run) };
#define HX_END_LOCAL_FUNC3(ret) HX_DYNAMIC_CALL3(ret,run) };
#define HX_END_LOCAL_FUNC4(ret) HX_DYNAMIC_CALL4(ret,run) };
#define HX_END_LOCAL_FUNC5(ret) HX_DYNAMIC_CALL5(ret,run) };
#define HX_END_LOCAL_FUNC6(ret) HX_DYNAMIC_CALL6(ret,run) };
#define HX_END_LOCAL_FUNC7(ret) HX_DYNAMIC_CALL7(ret,run) };
#define HX_END_LOCAL_FUNC8(ret) HX_DYNAMIC_CALL8(ret,run) };
#define HX_END_LOCAL_FUNC9(ret) HX_DYNAMIC_CALL9(ret,run) };
#define HX_END_LOCAL_FUNC10(ret) HX_DYNAMIC_CALL10(ret,run) };
#define HX_END_LOCAL_FUNC11(ret) HX_DYNAMIC_CALL11(ret,run) };
#define HX_END_LOCAL_FUNC12(ret) HX_DYNAMIC_CALL12(ret,run) };



#ifdef HX_INTERNAL_GC
namespace hx {
extern void SetTopOfStack(int *inTopOfStack,bool);
}
#define HX_TOP_OF_STACK \
		int t0 = 99; \
		hx::SetTopOfStack(&t0,false);
#else
	#define HX_TOP_OF_STACK
#endif

#ifdef ANDROID
// Java Main....
#include <jni.h>
#include <hx/Thread.h>

#ifdef __GNUC__
 #define GCC_EXTRA __attribute__ ((visibility("default")))
#else
 #define GCC_EXTRA
#endif

#define HX_BEGIN_MAIN \
extern "C" GCC_EXTRA JNIEXPORT void JNICALL Java_org_haxe_HXCPP_main(JNIEnv * env) { \
	HX_TOP_OF_STACK \
        try { \
	hx::Boot(); \
	__boot_all();


#define HX_END_MAIN \
        } catch (Dynamic e) { \
	  __hx_dump_stack(); \
          __android_log_print(ANDROID_LOG_ERROR, "Exception", "%s", e->toString().__CStr()); \
        }\
}

#else
// Console Main ...

#define HX_BEGIN_MAIN \
\
int main(int argc,char **argv){ \
	HX_TOP_OF_STACK \
	hx::Boot(); \
	try{ \
		__boot_all();

#define HX_END_MAIN \
	} \
	catch (Dynamic e){ \
		__hx_dump_stack(); \
		printf("Error : %s\n",e->toString().__CStr()); \
	} \
	return 0; \
}

#endif


// Run as library
#define HX_BEGIN_LIB_MAIN \
extern "C" {\
\
void __hxcpp_lib_main() \
{ \
	HX_TOP_OF_STACK \
	hx::Boot(); \
	__boot_all();

#define HX_END_LIB_MAIN \
} }



#endif

