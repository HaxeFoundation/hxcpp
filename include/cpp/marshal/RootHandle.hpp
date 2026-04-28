#pragma once

#include "Definitions.inc"

inline cpp::marshal::RootHandle::RootHandle(::hx::Object** obj) : object(obj) {}

inline cpp::marshal::RootHandle cpp::marshal::RootHandle::create(::Dynamic obj)
{
	if (null() == obj)
	{
		hx::NullReference("Dynamic", false);
	}

	auto root = new hx::Object* { obj.mPtr };

	hx::GCAddRoot(root);

	return RootHandle(root);
}

inline cpp::marshal::RootHandle cpp::marshal::RootHandle::fromVoidPointer(cpp::Pointer<void> ptr)
{
	if (nullptr == ptr.ptr)
	{
		hx::NullReference("Dynamic", false);
	}

	return RootHandle(static_cast<hx::Object**>(ptr.ptr));
}

inline void cpp::marshal::RootHandle::close()
{
	hx::GCRemoveRoot(object);

	delete object;
}

inline ::Dynamic cpp::marshal::RootHandle::getObject()
{
	return ::Dynamic{ *object };
}

inline cpp::Pointer<void> cpp::marshal::RootHandle::toVoidPointer()
{
	return object;
}