#include <hxcpp.h>
#include "SSL.h"

namespace
{
	void DestroyCert(Dynamic obj)
	{
		auto cert = obj.Cast<hx::ssl::windows::Cert>();

		CertFreeCertificateContext(cert->ctx);

		cert->ctx = nullptr;
	}

	void DestroyKey(Dynamic obj)
	{
		auto key = obj.Cast<hx::ssl::windows::Key>();

		BCryptDestroyKey(key->ctx);
	}
}

hx::ssl::windows::Cert_obj::Cert_obj(PCCERT_CONTEXT inCtx) : ctx(inCtx)
{
	_hx_set_finalizer(this, DestroyCert);
}

String hx::ssl::windows::Cert_obj::toString()
{
	return HX_CSTRING("cert");
}

hx::ssl::windows::Key_obj::Key_obj(BCRYPT_KEY_HANDLE inCtx) : ctx(inCtx)
{
	_hx_set_finalizer(this, DestroyKey);
}

String hx::ssl::windows::Key_obj::toString()
{
	return HX_CSTRING("key");
}