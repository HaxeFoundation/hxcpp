function main() {
	// char
	trace(untyped __cpp__('::String::create("Hello world")'));

	// wchar_t
	trace(untyped __cpp__('::String::create(L"Hello world")'));

	// char16_t
	trace(untyped __cpp__('::String::create(u"Hello world")'));

	// explicit 0 length

	// char
	trace(untyped __cpp__('::String::create("Hello world", 0)'));

	// wchar_t
	trace(untyped __cpp__('::String::create(L"Hello world", 0)'));

	// char16_t
	trace(untyped __cpp__('::String::create(u"Hello world", 0)'));
}
