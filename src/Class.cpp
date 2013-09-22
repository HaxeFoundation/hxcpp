#include <hxcpp.h>
#include <map>

#ifdef ANDROID
#include <android/log.h>
#endif


namespace hx
{

typedef std::map<String,Class> ClassMap;
static ClassMap *sClassMap = 0;

Class RegisterClass(const String &inClassName, CanCastFunc inCanCast,
                    String inStatics[], String inMembers[],
                    ConstructEmptyFunc inConstructEmpty, ConstructArgsFunc inConstructArgs,
                    Class *inSuperClass, ConstructEnumFunc inConstructEnum,
                    MarkFunc inMarkFunc
                    #ifdef HXCPP_VISIT_ALLOCS
                    ,VisitFunc inVisitFunc
                    #endif
                    #ifdef HXCPP_SCRIPTABLE
                    ,const hx::StorageInfo *inStorageInfo
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
                                  #endif
                                  );
   Class c(obj);
   (*sClassMap)[inClassName] = c;
   return c;
}

void RegisterClass(const String &inClassName, Class inClass)
{
   if (sClassMap==0)
      sClassMap = new ClassMap;
   (*sClassMap)[inClassName] = inClass;
}



}

using namespace hx;

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
   #endif

   if (inStatics)
   {
      mStatics = Array_obj<String>::__new(0,0);
      for(String *s = inStatics; s->length; s++)
         mStatics->Add( *s );
   }
   if (inMembers)
   {
      mMembers = Array_obj<String>::__new(0,0);
      for(String *m = inMembers; m->length; m++)
         mMembers->Add( *m );
   }
   mCanCast = inCanCast;
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
Static(Class_obj__mClass) = hx::RegisterClass(HX_CSTRING("Class"),TCanCast<Class_obj>,sNone,sNone, 0,0 , 0, 0 );
}


void Class_obj::MarkStatics(hx::MarkContext *__inCtx)
{
   HX_MARK_MEMBER(__meta__);
   if (mMarkFunc)
       mMarkFunc(__inCtx);
}
#ifdef HXCPP_VISIT_ALLOCS
void Class_obj::VisitStatics(hx::VisitContext *__inCtx)
{
   HX_VISIT_MEMBER(__meta__);
   if (mVisitFunc)
       mVisitFunc(__inCtx);
}
#endif

Class Class_obj::Resolve(String inName)
{
   ClassMap::const_iterator i = sClassMap->find(inName);
   if (i==sClassMap->end())
      return null();
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
   // Class fields do not include user fields...
   return mStatics->copy();
   /*
   Array<String> result = mSuper ? (*mSuper)->GetClassFields() : Array<String>(0,0);
   if (mStatics.mPtr)
   {
      for(int s=0;s<mStatics->size();s++)
      {
         const String &mem = mStatics[s];
         if (result->Find(mem)==-1)
            result.Add(mem);
      }
   }
   return result;
   */
}

bool Class_obj::__HasField(const String &inString)
{
   if (mStatics.mPtr)
      for(int s=0;s<mStatics->size();s++)
         if (mStatics[s]==inString)
            return true;
   if (mSuper)
      return (*mSuper)->__HasField(inString);
   return false;
}

Dynamic Class_obj::__Field(const String &inString, bool inCallProp)
{
   if (inString==HX_CSTRING("__meta__"))
      return __meta__;
   // Not the most efficient way of doing this!
   if (!mConstructEmpty)
      return null();
   Dynamic instance = mConstructEmpty();
   return instance->__Field(inString, inCallProp);
}

Dynamic Class_obj::__SetField(const String &inString,const Dynamic &inValue, bool inCallProp)
{
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
      if (mSuper)
         return (*mSuper)->GetMemberStorage(inName);
   }
   return 0;
}
#endif


namespace hx
{

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


}


