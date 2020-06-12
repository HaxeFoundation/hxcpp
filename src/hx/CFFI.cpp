#include <hxcpp.h>
#include <stdio.h>
#include <hx/Memory.h>
#include <hx/OS.h>

#define IGNORE_CFFI_API_H

#include <hx/CFFI.h>
#include <map>
#include <string>

#ifdef _MSC_VER
#pragma warning( disable : 4190 )
#endif


// Class for boxing external handles

namespace hx
{

class Abstract_obj : public Object
{
public:
   HX_IS_INSTANCE_OF enum { _hx_ClassId = hx::clsIdAbstract };

   Abstract_obj(int inType,void *inData)
   {
      mType = inType;
      mHandle = inData;
      mFinalizer = 0;
      mMarkSize = 0;
   }

   Abstract_obj(int inType,int inSize, finalizer inFinalizer)
   {
      mType = inType;
      mFinalizer = 0;
      mMarkSize = 0;
      mHandle = 0;
      if (inSize)
      {
         mMarkSize = inSize;
         mHandle = HxAlloc(inSize);
         memset(mHandle,0,mMarkSize);
      }

      SetFinalizer(inFinalizer);
   }


   virtual int __GetType() const { return mType; }
   virtual hx::ObjectPtr<hx::Class_obj> __GetClass() const { return 0; }
   virtual bool __IsClass(hx::Class inClass ) const { return false; }

   virtual void *__GetHandle() const
   {
      return mHandle;
   }

   void __Mark(hx::MarkContext *__inCtx)
   {
      HX_MARK_MEMBER(_hxcpp_toString);
      if (mMarkSize>=sizeof(void *) && mHandle)
      {
         hx::MarkConservative((int *)mHandle, ((int *)mHandle) + (mMarkSize/sizeof(int)), __inCtx );
      }
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx)
   {
      HX_VISIT_MEMBER(_hxcpp_toString);
      if (mFinalizer)
         mFinalizer->Visit(__inCtx);
   }
   #endif

   void SetFinalizer(finalizer inFinalizer)
   {
      if (!inFinalizer)
      {
         if (mFinalizer)
            mFinalizer->Detach();
         mFinalizer = 0;
      }
      else
      {
         if (!mFinalizer)
            mFinalizer = new hx::InternalFinalizer(this);
         mFinalizer->mFinalizer = inFinalizer;
      }
   }
   void Clear()
   {
      SetFinalizer(0);
      mType = 0;
      if (mMarkSize && mHandle)
         HxFree(mHandle);
      mHandle = 0;
   }

   String toString()
   {
      if (_hxcpp_toString.mPtr)
         return _hxcpp_toString( Dynamic(this) );

      char buffer[40];
      sprintf(buffer,"0x%p", mHandle);

      return HX_CSTRING("Abstract(") +
             __hxcpp_get_kind(this) +
             HX_CSTRING(":") +
             String::create(buffer,strlen(buffer)) +
             HX_CSTRING(")");
   }

   hx::Val __Field(const String &inString, hx::PropertyAccess inCallProp)
   {
      if (inString==HX_CSTRING("_hxcpp_toString")) return _hxcpp_toString;
      if (inString==HX_CSTRING("_hxcpp_kind")) return __hxcpp_get_kind(this);
      return hx::Object::__Field(inString, inCallProp);
   }

   hx::Val __SetField(const String &inName,const hx::Val &inValue, hx::PropertyAccess inCallProp)
   {
      if (inName==HX_CSTRING("_hxcpp_toString"))
      {
         _hxcpp_toString = inValue;
         return inValue;
      }
      return hx::Object::__SetField(inName, inValue,inCallProp);
   }


   Dynamic _hxcpp_toString;
   hx::InternalFinalizer *mFinalizer;
   void *mHandle;
   int mType;
   int mMarkSize;
};

typedef ObjectPtr<Abstract_obj> Abstract;

} // end namespace hx

vkind k_int32 = (vkind)vtAbstractBase;
vkind k_hash = (vkind)(vtAbstractBase + 1);
vkind k_cpp_pointer = (vkind)(vtAbstractBase + 2);
vkind k_cpp_struct = (vkind)(vtAbstractBase + 3);
vkind k_cpp_objc = (vkind)(vtAbstractBase + 4);
static int sgKinds = (int)(vtAbstractBase + 5);

typedef std::map<std::string,int> KindMap;
typedef std::map<int,std::string> ReverseKindMap;
static KindMap sgKindMap;
static ReverseKindMap sgReverseKindMap;


