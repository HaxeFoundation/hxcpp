#include <hxcpp.h>
#include <map>

#ifdef ANDROID
#include <android/log.h>
#endif


namespace hx
{

typedef std::map<String,Class> ClassMap;
static ClassMap *sClassMap = 0;

Class _hx_RegisterClass(const String &inClassName, CanCastFunc inCanCast,
                    String inStatics[], String inMembers[],
                    ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
                    Class *inSuperClass, ConstructEnumFunc inConstructEnum,
                    MarkFunc inMarkFunc
                    #ifdef HXCPP_VISIT_ALLOCS
                    ,VisitFunc inVisitFunc
                    #endif
                    #ifdef HXCPP_SCRIPTABLE
                    ,const hx::StorageInfo *inStorageInfo
                    ,const hx::StaticInfo *inStaticInfo
                    #endif
                    )
{
   if (sClassMap==0)
      sClassMap = new ClassMap;

   Class_obj *obj = new Class_obj(inClassName, inStatics, inMembers,
                                  inConstructEmpty, inConstructArgs, inSuperClass,
                                  inConstructEnum, inCanCast, inMarkFunc
                                  #ifdef HXCPP_VISIT_ALLOCS
                                  ,inVisitFunc
                                  #endif
                                  #ifdef HXCPP_SCRIPTABLE
                                  ,inStorageInfo
                                  ,inStaticInfo
                                  #endif
                                  );
   Class c(obj);
   (*sClassMap)[inClassName] = c;
   return c;
}

void _hx_RegisterClass(const String &inClassName, Class inClass)
{
   if (sClassMap==0)
      sClassMap = new ClassMap;
   (*sClassMap)[inClassName] = inClass;
}



// -------- Class ---------------------------------------


Class_obj::Class_obj(const String &inClassName,String inStatics[], String inMembers[],
             ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
             Class *inSuperClass,ConstructEnumFunc inConstructEnum,
             CanCastFunc inCanCast, MarkFunc inFunc
             #ifdef HXCPP_VISIT_ALLOCS
             ,VisitFunc inVisitFunc
             #endif
             #ifdef HXCPP_SCRIPTABLE
             ,const hx::StorageInfo *inStorageInfo
             ,const hx::StaticInfo *inStaticInfo
             #endif
             )
{
   mName = inClassName;
   mSuper = inSuperClass;
   mConstructEmpty = inConstructEmpty;
   mConstructArgs = inConstructArgs;
   mConstructEnum = inConstructEnum;
   mMarkFunc = inFunc;
   #ifdef HXCPP_VISIT_ALLOCS
   mVisitFunc = inVisitFunc;
   #endif

   #ifdef HXCPP_SCRIPTABLE
   mMemberStorageInfo = inStorageInfo;
   mStaticStorageInfo = inStaticInfo;
   #endif

   mStatics = dupFunctions(inStatics);
   mMembers = dupFunctions(inMembers);
   mCanCast = inCanCast;
}

bool Class_obj::GetNoStaticField(const String &inString, Dynamic &outValue, hx::PropertyAccess inCallProp)
{
   return false;
}
bool Class_obj::SetNoStaticField(const String &inString, Dynamic &ioValue, hx::PropertyAccess inCallProp)
{
   return false;
}



::Array< ::String > Class_obj::dupFunctions(String inFuncs[])
{
   if (!inFuncs)
      return null();

   Array<String> result = Array_obj<String>::__new(0,0);
   for(String *s = inFuncs; s->length; s++)
         result->Add( *s );
    return result;
}

void Class_obj::registerScriptable(bool inOverwrite)
{
   if (!inOverwrite && sClassMap->find(mName)!=sClassMap->end())
      return;
   (*sClassMap)[ mName ] = this;
}

Class Class_obj::GetSuper()
{
   if (!mSuper)
      return null();
	if (mSuper==&hx::Object::__SGetClass())
		return null();
   return *mSuper;
}

void Class_obj::__Mark(hx::MarkContext *__inCtx)
{
   HX_MARK_MEMBER(mName);
   HX_MARK_MEMBER(mStatics);
   HX_MARK_MEMBER(mMembers);
}

#ifdef HXCPP_VISIT_ALLOCS
void Class_obj::__Visit(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(mName);
   HX_VISIT_MEMBER(mStatics);
   HX_VISIT_MEMBER(mMembers);
   //HX_VISIT_OBJECT(*mSuper);
}
#endif

Class Class_obj__mClass;

Class  Class_obj::__GetClass() const { return Class_obj__mClass; }
Class &Class_obj::__SGetClass() { return Class_obj__mClass; }

void Class_obj::__boot()
{
Static(Class_obj__mClass) = hx::_hx_RegisterClass(HX_CSTRING("Class"),TCanCast<Class_obj>,sNone,sNone, 0,0 , 0, 0 );
}


void Class_obj::MarkStatics(hx::MarkContext *__inCtx)
{
   HX_MARK_MEMBER(__meta__);
   HX_MARK_MEMBER(__rtti__);
   if (mMarkFunc)
       mMarkFunc(__inCtx);
}
#ifdef HXCPP_VISIT_ALLOCS
void Class_obj::VisitStatics(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(__meta__);
   HX_VISIT_MEMBER(__rtti__);
   if (mVisitFunc)
       mVisitFunc(__inCtx);
}
#endif

Class Class_obj::Resolve(String inName)
{
   ClassMap::const_iterator i = sClassMap->find(inName);
   if (i==sClassMap->end())
   {
      // Class class...
      if (inName==HX_CSTRING("Enum"))
         return Class_obj__mClass;
      return null();
   }
   return i->second;
}

Dynamic Class_obj::ConstructEmpty()
{
   return mConstructEmpty();
}

Dynamic Class_obj::ConstructArgs(DynamicArray inArgs)
{
   return mConstructArgs(inArgs);
}

Dynamic Class_obj::ConstructEnum(String inName,DynamicArray inArgs)
{
   if (mConstructEnum==0)
      return null();
   return mConstructEnum(inName,inArgs);
}



String Class_obj::__ToString() const { return mName; }


Array<String> Class_obj::GetInstanceFields()
{
   Array<String> result = mSuper ? (*mSuper)->GetInstanceFields() : Array<String>(0,0);
   if (mMembers.mPtr)
      for(int m=0;m<mMembers->size();m++)
      {
         const String &mem = mMembers[m];
         if (result->Find(mem)==-1)
            result.Add(mem);
      }
   return result;
}

Array<String> Class_obj::GetClassFields()
{
   Array<String> result = mStatics.mPtr ? mStatics->copy() : new Array_obj<String>(0,0);
   if (__rtti__.__s)
      result->push( HX_CSTRING("__rtti") );
   return result;
}

bool Class_obj::__HasField(const String &inString)
{
   if (__rtti__.__s && inString==HX_CSTRING("__rtti"))
      return true;

   if (mStatics.mPtr)
      for(int s=0;s<mStatics->size();s++)
         if (mStatics[s]==inString)
            return true;
   return false;
}

hx::Val Class_obj::__Field(const String &inString, hx::PropertyAccess inCallProp)
{
   if (inString==HX_CSTRING("__meta__"))
      return __meta__;
   #if (HXCPP_API_LEVEL>320)
   if (inString==HX_CSTRING("__rtti"))
      return __rtti__;
   #endif

   if (mGetStaticField)
   {
      Dynamic result;
      if (mGetStaticField(inString,result,inCallProp))
         return result;
 
      // Throw ?
      return null();
   }

   // Not the most efficient way of doing this!
   if (!mConstructEmpty)
      return null();
   Dynamic instance = mConstructEmpty();
   return instance->__Field(inString, inCallProp);
}

hx::Val Class_obj::__SetField(const String &inString,const hx::Val &inValue, hx::PropertyAccess inCallProp)
{

   if (mSetStaticField)
   {
      Dynamic result = inValue;
      if (mSetStaticField(inString,result,inCallProp))
         return result;
 
      // Throw ?
      return inValue;
   }




   // Not the most efficient way of doing this!
   if (!mConstructEmpty)
      return null();
   Dynamic instance = mConstructEmpty();
   return instance->__SetField(inString,inValue, inCallProp);
}

bool Class_obj::__IsEnum()
{
   return mConstructEnum || this==GetVoidClass().GetPtr() || this==GetBoolClass().GetPtr();
}

#ifdef HXCPP_SCRIPTABLE
const hx::StorageInfo* Class_obj::GetMemberStorage(String inName)
{
   if (mMemberStorageInfo)
   {
      for(const StorageInfo *s = mMemberStorageInfo; s->offset; s++)
      {
         if (s->name == inName)
            return s;
      }
   }
   if (mSuper)
      return (*mSuper)->GetMemberStorage(inName);
   return 0;
}


const hx::StaticInfo* Class_obj::GetStaticStorage(String inName)
{
   if (mStaticStorageInfo)
   {
      for(const StaticInfo *s = mStaticStorageInfo; s->address; s++)
      {
         if (s->name == inName)
            return s;
      }
   }
   return 0;
}


#endif



void MarkClassStatics(hx::MarkContext *__inCtx)
{
   #ifdef HXCPP_DEBUG
   MarkPushClass("MarkClassStatics",__inCtx);
   #endif
   ClassMap::iterator end = sClassMap->end();
   for(ClassMap::iterator i = sClassMap->begin(); i!=end; ++i)
   {
      HX_MARK_MEMBER(i->first);

      // all strings should be constants anyhow - HX_MARK_MEMBER(i->first);
      HX_MARK_OBJECT(i->second.mPtr);

      #ifdef HXCPP_DEBUG
      hx::MarkPushClass(i->first.__s,__inCtx);
      hx::MarkSetMember("statics",__inCtx);
      #endif
   
      i->second->MarkStatics(__inCtx);

      #ifdef HXCPP_DEBUG
      hx::MarkPopClass(__inCtx);
      #endif
   }
   #ifdef HXCPP_DEBUG
   MarkPopClass(__inCtx);
   #endif
}

#ifdef HXCPP_VISIT_ALLOCS

void VisitClassStatics(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(Class_obj__mClass);
   ClassMap::iterator end = sClassMap->end();
   for(ClassMap::iterator i = sClassMap->begin(); i!=end; ++i)
   {
      // all strings should be constants anyhow - should not be needed?
      HX_VISIT_STRING(i->first.__s);

      HX_VISIT_OBJECT(i->second.mPtr);

      i->second->VisitStatics(__inCtx);
   }
}

#endif


} // End namespace hx

Array<String> __hxcpp_get_class_list()
{
   Array<String> result = Array_obj<String>::__new();
   if (hx::sClassMap)
   {
      for(hx::ClassMap::iterator i=hx::sClassMap->begin(); i!=hx::sClassMap->end(); ++i)
      {
         if (i->second.mPtr)
            result->push( i->first );
      }
   }
   return result;
}


