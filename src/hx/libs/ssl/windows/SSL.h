#pragma once

#include <hxcpp.h>
#include <wincrypt.h>
#include <ncrypt.h>

HX_DECLARE_CLASS3(hx, ssl, windows, Cert)
HX_DECLARE_CLASS3(hx, ssl, windows, Key)

namespace hx
{
	namespace ssl
	{
		namespace windows
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

				NCRYPT_KEY_HANDLE ctx;

				Key_obj(NCRYPT_KEY_HANDLE inCtx);

				String toString() override;
			};
		}
	}
}