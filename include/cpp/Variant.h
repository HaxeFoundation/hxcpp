#ifndef CPP_VARIANT_H
#define CPP_VARIANT_H

namespace cpp
{

struct Variant
{
   enum Type
   {
      typeNull = 0,
      typeInt,
      typeDouble,
      typeBool,
      typePointer,
      typeString,
      typeDynamic,
      typeObjC,
   };

   Type type;
   union
   {
      int valInt;
      double valDouble;
      bool valBool;
      String valString;
      Dynamic valDynamic;
      void *valPointer;
      #if defined(__OBJC__) && defined(HXCPP_OBJC)
      id  valObjC;
      #endif
   };

   inline Variant() : type(typeNull), valPointer(0) { }
   inline Variant(const null &) : type(typeNull), valPointer(0) { }
   inline Variant(bool inValue) : type(typeBool), valBool(inValue) { }
   inline Variant(int inValue) : type(typeInt), valInt(inValue) { }
   inline Variant(double inValue) : type(typeDouble), valDouble(inValue) { }
   inline Variant(::String inValue) : type(typeString), valString(inValue) { }
   #if defined(__OBJC__) && defined(HXCPP_OBJC)
   inline Variant(const id inValue) : type(typeObjC), valObjC(inValue) { }
   #endif

   inline explicit Variant(const Dynamic &inRHS) : type(typeDynamic), valDynamic(inRHS) { }
   inline Variant(hx::Object *inValue) : type(typeDynamic), valDynamic(inValue) { }
   inline Variant(void *inValue) : type(typePointer), valPointer(inValue) { }



   inline void mark(hx::MarkContext *__inCtx)
   {
      if (type==typeString)
      {
         HX_MARK_STRING(valString.__s);
      }
      else if (type==typeDynamic)
      {
         HX_MARK_OBJECT(valDynamic.mPtr);
      }
   }
   #ifdef HXCPP_VISIT_ALLOCS
   void visit(hx::VisitContext *__inCtx)
   {
      if (type==typeString)
      {
         HX_VISIT_STRING(valString.__s);
      }
      else if (type==typeDynamic)
      {
         HX_VISIT_OBJECT(valDynamic.mPtr);
      }
   }
   #endif

};


} // end namespace cpp

#endif
