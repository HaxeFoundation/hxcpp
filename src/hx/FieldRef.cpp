#include <hxcpp.h>

#define HX_FIELD_REF_IMPL_MEM_OP(op,ret) \
    ret hx::FieldRef::operator op (const FieldRef& inA) { return this->operator Dynamic() op inA.operator Dynamic(); } \
    ret hx::FieldRef::operator op (const IndexRef& inA) { return this->operator Dynamic() op inA.operator Dynamic(); } \
    ret hx::FieldRef::operator op (const hx::Val& inA) { \
		switch (inA.type) { \
			case hx::Val::typeObject: return this->operator Dynamic() op Dynamic { inA.asDynamic() }; \
			case hx::Val::typeString: return this->operator Dynamic() op inA.asString(); \
			case hx::Val::typeDouble: return this->operator Dynamic() op inA.asDouble(); \
			case hx::Val::typeInt   : return this->operator Dynamic() op inA.asInt(); \
			case hx::Val::typeInt64 : return this->operator Dynamic() op inA.asInt64(); \
			case hx::Val::typeBool  : return this->operator Dynamic() op inA.asInt(); \
		} \
	}

hx::FieldRef::FieldRef(hx::Object* inObj, const String& inName) : mObject(inObj), mName(inName) {}

hx::Val hx::FieldRef::operator=(const hx::Val& inRHS)
{
	return mObject->__SetField(mName, inRHS, HX_PROP_DYNAMIC);
}

hx::FieldRef::operator hx::Val()
{
	return mObject ? mObject->__Field(mName, HX_PROP_DYNAMIC) : null();
}

hx::FieldRef::operator Dynamic() const
{
	return mObject ? Dynamic(mObject->__Field(mName, HX_PROP_DYNAMIC)) : null();
}

hx::FieldRef::operator double() const
{
	return mObject->__Field(mName, HX_PROP_DYNAMIC);
}

hx::FieldRef::operator float() const
{
	return mObject->__Field(mName, HX_PROP_DYNAMIC);
}

hx::FieldRef::operator int() const
{
	return mObject->__Field(mName, HX_PROP_DYNAMIC);
}

hx::FieldRef::operator cpp::UInt64() const
{
	return mObject->__Field(mName, HX_PROP_DYNAMIC);
}

hx::FieldRef::operator cpp::Int64() const
{
	return mObject->__Field(mName, HX_PROP_DYNAMIC);
}

double hx::FieldRef::operator++(int)
{
	double d = mObject->__Field(mName, HX_PROP_DYNAMIC);
	mObject->__SetField(mName, d + 1, HX_PROP_DYNAMIC);
	return d;
}

double hx::FieldRef::operator++()
{
	double d = ((double)mObject->__Field(mName, HX_PROP_DYNAMIC)) + 1;
	mObject->__SetField(mName, d, HX_PROP_DYNAMIC);
	return d;
}

double hx::FieldRef::operator--(int)
{
	double d = mObject->__Field(mName, HX_PROP_DYNAMIC);
	mObject->__SetField(mName, d - 1, HX_PROP_DYNAMIC);
	return d;
}

double hx::FieldRef::operator--()
{
	double d = (double)(mObject->__Field(mName, HX_PROP_DYNAMIC)) - 1;
	mObject->__SetField(mName, d, HX_PROP_DYNAMIC);
	return d;
}

bool hx::FieldRef::operator!()
{
	return !((int)(mObject->__Field(mName, HX_PROP_DYNAMIC)));
}

int hx::FieldRef::operator~()
{
	return ~((int)mObject->__Field(mName, HX_PROP_DYNAMIC));
}

bool hx::FieldRef::operator==(const null&) const
{
	return !mObject;
}

bool hx::FieldRef::operator!=(const null&) const
{
	return mObject;
}

double hx::FieldRef::operator-()
{
	return -(double)(mObject->__Field(mName, HX_PROP_DYNAMIC));
}

bool hx::FieldRef::HasPointer() const
{
	return mObject;
}

HX_FIELD_REF_IMPL_MEM_OP(== , bool)
HX_FIELD_REF_IMPL_MEM_OP(!= , bool)
HX_FIELD_REF_IMPL_MEM_OP(< , bool)
HX_FIELD_REF_IMPL_MEM_OP(<= , bool)
HX_FIELD_REF_IMPL_MEM_OP(> , bool)
HX_FIELD_REF_IMPL_MEM_OP(>= , bool)

HX_FIELD_REF_IMPL_MEM_OP(+, Dynamic)

// Below has the above macros expanded since some operators on some times aren't supported and need manual dynamic wrapping.
// There may be some sort of tagged dispatch which could be done in the macro instead to avoid this.

double hx::FieldRef::operator * (const FieldRef& inA) { return this->operator Dynamic() * inA.operator Dynamic(); }
double hx::FieldRef::operator * (const IndexRef& inA) { return this->operator Dynamic() * inA.operator Dynamic(); }
double hx::FieldRef::operator * (const hx::Val& inA)
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

double hx::FieldRef::operator / (const FieldRef& inA) { return this->operator Dynamic() / inA.operator Dynamic(); }
double hx::FieldRef::operator / (const IndexRef& inA) { return this->operator Dynamic() / inA.operator Dynamic(); }
double hx::FieldRef::operator / (const hx::Val& inA)
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

double hx::FieldRef::operator - (const FieldRef& inA) { return this->operator Dynamic() - inA.operator Dynamic(); }
double hx::FieldRef::operator - (const IndexRef& inA) { return this->operator Dynamic() - inA.operator Dynamic(); }
double hx::FieldRef::operator - (const hx::Val& inA)
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

double hx::FieldRef::operator % (const FieldRef& inA) { return this->operator Dynamic() % inA.operator Dynamic(); }
double hx::FieldRef::operator % (const IndexRef& inA) { return this->operator Dynamic() % inA.operator Dynamic(); }
double hx::FieldRef::operator % (const hx::Val& inA)
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
