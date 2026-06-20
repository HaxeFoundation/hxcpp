#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

#include <hx/thread/Thread.hpp>
#include <hx/thread/CountingSemaphore.hpp>
#include <array>
#include <thread>

HX_DECLARE_CLASS2(hx, thread, ThreadImpl)

namespace hx
{
	namespace thread
	{
		class ThreadImpl_obj final : public Thread_obj
		{
			int cursor;
			Array<uint8_t> scratch;
			Array<Dynamic> slots;

		public:
			friend class Scratch;

			struct Native
			{
				std::thread thread;
				std::thread::native_handle_type handle;

				Native();
				Native(Thread_obj::CreateFunction _job, ThreadImpl _thread, CountingSemaphore _semaphore);
			};

			Native* native;
			const int id;

			ThreadImpl_obj(const int _id);
			ThreadImpl_obj(const int _id, Thread_obj::CreateFunction _run, CountingSemaphore _semaphore);

			String getName() HXCPP_OVERRIDE;
			void setName(const String& name) HXCPP_OVERRIDE;

			Dynamic getSlot(const int id);
			void setSlot(const int id, const Dynamic& obj);

			String toString() HXCPP_OVERRIDE;

			void __Mark(HX_MARK_PARAMS) HXCPP_OVERRIDE;
#ifdef HXCPP_VISIT_ALLOCS
			void __Visit(HX_VISIT_PARAMS) HXCPP_OVERRIDE;
#endif

			static void finalise(hx::Object* obj);
		};
	}
}
