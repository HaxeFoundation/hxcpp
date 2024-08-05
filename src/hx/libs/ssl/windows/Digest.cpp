#include <hxcpp.h>
#include <Windows.h>

#include "SSL.h"
#include "Utils.h"

Array<unsigned char> _hx_ssl_dgst_make(Array<unsigned char> buffer, String alg)
{
	auto handle    = BCRYPT_ALG_HANDLE();
	auto result    = 0;
	auto cb        = DWORD{ 0 };

	hx::strbuf buf;
	auto algorithm = alg.wchar_str(&buf);

	hx::EnterGCFreeZone();

	if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&handle, algorithm, nullptr, 0)))
	{
		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to open algorithm provider : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto objectLength = DWORD{ 0 };
	if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(DWORD), &cb, 0)))
	{
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to get object length : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto object = std::vector<uint8_t>(objectLength);
	auto hash   = BCRYPT_HASH_HANDLE();
	if (!BCRYPT_SUCCESS(result = BCryptCreateHash(handle, &hash, object.data(), object.size(), nullptr, 0, 0)))
	{
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to create hash object : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto hashLength = DWORD{ 0 };
	if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(DWORD), &cb, 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to get object length : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	if (!BCRYPT_SUCCESS(result = BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer->GetBase()), buffer->length, 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to hash data : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	hx::ExitGCFreeZone();
	auto output = Array<uint8_t>(hashLength, hashLength);
	hx::EnterGCFreeZone();

	if (!BCRYPT_SUCCESS(result = BCryptFinishHash(hash, reinterpret_cast<PUCHAR>(output->GetBase()), output->length, 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to finish hash : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	BCryptDestroyHash(hash);
	BCryptCloseAlgorithmProvider(handle, 0);

	hx::ExitGCFreeZone();

	return output;
}

Array<unsigned char> _hx_ssl_dgst_sign(Array<unsigned char> buffer, Dynamic hpkey, String alg)
{
	auto key       = hpkey.Cast<hx::ssl::windows::Key>();
	auto result    = 0;
	auto dummy     = DWORD{ 0 };
	hx::strbuf buf;
	auto algorithm = alg.wchar_str(&buf);
	auto padding   = BCRYPT_PKCS1_PADDING_INFO{ algorithm };

	hx::EnterGCFreeZone();

	auto handle = BCRYPT_ALG_HANDLE();
	if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&handle, algorithm, nullptr, 0)))
	{
		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to open algorithm provider : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto objectLength = DWORD{ 0 };
	if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(DWORD), &dummy, 0)))
	{
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to get object length : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto object = std::vector<uint8_t>(objectLength);
	auto hash   = BCRYPT_HASH_HANDLE();
	if (!BCRYPT_SUCCESS(result = BCryptCreateHash(handle, &hash, object.data(), object.size(), nullptr, 0, 0)))
	{
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to create hash object : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto hashLength = DWORD{ 0 };
	if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(DWORD), &dummy, 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to get object length : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	if (!BCRYPT_SUCCESS(result = BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer->GetBase()), buffer->length, 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to hash data : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto hashed = std::vector<uint8_t>(hashLength);
	if (!BCRYPT_SUCCESS(result = BCryptFinishHash(hash, reinterpret_cast<PUCHAR>(hashed.data()), hashed.size(), 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to finish hash : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	return null();

	/*auto signatureLength = DWORD{ 0 };
	if (!BCRYPT_SUCCESS(result = BCryptSignHash(key->ctx, nullptr, reinterpret_cast<PUCHAR>(hashed.data()), hashed.size(), nullptr, 0, &signatureLength, 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to signature length : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	hx::ExitGCFreeZone();
	auto signature = Array<uint8_t>(signatureLength, signatureLength);
	hx::EnterGCFreeZone();

	if (!BCRYPT_SUCCESS(result = BCryptSignHash(key->ctx, &padding, reinterpret_cast<PUCHAR>(hashed.data()), hashed.size(), reinterpret_cast<PUCHAR>(signature->GetBase()), signature->length, &signatureLength, BCRYPT_PAD_PKCS1)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to sign hash : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	BCryptDestroyHash(hash);
	BCryptCloseAlgorithmProvider(handle, 0);

	hx::ExitGCFreeZone();

	return signature;*/
}

bool _hx_ssl_dgst_verify(Array<unsigned char> buffer, Array<unsigned char> sign, Dynamic hpkey, String alg)
{
	auto key       = hpkey.Cast<hx::ssl::windows::Key>();
	auto handle    = BCRYPT_ALG_HANDLE();
	auto result    = 0;
	auto cb        = DWORD{ 0 };
	hx::strbuf buf;
	auto algorithm = alg.wchar_str(&buf);
	auto padding   = BCRYPT_PKCS1_PADDING_INFO{ algorithm };

	hx::EnterGCFreeZone();

	if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&handle, algorithm, nullptr, 0)))
	{
		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to open algorithm provider : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto objectLength = DWORD{ 0 };
	if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(DWORD), &cb, 0)))
	{
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to get object length : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto object = std::vector<uint8_t>(objectLength);
	auto hash   = BCRYPT_HASH_HANDLE();
	if (!BCRYPT_SUCCESS(result = BCryptCreateHash(handle, &hash, object.data(), object.size(), nullptr, 0, 0)))
	{
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to create hash object : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto hashLength = DWORD{ 0 };
	if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(DWORD), &cb, 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to get object length : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	if (!BCRYPT_SUCCESS(result = BCryptHashData(hash, reinterpret_cast<PUCHAR>(buffer->GetBase()), buffer->length, 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to hash data : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	auto output = std::vector<UCHAR>(hashLength);
	if (!BCRYPT_SUCCESS(result = BCryptFinishHash(hash, reinterpret_cast<PUCHAR>(output.data()), output.size(), 0)))
	{
		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(handle, 0);

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to finish hash : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	}

	return null();

	/*auto success = BCRYPT_SUCCESS(BCryptVerifySignature(key->ctx, &padding, output.data(), output.size(), reinterpret_cast<PUCHAR>(sign->GetBase()), sign->length, BCRYPT_PAD_PKCS1));

	hx::ExitGCFreeZone();

	return success;*/
}