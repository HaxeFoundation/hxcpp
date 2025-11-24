#pragma once

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
			bool done;
			int read;
			int write;

			Result() = default;
		};
	}
}