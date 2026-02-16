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


#define HX_FIELD_REF_MEM_OP(op,ret) \
    ret operator op (const FieldRef& inA); \
    ret operator op (const IndexRef& inA); \
    ret operator op (const hx::Val& inA);

class FieldRef
{
public:
    explicit FieldRef(hx::Object* inObj, const String& inName);

    hx::Val operator=(const hx::Val& inRHS);
    operator hx::Val();
    operator Dynamic() const;
    operator double() const;
    operator float() const;
    operator int() const;
    operator cpp::UInt64() const;
    operator cpp::Int64() const;

    // post-increment
    double operator++(int);
    // pre-increment
    double operator++();
    // post-decrement
    double operator--(int);
    // pre-decrement
    double operator--();
    bool operator !();
    int operator ~();

    bool operator==(const null&) const;
    bool operator!=(const null&) const;

    double operator -();

    bool HasPointer() const;

    HX_FIELD_REF_MEM_OP(==,bool)
    HX_FIELD_REF_MEM_OP(!=,bool)
    HX_FIELD_REF_MEM_OP(<,bool)
    HX_FIELD_REF_MEM_OP(<=,bool)
    HX_FIELD_REF_MEM_OP(>,bool)
    HX_FIELD_REF_MEM_OP(>=,bool)

    HX_FIELD_REF_MEM_OP(+,Dynamic)
    HX_FIELD_REF_MEM_OP(*,double)
    HX_FIELD_REF_MEM_OP(/,double)
    HX_FIELD_REF_MEM_OP(-,double)
    HX_FIELD_REF_MEM_OP(%,double)

    String  mName;
    hx::Object *mObject;
};

// We can define this one now...
template<typename T>
inline FieldRef ObjectPtr<T>::FieldRef(const String &inString)
{
   return hx::FieldRef(mPtr,inString);
}

#define HX_FIELD_REF_OP(op,ret) \
template<typename T> inline ret operator op (T &inT, const FieldRef &inA) \
   { return inT op ( inA.operator Dynamic()); }

HX_FIELD_REF_OP(==,bool)
HX_FIELD_REF_OP(!=,bool)
HX_FIELD_REF_OP(<,bool)
HX_FIELD_REF_OP(<=,bool)
HX_FIELD_REF_OP(>,bool)
HX_FIELD_REF_OP(>=,bool)

HX_FIELD_REF_OP(+,Dynamic)
HX_FIELD_REF_OP(*,double)
HX_FIELD_REF_OP(/,double)
HX_FIELD_REF_OP(-,double)
HX_FIELD_REF_OP(%,double)



// --- IndexRef --------------------------------------------------------------
//
// Like FieldRef, but for integer array access
//

#define HX_INDEX_REF_MEM_OP(op,ret) \
    ret operator op (const IndexRef &inA); \
    ret operator op (const FieldRef &inA); \
    ret operator op (const hx::Val& inA);

class IndexRef
{
public:
    explicit IndexRef(hx::Object* inObj, int inIndex);

    Dynamic operator=(const Dynamic& inRHS);
    operator Dynamic() const;
    operator double() const;
    operator int() const;

    // post-increment
    double operator++(int);
    // pre-increment
    double operator++();
    // post-decrement
    double operator--(int);
    // pre-decrement
    double operator--();

    bool operator !();
    int operator ~();
    double operator -();

    bool operator==(const null&) const;
    bool operator!=(const null&) const;

    HX_INDEX_REF_MEM_OP(== , bool)
    HX_INDEX_REF_MEM_OP(!= , bool)
    HX_INDEX_REF_MEM_OP(< , bool)
    HX_INDEX_REF_MEM_OP(<= , bool)
    HX_INDEX_REF_MEM_OP(> , bool)
    HX_INDEX_REF_MEM_OP(>= , bool)

    HX_INDEX_REF_MEM_OP(+, Dynamic)
    HX_INDEX_REF_MEM_OP(*, double)
    HX_INDEX_REF_MEM_OP(/ , double)
    HX_INDEX_REF_MEM_OP(-, double)
    HX_INDEX_REF_MEM_OP(%, double)

    bool HasPointer() const;

    int mIndex;
    hx::Object *mObject;
};

// We can define this one now...
template<typename T>
inline IndexRef ObjectPtr<T>::IndexRef(int inIndex)
{
   return hx::IndexRef(mPtr,inIndex);
}

#define HX_INDEX_REF_OP(op,ret) \
template<typename T> inline ret operator op (T &inT, const IndexRef &inA) \
   { return inT op ( inA. operator Dynamic()); }

HX_INDEX_REF_OP(==,bool)
HX_INDEX_REF_OP(!=,bool)
HX_INDEX_REF_OP(<,bool)
HX_INDEX_REF_OP(<=,bool)
HX_INDEX_REF_OP(>,bool)
HX_INDEX_REF_OP(>=,bool)

HX_INDEX_REF_OP(+,Dynamic)
HX_INDEX_REF_OP(*,double)
HX_INDEX_REF_OP(/,double)
HX_INDEX_REF_OP(-,double)
HX_INDEX_REF_OP(%,double)


// Disambiguate Dynamic operators...

#define HX_INDEX_REF_OP_DYNAMIC(op,ret) \
inline ret operator op (const Dynamic &inT, const IndexRef &inA) \
   { return inT op ( inA.operator Dynamic()); }

HX_INDEX_REF_OP_DYNAMIC(==,bool)
HX_INDEX_REF_OP_DYNAMIC(!=,bool)
HX_INDEX_REF_OP_DYNAMIC(+,Dynamic)
HX_INDEX_REF_OP_DYNAMIC(*,double)



template<typename _OBJ>
class __TArrayImplRef
{
public:
   _OBJ mObject;
   int mIndex;

   explicit __TArrayImplRef(_OBJ inObj,int inIndex) : mObject(inObj), mIndex(inIndex) { }

   template<typename _DATA>
   inline operator _DATA() { return mObject->__get(mIndex); }
   template<typename _DATA>
   inline void operator=(_DATA inRHS)
   {
      mObject->__set(mIndex,inRHS);
   }
};

template<typename _OBJ>
__TArrayImplRef<_OBJ> __ArrayImplRef(_OBJ inObj, int inIndex)
{
   return __TArrayImplRef<_OBJ>(inObj,inIndex);
}



} // end namespace hx


#endif
