#ifndef HX_MACROS_H
#define HX_MACROS_H

#ifdef HX_WINDOWS
#define STRING(s,len) String((L"\xffff\xffff" s)+2,len)
#else
#define STRING(s,len) String( (L"\xffffffff" s) + 1 ,len)
#endif

#define STRING_UTF8(s,len) String( ("\xff\xff\xff\xff" s) + 4 ,len)


#define DO_RTTI_BASE(Parent) \
   bool __Is(hxObject *inObj) const { return dynamic_cast<OBJ_ *>(inObj)!=0; } \


#define DO_RTTI_NO_TYPE(Parent) \
   DO_RTTI_BASE(Parent) \
   static hxObjectPtr<Class_obj> __mClass; \
   hxObjectPtr<Class_obj > __GetClass() const { return __mClass; } \
   static hxObjectPtr<Class_obj> &__SGetClass() { return __mClass; } \
   Dynamic __Field(const String &inString); \
   Dynamic __IField(int inFieldID); \
   void *GetRoot() { return this; } \
   void __GetFields(Array<String> &outFields); \
   Dynamic __SetField(const String &inString,const Dynamic &inValue);

#define DO_RTTI_(Parent) \
   DO_RTTI_NO_TYPE(Parent) \
   virtual int __GetType() const { return vtClass; }


#define DO_RTTI DO_RTTI_(hxObject)

#define DO_ENUM_RTTI_INTERNAL DO_RTTI_BASE(hxObject)  \
   Dynamic __Field(const String &inString); \
   static int __FindIndex(String inName); \
   static int __FindArgCount(String inName);

#define DO_ENUM_RTTI DO_ENUM_RTTI_INTERNAL \
   static hxObjectPtr<Class_obj> __mClass; \
   hxObjectPtr<Class_obj > __GetClass() const { return __mClass; } \
   static hxObjectPtr<Class_obj> &__SGetClass() { return __mClass; }

#define INTERFACE_DEF


#define DECLARE_IMPLEMENT_DYNAMIC  hxFieldMap *__mDynamicFields;

#define INIT_IMPLEMENT_DYNAMIC __mDynamicFields = hxFieldMapCreate();

#define MARK_DYNAMIC hxFieldMapMark(__mDynamicFields);

#define CHECK_DYNAMIC_GET_FIELD(inName) \
   { Dynamic d;  if (hxFieldMapGet(__mDynamicFields,inName,d)) return d; }

#define CHECK_DYNAMIC_GET_INT_FIELD(inID) \
   { Dynamic d;  if (hxFieldMapGet(__mDynamicFields,inID,d)) return d; }

#define DYNAMIC_SET_FIELD(inName,inValue) hxFieldMapSet(__mDynamicFields,inName,inValue) 

#define APPEND_DYNAMIC_FIELDS(outFields) hxFieldMapAppendFields(__mDynamicFields,outFields)

#define ARRAY_LIST0
#define ARRAY_LIST1 inArgs[0]
#define ARRAY_LIST2 ARRAY_LIST1,inArgs[1]
#define ARRAY_LIST3 ARRAY_LIST2,inArgs[2]
#define ARRAY_LIST4 ARRAY_LIST3,inArgs[3]
#define ARRAY_LIST5 ARRAY_LIST4,inArgs[4]
#define ARRAY_LIST6 ARRAY_LIST5,inArgs[5]
#define ARRAY_LIST7 ARRAY_LIST6,inArgs[6]
#define ARRAY_LIST8 ARRAY_LIST7,inArgs[8]
#define ARRAY_LIST9 ARRAY_LIST8,inArgs[9]
#define ARRAY_LIST10 ARRAY_LIST9,inArgs[10]
#define ARRAY_LIST11 ARRAY_LIST10,inArgs[11]
#define ARRAY_LIST12 ARRAY_LIST11,inArgs[12]

