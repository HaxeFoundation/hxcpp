#include <hxcpp.h>

#define HX_INDEX_REF_IMPL_MEM_OP(op,ret) \
    ret hx::IndexRef::operator op (const FieldRef& inA) { return this->operator Dynamic() op inA.operator Dynamic(); } \
    ret hx::IndexRef::operator op (const IndexRef& inA) { return this->operator Dynamic() op inA.operator Dynamic(); } \
    ret hx::IndexRef::operator op (const hx::Val& inA) { \
		switch (inA.type) { \
			case hx::Val::typeObject: return this->operator Dynamic() op Dynamic { inA.asDynamic() }; \
			case hx::Val::typeString: return this->operator Dynamic() op inA.asString(); \
			case hx::Val::typeDouble: return this->operator Dynamic() op inA.asDouble(); \
			case hx::Val::typeInt   : return this->operator Dynamic() op inA.asInt(); \
			case hx::Val::typeInt64 : return this->operator Dynamic() op inA.asInt64(); \
			case hx::Val::typeBool  : return this->operator Dynamic() op inA.asInt(); \
		} \
	}

hx::IndexRef::IndexRef(hx::Object* inObj, int inIndex) : mObject(inObj), mIndex(inIndex) {}

Dynamic hx::IndexRef::operator=(const Dynamic& inRHS)
{
	return mObject->__SetItem(mIndex, inRHS);
}

hx::IndexRef::operator Dynamic() const
{
	return mObject->__GetItem(mIndex);
}

hx::IndexRef::operator double() const
{
	return mObject->__GetItem(mIndex);
}

hx::IndexRef::operator int() const
{
	return mObject->__GetItem(mIndex);
}

double hx::IndexRef::operator ++(int)
{
	double d = mObject->__GetItem(mIndex)->__ToDouble();
	mObject->__SetItem(mIndex, d + 1);
	return d;
}

double hx::IndexRef::operator ++()
{
	double d = mObject->__GetItem(mIndex)->__ToDouble() + 1;
	mObject->__SetItem(mIndex, d);
	return d;
}

double hx::IndexRef::operator --(int)
{
	double d = mObject->__GetItem(mIndex)->__ToDouble();
	mObject->__SetItem(mIndex, d - 1);
	return d;
}

double hx::IndexRef::operator --()
{
	double d = mObject->__GetItem(mIndex)->__ToDouble() - 1;
	mObject->__SetItem(mIndex, d);
	return d;
}

bool hx::IndexRef::operator !()
{
	return !mObject->__GetItem(mIndex)->__ToInt();
}

int hx::IndexRef::operator ~()
{
	return ~mObject->__GetItem(mIndex)->__ToInt();
}

double hx::IndexRef::operator -()
{
	return -mObject->__GetItem(mIndex)->__ToDouble();
}

bool hx::IndexRef::operator ==(const null&) const
{
	return !mObject;
}

bool hx::IndexRef::operator !=(const null&) const
{
	return mObject;
}

bool hx::IndexRef::HasPointer() const
{
	return mObject;
}

HX_INDEX_REF_IMPL_MEM_OP(== , bool)
HX_INDEX_REF_IMPL_MEM_OP(!= , bool)
HX_INDEX_REF_IMPL_MEM_OP(< , bool)
HX_INDEX_REF_IMPL_MEM_OP(<= , bool)
HX_INDEX_REF_IMPL_MEM_OP(> , bool)
HX_INDEX_REF_IMPL_MEM_OP(>= , bool)

HX_INDEX_REF_IMPL_MEM_OP(+, Dynamic)

// Below has the above macros expanded since some operators on some times aren't supported and need manual dynamic wrapping.
// There may be some sort of tagged dispatch which could be done in the macro instead to avoid this.

double hx::IndexRef::operator * (const FieldRef& inA) { return this->operator Dynamic() * inA.operator Dynamic(); }
double hx::IndexRef::operator * (const IndexRef& inA) { return this->operator Dynamic() * inA.operator Dynamic(); }
double hx::IndexRef::operator * (const hx::Val& inA)
{
	switch (inA.type)
	{
	case hx::Val::typeObject: return this->operator double() * inA.asDouble();
	case hx::Val::typeString: return this->operator double() * inA.asDouble();
	case hx::Val::typeDouble: return this->operator double() * inA.asDouble();
	case hx::Val::typeInt: return this->operator double() * inA.asInt();
	case hx::Val::typeInt64: return this->operator double() * inA.asInt64();
	case hx::Val::typeBool: return this->operator double() * inA.asInt();
	}
}

double hx::IndexRef::operator / (const FieldRef& inA) { return this->operator Dynamic() / inA.operator Dynamic(); }
double hx::IndexRef::operator / (const IndexRef& inA) { return this->operator Dynamic() / inA.operator Dynamic(); }
double hx::IndexRef::operator / (const hx::Val& inA)
{
	switch (inA.type) {
	case hx::Val::typeObject: return this->operator double() / inA.asDouble();
	case hx::Val::typeString: return this->operator double() / inA.asDouble();
	case hx::Val::typeDouble: return this->operator double() / inA.asDouble();
	case hx::Val::typeInt: return this->operator double() / inA.asInt();
	case hx::Val::typeInt64: return this->operator double() / inA.asInt64();
	case hx::Val::typeBool: return this->operator double() / inA.asInt();
	}
}

double hx::IndexRef::operator - (const FieldRef& inA) { return this->operator Dynamic() - inA.operator Dynamic(); }
double hx::IndexRef::operator - (const IndexRef& inA) { return this->operator Dynamic() - inA.operator Dynamic(); }
double hx::IndexRef::operator - (const hx::Val& inA)
{
	switch (inA.type) {
	case hx::Val::typeObject: return this->operator double() - inA.asDouble();
	case hx::Val::typeString: return this->operator double() - inA.asDouble();
	case hx::Val::typeDouble: return this->operator double() - inA.asDouble();
	case hx::Val::typeInt: return this->operator double() - inA.asInt();
	case hx::Val::typeInt64: return this->operator double() - inA.asInt64();
	case hx::Val::typeBool: return this->operator double() - inA.asInt();
	}
}

double hx::IndexRef::operator % (const FieldRef& inA) { return this->operator Dynamic() % inA.operator Dynamic(); }
double hx::IndexRef::operator % (const IndexRef& inA) { return this->operator Dynamic() % inA.operator Dynamic(); }
double hx::IndexRef::operator % (const hx::Val& inA)
{
	switch (inA.type) {
	case hx::Val::typeObject: return this->operator int() % inA.asInt();
	case hx::Val::typeString: return this->operator int() % inA.asInt();
	case hx::Val::typeDouble: return this->operator int() % inA.asInt();
	case hx::Val::typeInt: return this->operator int() % inA.asInt();
	case hx::Val::typeInt64: return this->operator int() % inA.asInt64();
	case hx::Val::typeBool: return this->operator int() % inA.asInt();
	}
}