#pragma once

#ifndef HXCPP_H
#include <hxcpp.h>
#endif

HX_DECLARE_CLASS2(hx, zip, Zip)

namespace hx
{
	namespace zip
	{
		enum Flush {
			None,
			Sync,
			Full,
			Finish,
			Block
		};

		struct Result final {
			const bool done;
			const int read;
			const int write;

			Result(int inDone, int inRead, int inWrite);
		};

		struct Zip_obj : hx::Object
		{
			virtual Result execute(cpp::marshal::View<uint8_t> src, cpp::marshal::View<uint8_t> dst) = 0;
			virtual void setFlushMode(Flush mode) = 0;
			virtual void close() = 0;
		};
	}
}