#define DYNAMIC_ARG_LIST0
#define DYNAMIC_ARG_LIST1 const Dynamic &inArg0
#define DYNAMIC_ARG_LIST2 DYNAMIC_ARG_LIST1,const Dynamic &inArg1
#define DYNAMIC_ARG_LIST3 DYNAMIC_ARG_LIST2,const Dynamic &inArg2
#define DYNAMIC_ARG_LIST4 DYNAMIC_ARG_LIST3,const Dynamic &inArg3
#define DYNAMIC_ARG_LIST5 DYNAMIC_ARG_LIST4,const Dynamic &inArg4
#define DYNAMIC_ARG_LIST6 DYNAMIC_ARG_LIST5,const Dynamic &inArg5
#define DYNAMIC_ARG_LIST7 DYNAMIC_ARG_LIST6,const Dynamic &inArg6
#define DYNAMIC_ARG_LIST8 DYNAMIC_ARG_LIST7,const Dynamic &inArg7
#define DYNAMIC_ARG_LIST9 DYNAMIC_ARG_LIST8,const Dynamic &inArg8
#define DYNAMIC_ARG_LIST10 DYNAMIC_ARG_LIST9,const Dynamic &inArg9
#define DYNAMIC_ARG_LIST11 DYNAMIC_ARG_LIST10,const Dynamic &inArg10
#define DYNAMIC_ARG_LIST12 DYNAMIC_ARG_LIST11,const Dynamic &inArg11

#define ARG_LIST0
#define ARG_LIST1 inArg0
#define ARG_LIST2 ARG_LIST1,inArg1
#define ARG_LIST3 ARG_LIST2,inArg2
#define ARG_LIST4 ARG_LIST3,inArg3
#define ARG_LIST5 ARG_LIST4,inArg4
#define ARG_LIST6 ARG_LIST5,inArg5
#define ARG_LIST7 ARG_LIST6,inArg6
#define ARG_LIST8 ARG_LIST7,inArg7
#define ARG_LIST9 ARG_LIST8,inArg8
#define ARG_LIST10 ARG_LIST9,inArg9
#define ARG_LIST11 ARG_LIST10,inArg10
#define ARG_LIST12 ARG_LIST11,inArg11


