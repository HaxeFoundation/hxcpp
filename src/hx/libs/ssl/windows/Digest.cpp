#include <hxcpp.h>
#include <Windows.h>

#include "SSL.h"
#include "Cng.h"

Array<unsigned char> _hx_ssl_dgst_make(Array<unsigned char> buffer, String algId)
{
	hx::strbuf buf;

	auto algorithm = hx::ssl::windows::CngAlgorithm::create(algId.wchar_str(&buf), 0);
	auto hash      = algorithm->hash();

	hash->hash(buffer);

	return hash->finish();
}

Array<unsigned char> _hx_ssl_dgst_sign(Array<unsigned char> buffer, Dynamic hpkey, String algId)
{
	hx::strbuf buf;

	auto key       = hpkey.Cast<hx::ssl::windows::Key>();
	auto algorithm = algId.wchar_str(&buf);
	auto alg       = hx::ssl::windows::CngAlgorithm::create(algorithm, 0);
	auto hash      = alg->hash();
	auto hashed    = std::vector<uint8_t>(hash->getProperty<DWORD>(BCRYPT_HASH_LENGTH));

	hash->hash(buffer);
	hash->finish(hashed.data(), hashed.size());

	hx::EnterGCFreeZone();

	auto result          = DWORD{ 0 };
	auto padding         = BCRYPT_PKCS1_PADDING_INFO{ algorithm };
	auto signatureLength = DWORD{ 0 };
	if (ERROR_SUCCESS != (result = NCryptSignHash(key->ctx, &padding, reinterpret_cast<PUCHAR>(hashed.data()), hashed.size(), nullptr, 0, &signatureLength, BCRYPT_PAD_PKCS1)))
	{
		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to signature length : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	hx::ExitGCFreeZone();
	auto signature = Array<uint8_t>(signatureLength, signatureLength);
	hx::EnterGCFreeZone();

	if (ERROR_SUCCESS != (result = NCryptSignHash(key->ctx, &padding, reinterpret_cast<PUCHAR>(hashed.data()), hashed.size(), reinterpret_cast<PUCHAR>(signature->GetBase()), signature->length, &signatureLength, BCRYPT_PAD_PKCS1)))
	{
		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to sign hash : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	hx::ExitGCFreeZone();

	return signature;
}

bool _hx_ssl_dgst_verify(Array<unsigned char> buffer, Array<unsigned char> sign, Dynamic hpkey, String algId)
{
	hx::strbuf buf;

	auto key       = hpkey.Cast<hx::ssl::windows::Key>();
	auto algorithm = algId.wchar_str(&buf);
	auto alg       = hx::ssl::windows::CngAlgorithm::create(algorithm, 0);
	auto hash      = alg->hash();
	auto hashed    = std::vector<uint8_t>(hash->getProperty<DWORD>(BCRYPT_HASH_LENGTH));

	hash->hash(buffer);
	hash->finish(hashed.data(), hashed.size());

	hx::EnterGCFreeZone();

	auto padding = BCRYPT_PKCS1_PADDING_INFO{ algorithm };
	auto success = ERROR_SUCCESS == NCryptVerifySignature(key->ctx, &padding, hashed.data(), hashed.size(), reinterpret_cast<PUCHAR>(sign->GetBase()), sign->length, BCRYPT_PAD_PKCS1);

	hx::ExitGCFreeZone();

	return success;
}