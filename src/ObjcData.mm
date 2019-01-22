#include <hxcpp.h>
#include <math.h>
#include <hxMath.h>
#include <stdio.h>

using namespace hx;



@implementation NSHaxeWrapperClass

- (id)init:( hx::Object * ) inHaxe  {
   self = [super init];
   haxeObject = inHaxe;
   hx::GCAddRoot(&haxeObject);
   if (self!=nil) {
   }
   return self;
}

- (void)dealloc {
   GCRemoveRoot(&haxeObject);
   #ifndef OBJC_ARC
   [super dealloc];
   #endif
}
@end



id _hx_value_to_objc(Dynamic d)
{
   if (!d.mPtr)
      return nil;

   switch(d->__GetType())
   {
      case vtNull:
      case vtUnknown:
        return nil;

      case vtInt:
      case vtBool:
         return [ NSNumber numberWithInt: (int)d ];
      case vtInt64:
         return [ NSNumber numberWithLongLong: (cpp::Int64)d ];
      case vtFloat:
         return [ NSNumber numberWithDouble: (double)d ];
      case vtString:
         return (NSString *)String(d);
      case vtObject:
         return _hx_obj_to_nsdictionary(d);
      case vtArray:
         {
            Array_obj<unsigned char> *bytes = dynamic_cast< Array_obj<unsigned char> * >(d.mPtr);
            if (bytes)
            {
               if (!bytes->length)
                  return [NSData alloc];
               else
                  return [NSData dataWithBytes:bytes->GetBase() length:bytes->length];
            }
            else
            {
               cpp::VirtualArray varray = d;
               int len = varray->get_length();
               NSMutableArray *array = [NSMutableArray arrayWithCapacity:len];
               for(int i=0;i<len;i++)
                  array[i] = _hx_value_to_objc( varray->__get(i) );
               return array;
            }
         }
      case vtClass:
         if (d->__GetClass()->mName==HX_CSTRING("haxe.io.Bytes"))
         {
            return _hx_value_to_objc( d->__Field( HX_CSTRING("b"), hx::paccDynamic ) );
         }
         // else fallthough...
        
      case vtFunction:
      case vtEnum:
      case vtAbstractBase:
         {
         return (NSString *)d->toString();
         }
   }
   return nil;
}


NSDictionary<NSString *, id> *_hx_obj_to_nsdictionary(Dynamic d)
{
   if (d==null())
      return nil;

   Array<String> fields = Array_obj<String>::__new();
   d->__GetFields(fields);

   NSMutableArray *objects = [[NSMutableArray alloc] initWithCapacity:fields->length ];
   NSMutableArray *keys = [[NSMutableArray alloc] initWithCapacity:fields->length ];
   for(int i=0; i <fields->length; i++)
   {
      objects[i] = _hx_value_to_objc( d->__Field(fields[i], hx::paccDynamic ) );
      keys[i] = (NSString *)fields[i];
   }

   NSDictionary *result =  [NSDictionary<NSString *,id> dictionaryWithObjects:objects forKeys:keys];

   return result;
}

Array<unsigned char> _hx_objc_to_bytes(id value)
{
     NSData *data = value;
     return Array_obj<unsigned char>::fromData((const unsigned char *)data.bytes, data.length);
}

hx::Val _hx_objc_to_value(id value)
{
   if (value==nil)
      return null();

   else if ([value isKindOfClass:[NSNumber class]])
      return [value floatValue];

   else if ([value isKindOfClass:[NSString class]])
      return String( (NSString *)value );

  else if ([value isKindOfClass:[NSDictionary class]])
      return _hx_nsdictionary_to_obj( (NSDictionary *) value);

  else if ([value isKindOfClass:[NSArray class]])
  {
      NSArray *array = value;
      cpp::VirtualArray varray = cpp::VirtualArray_obj::__new();
      for(id object in array)
         varray->push( _hx_objc_to_value(object) );

       return varray;
  }
  else if ([value isKindOfClass:[NSData class]])
  {
     return _hx_objc_to_bytes(value);
  }
  else
     return String( [value stringValue] );
}




Dynamic _hx_nsdictionary_to_obj(NSDictionary<NSString *, id> *inDictionary)
{
   if (!inDictionary)
      return null();
   hx::Anon obj = new hx::Anon_obj();

   for (NSString *key in inDictionary)
   {
      id value = inDictionary[key];
      obj->__SetField( String(key), _hx_objc_to_value(value), hx::paccDynamic);
   }
   return obj;
}



Dynamic _hx_objc_to_dynamic(id inValue)
{
   return _hx_objc_to_value(inValue);
}



namespace hx {

extern hx::Class __ObjcClass;

class ObjcData : public hx::Object
{
public:
   inline void *operator new( size_t inSize, hx::NewObjectType inAlloc=NewObjAlloc,const char *inName="objc.BoxedType")
      { return hx::Object::operator new(inSize,inAlloc,inName); }

   ObjcData(const id inValue) : mValue(inValue) 
   {
      #ifndef OBJC_ARC
      [ inValue retain ];
      #endif
      mFinalizer = new hx::InternalFinalizer(this,clean);
   };

   static void clean(hx::Object *inObj)
   {
      ObjcData *m = dynamic_cast<ObjcData *>(inObj);
      if (m)
      {
         #ifndef OBJC_ARC
         [m->mValue release];
         #else
         m->mValue = nil;
         #endif
      }
   }

   #ifdef HXCPP_VISIT_ALLOCS
   void __Visit(hx::VisitContext *__inCtx) { mFinalizer->Visit(__inCtx); }
   #endif

   hx::Class __GetClass() const { return hx::__ObjcClass; }

   #if (HXCPP_API_LEVEL<331)
   bool __Is(hx::Object *inClass) const { return dynamic_cast< ObjcData *>(inClass); }
   #endif

   // k_cpp_objc
   int __GetType() const { return vtAbstractBase + 4; }
   #ifdef OBJC_ARC
   void * __GetHandle() const { return (__bridge void *) mValue; }
   #else
   void * __GetHandle() const { return (void *) mValue; }
   #endif
   String toString()
   {
      return String(!mValue ? "null" : [[mValue description] UTF8String]);
   }
   String __ToString() const { return String(!mValue ? "null" : [[mValue description] UTF8String]); }

   int __Compare(const hx::Object *inRHS) const
   {
      if (!inRHS)
        return mValue == 0 ? 0 : 1;
      const ObjcData *data = dynamic_cast< const ObjcData *>(inRHS);
      if (data) 
      {
         return [data->mValue isEqual:mValue] ? 0 : mValue < data->mValue ? -1 : 1;
      }
      else
      {
        void *r = inRHS->__GetHandle();

        #ifdef OBJC_ARC
        void * ptr = (__bridge void *) mValue;
        #else
        void * ptr = (void *) mValue;
        #endif
 
        return ptr < r ? -1 : ptr==r ? 0 : 1;
      }
   }

   #ifdef OBJC_ARC
   id mValue;
   #else
   const id mValue;
   #endif
   hx::InternalFinalizer *mFinalizer;
};
}

Dynamic::Dynamic(const id inVal)
{
   mPtr = inVal ? (hx::Object *)new ObjcData(inVal) : 0;
}