#define DEFINE_DYNAMIC_FUNC(class,N,func,ret,array_list,dynamic_arg_list,arg_list) \
struct class##func : public hxObject \
{ \
   int __GetType() const { return vtFunction; } \
   int __ArgCount() const { return N; } \
   hxObjectPtr<class> mThis; \
   class##func(class *inThis) : mThis(inThis) { } \
   String __ToString() const{ return L###func ; } \
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



#define DECLARE_DYNAMIC_FUNC(func,dynamic_arg_list) \
   Dynamic func##_dyn(dynamic_arg_list);

#define STATIC_DECLARE_DYNAMIC_FUNC(func,dynamic_arg_list) \
   static Dynamic func##_dyn(dynamic_arg_list);


#define DEFINE_DYNAMIC_FUNC0(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,0,func,ret,ARRAY_LIST0,DYNAMIC_ARG_LIST0,ARG_LIST0)
#define DEFINE_DYNAMIC_FUNC1(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,1,func,ret,ARRAY_LIST1,DYNAMIC_ARG_LIST1,ARG_LIST1)
#define DEFINE_DYNAMIC_FUNC2(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,2,func,ret,ARRAY_LIST2,DYNAMIC_ARG_LIST2,ARG_LIST2)
#define DEFINE_DYNAMIC_FUNC3(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,3,func,ret,ARRAY_LIST3,DYNAMIC_ARG_LIST3,ARG_LIST3)
#define DEFINE_DYNAMIC_FUNC4(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,4,func,ret,ARRAY_LIST4,DYNAMIC_ARG_LIST4,ARG_LIST4)
#define DEFINE_DYNAMIC_FUNC5(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,5,func,ret,ARRAY_LIST5,DYNAMIC_ARG_LIST5,ARG_LIST5)
#define DEFINE_DYNAMIC_FUNC6(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,6,func,ret,ARRAY_LIST6,DYNAMIC_ARG_LIST6,ARG_LIST6)
#define DEFINE_DYNAMIC_FUNC7(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,7,func,ret,ARRAY_LIST7,DYNAMIC_ARG_LIST7,ARG_LIST7)
#define DEFINE_DYNAMIC_FUNC8(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,8,func,ret,ARRAY_LIST8,DYNAMIC_ARG_LIST8,ARG_LIST8)
#define DEFINE_DYNAMIC_FUNC9(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,9,func,ret,ARRAY_LIST9,DYNAMIC_ARG_LIST9,ARG_LIST9)
#define DEFINE_DYNAMIC_FUNC10(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,10,func,ret,ARRAY_LIST10,DYNAMIC_ARG_LIST10,ARG_LIST10)
#define DEFINE_DYNAMIC_FUNC11(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,11,func,ret,ARRAY_LIST11,DYNAMIC_ARG_LIST11,ARG_LIST11)
#define DEFINE_DYNAMIC_FUNC12(class,func,ret) \
          DEFINE_DYNAMIC_FUNC(class,12,func,ret,ARRAY_LIST12,DYNAMIC_ARG_LIST12,ARG_LIST12)



#define STATIC_DEFINE_DYNAMIC_FUNC(class,N,func,ret,array_list,dynamic_arg_list,arg_list) \
struct class##func : public hxObject \
{ \
   class##func() { } \
   int __GetType() const { return vtFunction; } \
   int __ArgCount() const { return N; } \
   String __ToString() const{ return L###func ; } \
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



#define STATIC_DEFINE_DYNAMIC_FUNC0(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,0,func,ret,ARRAY_LIST0,DYNAMIC_ARG_LIST0,ARG_LIST0)
#define STATIC_DEFINE_DYNAMIC_FUNC1(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,1,func,ret,ARRAY_LIST1,DYNAMIC_ARG_LIST1,ARG_LIST1)
#define STATIC_DEFINE_DYNAMIC_FUNC2(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,2,func,ret,ARRAY_LIST2,DYNAMIC_ARG_LIST2,ARG_LIST2)
#define STATIC_DEFINE_DYNAMIC_FUNC3(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,3,func,ret,ARRAY_LIST3,DYNAMIC_ARG_LIST3,ARG_LIST3)
#define STATIC_DEFINE_DYNAMIC_FUNC4(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,4,func,ret,ARRAY_LIST4,DYNAMIC_ARG_LIST4,ARG_LIST4)
#define STATIC_DEFINE_DYNAMIC_FUNC5(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,5,func,ret,ARRAY_LIST5,DYNAMIC_ARG_LIST5,ARG_LIST5)
#define STATIC_DEFINE_DYNAMIC_FUNC6(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,6,func,ret,ARRAY_LIST6,DYNAMIC_ARG_LIST6,ARG_LIST6)
#define STATIC_DEFINE_DYNAMIC_FUNC7(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,7,func,ret,ARRAY_LIST7,DYNAMIC_ARG_LIST7,ARG_LIST7)
#define STATIC_DEFINE_DYNAMIC_FUNC8(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,8,func,ret,ARRAY_LIST8,DYNAMIC_ARG_LIST8,ARG_LIST8)
#define STATIC_DEFINE_DYNAMIC_FUNC9(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,9,func,ret,ARRAY_LIST9,DYNAMIC_ARG_LIST9,ARG_LIST9)
#define STATIC_DEFINE_DYNAMIC_FUNC10(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,10,func,ret,ARRAY_LIST10,DYNAMIC_ARG_LIST10,ARG_LIST10)
#define STATIC_DEFINE_DYNAMIC_FUNC11(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,11,func,ret,ARRAY_LIST11,DYNAMIC_ARG_LIST11,ARG_LIST11)
#define STATIC_DEFINE_DYNAMIC_FUNC12(class,func,ret) \
          STATIC_DEFINE_DYNAMIC_FUNC(class,12,func,ret,ARRAY_LIST12,DYNAMIC_ARG_LIST12,ARG_LIST12)



#define DEFINE_CREATE_ENUM(enum_obj) \
static Dynamic Create##enum_obj(String inName,DynamicArray inArgs) \
{ \
   int idx =  enum_obj::__FindIndex(inName); \
   if (idx<0) throw INVALID_CONSTRUCTOR; \
   int count =  enum_obj::__FindArgCount(inName); \
   int args = inArgs.GetPtr() ? inArgs.__length() : 0; \
   if (args!=count)  throw INVALID_ARG_COUNT; \
   return CreateEnum<enum_obj >(inName,idx,inArgs); \
}


#define DECLARE_CLASS0(klass) class klass##_obj; typedef hxObjectPtr<klass##_obj> klass;
#define DECLARE_CLASS1(ns1,klass) namespace ns1 { DECLARE_CLASS0(klass) }
#define DECLARE_CLASS2(ns2,ns1,klass) namespace ns2 { DECLARE_CLASS1(ns1,klass) }
#define DECLARE_CLASS3(ns3,ns2,ns1,klass) namespace ns3 { DECLARE_CLASS2(ns2,ns1,klass) }
#define DECLARE_CLASS4(ns4,ns3,ns2,ns1,klass) namespace ns4 { DECLARE_CLASS3(ns3,ns2,ns1,klass) }
#define DECLARE_CLASS5(ns5,ns4,ns3,ns2,ns1,klass) namespace ns5 { DECLARE_CLASS4(ns4,ns3,ns2,ns1,klass) }
#define DECLARE_CLASS6(ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns6 { DECLARE_CLASS5(ns5,ns4,ns3,ns2,ns1,klass) }
#define DECLARE_CLASS7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns7 { DECLARE_CLASS6(ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define DECLARE_CLASS8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns8 { DECLARE_CLASS7(ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }
#define DECLARE_CLASS9(ns9,ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) namespace ns9 { DECLARE_CLASS8(ns8,ns7,ns6,ns5,ns4,ns3,ns2,ns1,klass) }




#define DYNAMIC_CALL(ret,func,array_args,dyn_arg_list,arg_list) \
   Dynamic __Run(const Array<Dynamic> &inArgs) { ret func( array_args ); return null();} \
   Dynamic __run(dyn_arg_list) { ret func( arg_list ); return null();}

#define DYNAMIC_CALL0(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST0,DYNAMIC_ARG_LIST0,ARG_LIST0)
#define DYNAMIC_CALL1(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST1,DYNAMIC_ARG_LIST1,ARG_LIST1)
#define DYNAMIC_CALL2(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST2,DYNAMIC_ARG_LIST2,ARG_LIST2)
#define DYNAMIC_CALL3(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST3,DYNAMIC_ARG_LIST3,ARG_LIST3)
#define DYNAMIC_CALL4(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST4,DYNAMIC_ARG_LIST4,ARG_LIST4)
#define DYNAMIC_CALL5(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST5,DYNAMIC_ARG_LIST5,ARG_LIST5)
#define DYNAMIC_CALL6(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST6,DYNAMIC_ARG_LIST6,ARG_LIST6)
#define DYNAMIC_CALL7(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST7,DYNAMIC_ARG_LIST7,ARG_LIST7)
#define DYNAMIC_CALL8(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST8,DYNAMIC_ARG_LIST8,ARG_LIST8)
#define DYNAMIC_CALL9(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST9,DYNAMIC_ARG_LIST9,ARG_LIST9)
#define DYNAMIC_CALL10(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST10,DYNAMIC_ARG_LIST10,ARG_LIST10)
#define DYNAMIC_CALL11(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST11,DYNAMIC_ARG_LIST11,ARG_LIST11)
#define DYNAMIC_CALL12(ret,func) DYNAMIC_CALL(ret,func,ARRAY_LIST12,DYNAMIC_ARG_LIST12,ARG_LIST12)


#define BEGIN_DEFAULT_FUNC(name,t0) \
	namespace { \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   hxObjectPtr<t0> __this; \
   name(hxObjectPtr<t0> __0 = null()) : __this(__0) {}


#define END_DEFAULT_FUNC \
}


#define BEGIN_LOCAL_FUNC0(name) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   name() {}

#define BEGIN_LOCAL_FUNC1(name,t0,v0) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; \
   void __Mark() { MarkMember(v0); } \
   name(t0 __0) : v0(__0) {}

#define BEGIN_LOCAL_FUNC2(name,t0,v0,t1,v1) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   void __Mark() { MarkMember(v0); MarkMember(v1); } \
   t0 v0; t1 v1;\
   name(t0 __0,t1 __1) : v0(__0), v1(__1) {}

#define BEGIN_LOCAL_FUNC3(name,t0,v0,t1,v1,t2,v2) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2;\
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); } \
   name(t0 __0,t1 __1,t2 __2) : v0(__0), v1(__1), v2(__2) {}

#define BEGIN_LOCAL_FUNC4(name,t0,v0,t1,v1,t2,v2,t3,v3) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3) : v0(__0), v1(__1), v2(__2), v3(__3) {}

#define BEGIN_LOCAL_FUNC5(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4) {}

#define BEGIN_LOCAL_FUNC6(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5) {}

#define BEGIN_LOCAL_FUNC7(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); MarkMember(v6);  } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6) {}


#define BEGIN_LOCAL_FUNC8(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); MarkMember(v6); MarkMember(v7); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7) {}

#define BEGIN_LOCAL_FUNC9(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); MarkMember(v6); MarkMember(v7); MarkMember(v8); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8) {}

#define BEGIN_LOCAL_FUNC10(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); MarkMember(v6); MarkMember(v7); MarkMember(v8); MarkMember(v9); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9) {}

#define BEGIN_LOCAL_FUNC11(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9,t10,v10) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; t10 v10;  \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); MarkMember(v6); MarkMember(v7); MarkMember(v8); MarkMember(v9); MarkMember(v10); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9, t10 __10) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9), v10(__10) {}

#define BEGIN_LOCAL_FUNC12(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9,t10,v10,t11,v11) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; t10 v10; t11 v11; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); MarkMember(v6); MarkMember(v7); MarkMember(v8); MarkMember(v9); MarkMember(v10); MarkMember(v11); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9, t10 __10, t11 __11) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9), v10(__10), v11(__11) {}

#define BEGIN_LOCAL_FUNC13(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9,t10,v10,t11,v11,t12,v12) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; t10 v10; t11 v11; t12 v12; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); MarkMember(v6); MarkMember(v7); MarkMember(v8); MarkMember(v9); MarkMember(v10); MarkMember(v11); MarkMember(v12);  } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9, t10 __10, t11 __11, t12 __12) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9), v10(__10), v11(__11), v12(__12) {}


#define BEGIN_LOCAL_FUNC14(name,t0,v0,t1,v1,t2,v2,t3,v3,t4,v4,t5,v5,t6,v6,t7,v7,t8,v8,t9,v9,t10,v10,t11,v11,t12,v12,t13,v13) \
   struct name : public hxObject { int __GetType() const { return vtFunction; } \
   t0 v0; t1 v1; t2 v2; t3 v3; t4 v4; t5 v5; t6 v6; t7 v7; t8 v8; t9 v9; t10 v10; t11 v11; t12 v12; t13 v13; \
   void __Mark() { MarkMember(v0); MarkMember(v1); MarkMember(v2); MarkMember(v3); MarkMember(v4); MarkMember(v5); MarkMember(v6); MarkMember(v7); MarkMember(v8); MarkMember(v9); MarkMember(v10); MarkMember(v11); MarkMember(v12); MarkMember(v13); } \
   name(t0 __0,t1 __1,t2 __2, t3 __3, t4 __4,t5 __5,t6 __6,t7 __7,t8 __8,t9 __9, t10 __10, t11 __11, t12 __12, t13 __13) : v0(__0), v1(__1), v2(__2), v3(__3), v4(__4), v5(__5), v6(__6), v7(__7), v8(__8), v9(__9), v10(__10), v11(__11), v12(__12), v13(__13) {}


#define END_LOCAL_FUNC0(ret) DYNAMIC_CALL0(ret,run) };
#define END_LOCAL_FUNC1(ret) DYNAMIC_CALL1(ret,run) };
#define END_LOCAL_FUNC2(ret) DYNAMIC_CALL2(ret,run) };
#define END_LOCAL_FUNC3(ret) DYNAMIC_CALL3(ret,run) };
#define END_LOCAL_FUNC4(ret) DYNAMIC_CALL4(ret,run) };
#define END_LOCAL_FUNC5(ret) DYNAMIC_CALL5(ret,run) };
#define END_LOCAL_FUNC6(ret) DYNAMIC_CALL6(ret,run) };
#define END_LOCAL_FUNC7(ret) DYNAMIC_CALL7(ret,run) };
#define END_LOCAL_FUNC8(ret) DYNAMIC_CALL8(ret,run) };
#define END_LOCAL_FUNC9(ret) DYNAMIC_CALL9(ret,run) };
#define END_LOCAL_FUNC10(ret) DYNAMIC_CALL10(ret,run) };
#define END_LOCAL_FUNC11(ret) DYNAMIC_CALL11(ret,run) };
#define END_LOCAL_FUNC12(ret) DYNAMIC_CALL12(ret,run) };


#define DYNAMIC_COMPARE_OP_NOT_EQUAL \
   bool operator != (const Dynamic &inRHS) const { return (Compare(inRHS) != 0); } \
   bool operator != (const String &inRHS)  const { return !mPtr || ((String)(*this) != inRHS); } \
   bool operator != (double inRHS)  const { return !mPtr || ((double)(*this) != inRHS); } \
   bool operator != (int inRHS)  const { return !mPtr || ((double)(*this) != (double)inRHS); } \
   bool operator != (bool inRHS)  const { return !mPtr || ((double)(*this) != (double)inRHS); }


#define DYNAMIC_COMPARE_OP( op ) \
   bool operator op (const String &inRHS)  const { return mPtr && ((String)(*this) op inRHS); } \
   bool operator op (double inRHS)  const { return mPtr && ((double)(*this) op inRHS); } \
   bool operator op (int inRHS)  const { return mPtr && ((double)(*this) op (double)inRHS); } \
   bool operator op (bool inRHS)  const { return mPtr && ((double)(*this) op (double)inRHS); }

#define DYNAMIC_COMPARE_OP_ALL( op ) \
   bool operator op (const Dynamic &inRHS) const { return mPtr && (Compare(inRHS) op 0); } \
   DYNAMIC_COMPARE_OP(op)

#define COMPARE_DYNAMIC_OP_NOT_EQUAL \
   inline bool operator != (double inLHS,const Dynamic &inRHS) \
      { return !inRHS.GetPtr() || (inLHS != (double)inRHS); } \
   inline bool operator != (int inLHS,const Dynamic &inRHS) \
      { return !inRHS.GetPtr() || (inLHS != (double)inRHS); } \
   inline bool operator != (bool inLHS,const Dynamic &inRHS) \
      { return !inRHS.GetPtr() || ((double)inLHS != (double)inRHS); }


#define COMPARE_DYNAMIC_OP( op ) \
   inline bool operator op (double inLHS,const Dynamic &inRHS) \
      { return inRHS.GetPtr() && (inLHS op (double)inRHS); } \
   inline bool operator op (int inLHS,const Dynamic &inRHS) \
      { return inRHS.GetPtr() && (inLHS op (double)inRHS); } \
   inline bool operator op (bool inLHS,const Dynamic &inRHS) \
      { return inRHS.GetPtr() && ((double)inLHS op (double)inRHS); }

#define DYNAMIC_ARITH( op ) \
   double operator op (const Dynamic &inRHS) const { return (double)(*this) op (double)inRHS; } \
   double operator op (const double &inRHS) const { return (double)(*this) op (double)inRHS; } \
   double operator op (const int &inRHS) const { return (double)(*this) op (double)inRHS; } \

#define ARITH_DYNAMIC( op ) \
   inline double operator op (const double &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS;} \
   inline double operator op (const int &inLHS,const Dynamic &inRHS) { return inLHS op (double)inRHS; } \


#define BEGIN_MAIN \
int main(int argc,char **argv){ \
	__boot_hxcpp(); \
	try{ \
		__boot_all();

#define END_MAIN \
	} \
	catch (Dynamic e){ \
		printf("Error : %s\n",e->toString().__CStr()); \
	} \
	return 0; \
}


#define BEGIN_LIB_MAIN \
extern "C" {\
void __hxcpp_lib_main() \
{ \
	__boot_hxcpp(); \
	__boot_all();

#define END_LIB_MAIN \
} }



#endif

