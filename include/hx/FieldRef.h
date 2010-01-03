#ifndef HX_FIELD_REF_H
#define HX_FIELD_REF_H

namespace hx
{

// --- FieldRef ----------------------------------------------------------
//
//  This is used to provide syntaxe for setting fields by name.  This is because
//   the field can't be returned by reference, because it may not exist as a dynamic.
//
//  eg, consider class 'A' with variable 'x':
//  class A { int x; }
//
//  And you have a Dynamic pointing to it:
//  Dynamic d = new A;  Then you access x by name:
//  d->__Field("x") = 1;
//
//  __Field can't return a Dynamic & because x is a int, not Dynamic. So I use this class.
//  Note that this may change if I fix the generator to create __SetField("x",1) directly.

class FieldRef
{
public:
   explicit FieldRef(hx::Object *inObj,const String &inName) : mObject(inObj), mName(inName)
   {
   }

   Dynamic operator=(const Dynamic &inRHS)
   {
      return mObject->__SetField(mName,inRHS);
   }
   inline operator Dynamic() const { return mObject->__Field(mName); }
   inline operator double() const { return mObject->__Field(mName); }
   inline operator int() const { return mObject->__Field(mName); }

   // post-increment
   inline double operator++(int)
   {
      double d = mObject->__Field(mName)->__ToDouble();
      mObject->__SetField(mName,d+1);
      return d;
   }
   // pre-increment
   inline double operator++()
   {
      double d = mObject->__Field(mName)->__ToDouble() + 1;
      mObject->__SetField(mName,d);
      return d;
   }
   // post-decrement
   inline double operator--(int)
   {
      double d = mObject->__Field(mName)->__ToDouble();
      mObject->__SetField(mName,d-1);
      return d;
   }
   // pre-decrement
   inline double operator--()
   {
      double d = mObject->__Field(mName)->__ToDouble() - 1;
      mObject->__SetField(mName,d);
      return d;
   }
   bool operator !() { return ! mObject->__Field(mName)->__ToInt(); }


   String  mName;
   hx::Object *mObject;
};

// We can define this one now...
template<typename T>
inline FieldRef ObjectPtr<T>::FieldRef(const String &inString)
{
   return hx::FieldRef(mPtr,inString);
}

} // end namespace hx


#endif