int hxcpp_alloc_kind()
{
   return ++sgKinds;
}


void hxcpp_kind_share(int &ioKind,const char *inName)
{
   int &kind = sgKindMap[inName];
   if (kind==0)
      kind = hxcpp_alloc_kind();
   ioKind = kind;
   sgReverseKindMap[kind] = inName;
}

String __hxcpp_get_kind(Dynamic inObject)
{
   int type = inObject->__GetType();
   if (type<vtAbstractBase)
      return null();
   if (type==(int)(size_t)k_cpp_pointer)
      return HX_CSTRING("cpp.Pointer");
   ReverseKindMap::const_iterator it = sgReverseKindMap.find(type);
   if (it==sgReverseKindMap.end())
      return null();
   return String::create(it->second.c_str(), it->second.size());
}


#define THROWS throw(Dynamic)
//#define THROWS


extern "C" {


/*
 This bit of Macro magic is used to define extern function pointers
  in ndlls, define stub implementations that link back to the hxcpp dll
  and glue up the implementation in the hxcpp runtime.

 For the static link case, these functions are linked directly.
*/

void hx_error() THROWS
{
   hx::Throw( HX_CSTRING("ERROR") );
}


void val_throw(hx::Object * arg1) THROWS
{
   if (arg1==0)
      hx::Throw( null() );
   hx::Throw( arg1 );
}


void hx_fail(const char * inMessage,const char * inFile,int inLine)
{
   if (inFile!=0 && inLine!=0)
      hx::Throw( HX_CSTRING("Failure ") + String(inMessage) + HX_CSTRING(" @ ") +
                    String(inFile) + HX_CSTRING(":") + Dynamic(inLine) );
   else
      hx::Throw( HX_CSTRING("Failure ") + String(inMessage) );
}



// Determine hx::Object * type
int val_type(hx::Object * arg1)
{
   if (arg1==0)
      return valtNull;
   return arg1->__GetType();
}

\
vkind val_kind(hx::Object * arg1) THROWS
{
   if (arg1==0)
      hx::Throw( HX_CSTRING("Value has no 'kind'") );
   int type = arg1->__GetType();
   if (type<valtAbstractBase)
      hx::Throw( HX_CSTRING("Value has no 'kind'") );
   return (vkind)(intptr_t)(type);
}


void * val_to_kind(hx::Object * arg1,vkind arg2)
{
   if (arg1==0)
      return 0;
   if ((int)(intptr_t)arg2 == arg1->__GetType())
      return arg1->__GetHandle();
   return 0;
}


// don't check the 'kind' ...
void * val_data(hx::Object * arg1)
{
   if (arg1==0)
      return 0;
   return arg1->__GetHandle();
}


int val_fun_nargs(hx::Object * arg1)
{
   if (arg1==0)
      return faNotFunction;
   return arg1->__ArgCount();
}




// Extract hx::Object * type
bool val_bool(hx::Object * arg1)
{
   if (arg1==0) return false;
   return arg1->__ToInt()!=0;
}


int val_int(hx::Object * arg1)
{
   if (arg1==0) return 0;
   return arg1->__ToInt();
}


double val_float(hx::Object * arg1)
{
   if (arg1==0) return 0.0;
   return arg1->__ToDouble();
}


double val_number(hx::Object * arg1)
{
   if (arg1==0) return 0.0;
   return arg1->__ToDouble();
}



// Create hx::Object * type

hx::Object * alloc_null() { return 0; }
hx::Object * alloc_bool(bool arg1) { return Dynamic(arg1).GetPtr(); }
hx::Object * alloc_int(int arg1) { return Dynamic(arg1).GetPtr(); }
hx::Object * alloc_float(double arg1) { return Dynamic(arg1).GetPtr(); }
hx::Object * alloc_empty_object() { return new hx::Anon_obj(); }


hx::Object * alloc_abstract(vkind arg1,void * arg2)
{
   int type = (int)(intptr_t)arg1;
   return new hx::Abstract_obj(type,arg2);
}

hx::Object *create_abstract(vkind inKind,int inMemSize, hx::finalizer inFree )
{
   int type = (int)(intptr_t)inKind;

   return new hx::Abstract_obj(type,inMemSize,inFree);
}


void free_abstract(hx::Object *obj)
{
   if (obj)
   {
      hx::Abstract_obj *abstract = dynamic_cast<hx::Abstract_obj *>(obj);
      if (abstract)
         abstract->Clear();
   }
}

hx::Object * alloc_best_int(int arg1) { return Dynamic(arg1).GetPtr(); }
hx::Object * alloc_int32(int arg1) { return Dynamic(arg1).GetPtr(); }



// String access
int val_strlen(hx::Object * arg1)
{
   if (arg1==0) return 0;
   return arg1->toString().length;
}


const wchar_t * val_wstring(hx::Object * arg1)
{
   if (arg1==0) return L"";
   return arg1->toString().__WCStr();
}


const char * val_string(hx::Object * arg1)
{
   if (arg1==0) return "";
   return arg1->__CStr();
}


hx::Object * alloc_string(const char * arg1)
{
   return Dynamic( String::create(arg1) ).GetPtr();
}

hx::StringEncoding hxs_encoding(const String &str)
{
   #ifdef HX_SMART_STRINGS
   if (str.isUTF16Encoded())
      return hx::StringUtf16;
   else
      return hx::StringAscii;
   #else
   return hx::StringUtf8;
   #endif
}


wchar_t * val_dup_wstring(value inVal)
{
   hx::Object *obj = (hx::Object *)inVal;
   if (!obj)
      return 0;
   String  s = obj->toString();
   if (!s.raw_ptr())
      return 0;
   #ifdef HX_SMART_STRINGS
   if ( sizeof(wchar_t)==2 && s.isUTF16Encoded())
   {
      wchar_t *result = (wchar_t *)hx::NewGCBytes(0,(s.length+1)*2);
      memcpy(result, s.wchar_str(), s.length*2 );
      result[s.length]=0;
      return result;
   }
   #endif
   return (wchar_t *)s.wchar_str();
}

char * val_dup_string(value inVal)
{
   hx::Object *obj = (hx::Object *)inVal;
   if (!obj) return 0;
   String  s = obj->toString();
   if (!s.raw_ptr())
      return 0;
   #ifdef HX_SMART_STRINGS
   if (s.isUTF16Encoded())
      return (char *)s.utf8_str();
   #endif

   char *result = (char *)hx::NewGCBytes(0,s.length+1);
   memcpy(result, s.raw_ptr(), s.length);
   result[s.length] = 0;
   return result;
}


char *alloc_string_data(const char *inData, int inLength)
{
   char *result = (char *)hx::NewGCBytes(0,inLength+1);
   if (inData)
   {
      memcpy(result, inData, inLength);
      result[inLength] = 0;
   }
   return result;
}

hx::Object *alloc_string_len(const char *inStr,int inLen)
{
   return Dynamic( String::create(inStr,inLen) ).GetPtr();
}

hx::Object *alloc_wstring_len(const wchar_t *inStr,int inLen)
{
   String str = String::create(inStr,inLen);
   return Dynamic(str).GetPtr();
}

// Array access - generic
int val_array_size(hx::Object * arg1)
{
   if (arg1==0) return 0;
   return arg1->__length();
}


hx::Object * val_array_i(hx::Object * arg1,int arg2)
{
   if (arg1==0) return 0;
   return arg1->__GetItem(arg2).GetPtr();
}

void val_array_set_i(hx::Object * arg1,int arg2,hx::Object *inVal)
{
   if (arg1==0) return;
   arg1->__SetItem(arg2, Dynamic(inVal) );
}

void val_array_set_size(hx::Object * arg1,int inLen)
{
   #if (HXCPP_API_LEVEL<330)
   if (arg1==0) return;
   arg1->__SetSize(inLen);
   #else
   hx::ArrayBase *base = dynamic_cast<hx::ArrayBase *>(arg1);
   if (base)
   {
      base->__SetSize(inLen);
   }
   else
   {
      cpp::VirtualArray_obj *va = dynamic_cast<cpp::VirtualArray_obj *>(arg1);
      if (va)
         va->__SetSize(inLen);
   }
   #endif
}

void val_array_push(hx::Object * arg1,hx::Object *inValue)
{
   hx::ArrayBase *base = dynamic_cast<hx::ArrayBase *>(arg1);
   if (base==0) return;
   base->__push(inValue);
}


hx::Object * alloc_array(int arg1)
{
   Array<Dynamic> array(arg1,arg1);
   return array.GetPtr();
}



// Array access - fast if possible - may return null
// Resizing the array may invalidate the pointer
bool * val_array_bool(hx::Object * arg1)
{
   #if (HXCPP_API_LEVEL>330)
   hx::ArrayCommon *common = dynamic_cast< hx::ArrayCommon * >(arg1);
   if (!common) return 0;
   arg1 = common->__GetRealObject();
   #endif
   Array_obj<bool> *a = dynamic_cast< Array_obj<bool> * >(arg1);
   if (a==0)
      return 0;
   return (bool *)a->GetBase();
}


int * val_array_int(hx::Object * arg1)
{
   #if (HXCPP_API_LEVEL>330)
   hx::ArrayCommon *common = dynamic_cast< hx::ArrayCommon * >(arg1);
   if (!common) return 0;
   arg1 = common->__GetRealObject();
   #endif
   Array_obj<int> *a = dynamic_cast< Array_obj<int> * >(arg1);
   if (a==0)
      return 0;
   return (int *)a->GetBase();
}


double * val_array_double(hx::Object * arg1)
{
   #if (HXCPP_API_LEVEL>330)
   hx::ArrayCommon *common = dynamic_cast< hx::ArrayCommon * >(arg1);
   if (!common) return 0;
   arg1 = common->__GetRealObject();
   #endif
   Array_obj<double> *a = dynamic_cast< Array_obj<double> * >(arg1);
   if (a==0)
      return 0;
   return (double *)a->GetBase();
}


float * val_array_float(hx::Object * arg1)
{
   #if (HXCPP_API_LEVEL>330)
   hx::ArrayCommon *common = dynamic_cast< hx::ArrayCommon * >(arg1);
   if (!common) return 0;
   arg1 = common->__GetRealObject();
   #endif
   Array_obj<float> *a = dynamic_cast< Array_obj<float> * >(arg1);
   if (a==0)
      return 0;
   return (float *)a->GetBase();
}

value * val_array_value(hx::Object * arg1)
{
   return 0;
}




typedef Array_obj<unsigned char> *ByteArray;

// Byte arrays
// The byte array may be a string or a Array<bytes> depending on implementation
buffer val_to_buffer(hx::Object * arg1)
{
   ByteArray b = dynamic_cast< ByteArray >(arg1);
   return (buffer)b;
}

bool val_is_buffer(value inVal) { return val_to_buffer((hx::Object *)inVal)!=0; }



buffer alloc_buffer(const char *inStr)
{
   int len = inStr ? strlen(inStr) : 0;
   ByteArray b = new Array_obj<unsigned char>(len,len);
   if (len)
      memcpy(b->GetBase(),inStr,len);
   return (buffer)b;
}


buffer alloc_buffer_len(int inLen)
{
   ByteArray b = new Array_obj<unsigned char>(inLen,inLen);
   return (buffer)b;
}


value buffer_val(buffer b)
{
   return (value)b;
}


value buffer_to_string(buffer inBuffer)
{
   ByteArray b = (ByteArray) inBuffer;
   String str(b->GetBase(),b->length);
   Dynamic d(str);
   return (value)d.GetPtr();
}


void buffer_append(buffer inBuffer,const char *inStr)
{
   ByteArray b = (ByteArray)inBuffer;
   int olen = b->length;
   int len = strlen(inStr);
   b->__SetSize(olen+len);
   memcpy(b->GetBase()+olen,inStr,len);

}


int buffer_size(buffer inBuffer)
{
   ByteArray b = (ByteArray)inBuffer;
   return b->length;
}


void buffer_set_size(buffer inBuffer,int inLen)
{
   ByteArray b = (ByteArray)inBuffer;
   b->__SetSize(inLen);
}


void buffer_append_sub(buffer inBuffer,const char *inStr,int inLen)
{
   ByteArray b = (ByteArray)inBuffer;
   int olen = b->length;
   b->__SetSize(olen+inLen);
   memcpy(b->GetBase()+olen,inStr,inLen);
}


void buffer_append_char(buffer inBuffer,int inChar)
{
   ByteArray b = (ByteArray)inBuffer;
   b->Add(inChar);
}


char * buffer_data(buffer inBuffer)
{
   ByteArray b = (ByteArray)inBuffer;
   return b->GetBase();
}


// Append value to buffer
void val_buffer(buffer inBuffer,value inValue)
{
   hx::Object *obj = (hx::Object *)inValue;
   if (obj)
   {
       buffer_append(inBuffer, obj->toString().__CStr());
   }
   else
   {
      buffer_append_sub(inBuffer,"null",4);
   }
}






// Call Function 
hx::Object * val_call0(hx::Object * arg1) THROWS
{
   if (!arg1) Dynamic::ThrowBadFunctionError();
   return arg1->__run().GetPtr();
}

hx::Object * val_call0_traceexcept(hx::Object * arg1) THROWS
{
   try
   {
   if (!arg1) Dynamic::ThrowBadFunctionError();
   return arg1->__run().GetPtr();
   }
   catch(Dynamic e)
   {
      String s = e;
      fprintf(stderr,"Fatal Error : %s\n",s.__CStr());
      exit(1);
   }
   return 0;
}


hx::Object * val_call1(hx::Object * arg1,hx::Object * arg2) THROWS
{
   if (!arg1) Dynamic::ThrowBadFunctionError();
   return arg1->__run(arg2).GetPtr();
}


hx::Object * val_call2(hx::Object * arg1,hx::Object * arg2,hx::Object * arg3) THROWS
{
   if (!arg1) Dynamic::ThrowBadFunctionError();
   return arg1->__run(arg2,arg3).GetPtr();
}


hx::Object * val_call3(hx::Object * arg1,hx::Object * arg2,hx::Object * arg3,hx::Object * arg4) THROWS
{
   if (!arg1) Dynamic::ThrowBadFunctionError();
   return arg1->__run(arg2,arg3,arg4).GetPtr();
}


hx::Object * val_callN(hx::Object * arg1,hx::Object ** arg2, int nCount) THROWS
{
   if (!arg1) Dynamic::ThrowBadFunctionError();
   Array<Dynamic> args = Array_obj<Dynamic>::__new(0, nCount);
   while (nCount--)
     args << *arg2++;
   return arg1->__Run( args ).GetPtr();
}


// Call object field
hx::Object * val_ocall0(hx::Object * arg1,int arg2) THROWS
{
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   return arg1->__IField(arg2)->__run().GetPtr();
}


hx::Object * val_ocall1(hx::Object * arg1,int arg2,hx::Object * arg3) THROWS
{
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   return arg1->__IField(arg2)->__run(arg3).GetPtr();
}


hx::Object * val_ocall2(hx::Object * arg1,int arg2,hx::Object * arg3,hx::Object * arg4) THROWS
{
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   return arg1->__IField(arg2)->__run(arg3,arg4).GetPtr();
}


hx::Object * val_ocall3(hx::Object * arg1,int arg2,hx::Object * arg3,hx::Object * arg4,hx::Object * arg5) THROWS
{
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   return arg1->__IField(arg2)->__run(arg3,arg4,arg5).GetPtr();
}


hx::Object * val_ocallN(hx::Object * arg1,int arg2,hx::Object * arg3) THROWS
{
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   return arg1->__IField(arg2)->__run(Dynamic(arg3)).GetPtr();
}



// Objects access
int val_id(const char * arg1)
{
   return __hxcpp_field_to_id(arg1);
}


void alloc_field(hx::Object * arg1,int arg2,hx::Object * arg3) THROWS
{
   //hx::InternalCollect();
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   arg1->__SetField(__hxcpp_field_from_id(arg2),arg3, HX_PROP_DYNAMIC );
}
void hxcpp_alloc_field(hx::Object * arg1,int arg2,hx::Object * arg3)
{
   return alloc_field(arg1,arg2,arg3);
}

void alloc_field_numeric(hx::Object * arg1,int arg2,double arg3) THROWS
{
   //hx::InternalCollect();
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   arg1->__SetField(__hxcpp_field_from_id(arg2),arg3, HX_PROP_DYNAMIC );
}
void hxcpp_alloc_field_numeric(hx::Object * arg1,int arg2,double arg3)
{
   return alloc_field_numeric(arg1,arg2,arg3);
}



hx::Object * val_field(hx::Object * arg1,int arg2) THROWS
{
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   return arg1->__IField(arg2).GetPtr();
}

double val_field_numeric(hx::Object * arg1,int arg2) THROWS
{
   if (!arg1) hx::Throw(HX_INVALID_OBJECT);
   return arg1->__INumField(arg2);
}

value val_field_name(field inField)
{
   return (value)Dynamic(__hxcpp_field_from_id(inField)).mPtr;
}


void val_iter_field_vals(hx::Object *inObj, __hx_field_iter inFunc ,void *inCookie)
{
   if (inObj)
   {
      Array<String> fields = Array_obj<String>::__new(0,0);

      inObj->__GetFields(fields);

      for(int i=0;i<fields->length;i++)
      {
         inFunc((value)Dynamic(inObj->__Field(fields[i], HX_PROP_NEVER )).mPtr, __hxcpp_field_to_id(fields[i].__CStr()), inCookie);
      }
   }
}


void val_iter_fields(hx::Object *inObj, __hx_field_iter inFunc ,void *inCookie)
{
   if (inObj)
   {
      Array<String> fields = Array_obj<String>::__new(0,0);

      inObj->__GetFields(fields);

      for(int i=0;i<fields->length;i++)
      {
         inFunc((value)inObj, __hxcpp_field_to_id(fields[i].__CStr()), inCookie);
      }
   }
}



   // Abstract types
vkind alloc_kind()
{
   return (vkind)(intptr_t)hxcpp_alloc_kind();
}

void kind_share(vkind *inKind,const char *inName)
{
   int k = (int)(intptr_t)*inKind;
   hxcpp_kind_share(k,inName);
   *inKind = (vkind)(intptr_t)k;
}



// Garbage Collection
void * hx_alloc(int arg1)
{
   return hx::NewGCBytes(0,arg1);
}


void * alloc_private(int arg1)
{
   return hx::NewGCPrivate(0,arg1);
}

hx::Object * alloc_raw_string(int length)
{
   return Dynamic( String( (HX_CHAR *) alloc_private(length+1), length) ).GetPtr();
}

void  val_gc(hx::Object * arg1,hx::finalizer arg2) THROWS
{
   hx::Abstract_obj *abstract = dynamic_cast<hx::Abstract_obj *>(arg1);
   if (!abstract)
   {
      hx::GCSetFinalizer(arg1,arg2);
      //hx::Throw(HX_CSTRING("Finalizer not on abstract object"));
   }
   else
      abstract->SetFinalizer(arg2);
}

void  val_gc_ptr(void * arg1,hxPtrFinalizer arg2) THROWS
{
   hx::Throw(HX_CSTRING("Finalizer not supported here"));
}

void  val_gc_add_root(hx::Object **inRoot)
{
   hx::GCAddRoot(inRoot);
}


void  val_gc_remove_root(hx::Object **inRoot)
{
   hx::GCRemoveRoot(inRoot);
}

void  gc_set_top_of_stack(int *inTopOfStack,bool inForce)
{
   hx::SetTopOfStack(inTopOfStack,inForce);
}


void gc_change_managed_memory(int inDelta, const char *inWhy)
{
   hx::GCChangeManagedMemory(inDelta, inWhy);
}


value *alloc_root()
{
   hx::Object ** result = new hx::Object *();
   hx::GCAddRoot(result);
   return (value *)result;
}

void free_root(value *inValue)
{
   hx::Object **root = (hx::Object **) inValue;
   hx::GCRemoveRoot(root);
   delete root;
}


// Used for finding functions in static libraries
int hx_register_prim( const char * arg1, void* arg2)
{
   __hxcpp_register_prim(arg1,arg2);
   return 0;
}

void gc_enter_blocking()
{
   hx::EnterGCFreeZone();
}

void gc_exit_blocking()
{
   hx::ExitGCFreeZone();
}


bool gc_try_blocking()
{
   return hx::TryGCFreeZone();
}

bool gc_try_unblocking()
{
   return hx::TryExitGCFreeZone();
}


void gc_safe_point()
{
   if (hx::gPauseForCollect)
      hx::PauseForCollect();
}

gcroot create_root(value) { return 0; }
value query_root(gcroot) { return 0; }
void destroy_root(gcroot) { }


String alloc_hxs_wchar(const wchar_t *ptr,int size)
{
   return String::create(ptr,size);
}

String alloc_hxs_utf16(const char16_t *ptr,int size)
{
   return String::create(ptr,size);
}

String alloc_hxs_utf8(const char *ptr,int size)
{
   return String::create(ptr,size);
}

const char * hxs_utf8(const String &string,hx::IStringAlloc *alloc)
{
   return string.utf8_str(alloc);
}

const wchar_t * hxs_wchar(const String &string,hx::IStringAlloc *alloc)
{
   return string.wchar_str(alloc);
}

const char16_t * hxs_utf16(const String &string,hx::IStringAlloc *alloc)
{
   return string.wc_str(alloc);
}


EXPORT void * hx_cffi(const char *inName)
{
   #define HXCPP_PRIME
   #define DEFFUNC(name,r,b,c) if ( !strcmp(inName,#name) ) return (void *)name;

   #include <hx/CFFIAPI.h>

   return 0;
}

}
