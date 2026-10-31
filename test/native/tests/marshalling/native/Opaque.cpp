#include <Opaque.hpp>

struct Opaque {};

Opaque* opaque_create()
{
	return new Opaque();
}