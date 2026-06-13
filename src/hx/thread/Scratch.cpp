#include <hxcpp.h>
#include <hx/thread/Scratch.hpp>
#include "ThreadImpl.hpp"

hx::thread::Scratch hx::thread::Scratch::alloc(int bytes)
{
	auto current = reinterpret_cast<hx::thread::ThreadImpl_obj*>(hx::thread::Thread_obj::current().GetPtr());
	auto required = bytes + sizeof(int);

	// If there is not enough space in the thread array then do a standard malloc.
	// Maybe we want some warning log message so the user knows it's hitting a slower path?
	if (required > static_cast<uint64_t>(current->scratch->length) - current->cursor)
	{
		auto alloc   = std::malloc(required);
		auto storage = new(alloc) int;
		auto view    = cpp::marshal::View<uint8_t>(static_cast<uint8_t*>(alloc) + sizeof(int), bytes);

		std::memset(alloc, 0, required);

		return
			hx::thread::Scratch(storage, view, [](cpp::marshal::View<uint8_t> view) {
				std::free(view.ptr.ptr - sizeof(int));
			});
	}
	else
	{
		auto storage = new(current->scratch->GetBase() + current->cursor) int;
		auto view    = cpp::marshal::View<uint8_t>(current->scratch->GetBase() + current->cursor + sizeof(int), bytes);

		current->cursor += bytes + sizeof(int);

		return
			hx::thread::Scratch(storage, view, [](cpp::marshal::View<uint8_t> view) {
				view.fill(0);

				auto current = reinterpret_cast<hx::thread::ThreadImpl_obj*>(hx::thread::Thread_obj::current().GetPtr());

				current->cursor -= view.length + sizeof(int);
			});
	}
}

hx::thread::Scratch::Scratch(int* _count, cpp::marshal::View<uint8_t> _view, ReleaseFunc _release) : count(_count), view(_view), release(_release)
{
	(*count)++;
}

hx::thread::Scratch::Scratch(const Scratch& _other) : count(_other.count), view(_other.view), release(_other.release)
{
	(*count)++;
}

hx::thread::Scratch::~Scratch()
{
	if (0 == --(*count))
	{
		release(view);
	}
}

hx::thread::Scratch& hx::thread::Scratch::operator=(const Scratch& _other)
{
	auto old = *count;

	count = _other.count;
	(*count)++;

	if (0 == (--old))
	{
		release(view);
	}

	return *this;
}