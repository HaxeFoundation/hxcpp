#pragma once

#include <hxcpp.h>
#include <wincrypt.h>
#include <bcrypt.h>

HX_DECLARE_CLASS3(hx, ssl, windows, Cert)
HX_DECLARE_CLASS3(hx, ssl, windows, Key)

namespace hx::ssl::windows
{
	class Cert_obj : public hx::Object
	{
	public:
		HX_IS_INSTANCE_OF enum { _hx_classId = hx::clsIdSslCert };

		PCCERT_CONTEXT ctx;

		Cert_obj(PCCERT_CONTEXT inCtx);

		String toString() override;
	};

	class Key_obj : public hx::Object
	{
	public:
		HX_IS_INSTANCE_OF enum { _hx_classId = hx::clsIdSslKey };

		BCRYPT_KEY_HANDLE ctx;

		Key_obj(BCRYPT_KEY_HANDLE inCtx);

		String toString() override;
	};
}