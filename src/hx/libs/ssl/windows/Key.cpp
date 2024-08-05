#include <hxcpp.h>
#include <Windows.h>

#include "SSL.h"
#include "Utils.h"

namespace
{
	Dynamic DecodePublicRsaKey(const uint8_t* data, const DWORD length)
	{
		auto size = DWORD{ 0 };
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PUBLIC_KEY_BLOB, data, length, 0, nullptr, nullptr, &size))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to get the size of the RSA public key blob"));
		}

		auto keyblob = std::vector<uint8_t>(size);
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PUBLIC_KEY_BLOB, data, length, 0, nullptr, keyblob.data(), &size))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to decode the key into a RSA public key blob"));
		}

		auto provider = NCRYPT_PROV_HANDLE();
		if (NCryptOpenStorageProvider(&provider, MS_KEY_STORAGE_PROVIDER, 0))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to open Microsoft key storage provider"));
		}

		auto key = NCRYPT_KEY_HANDLE();
		if (NCryptImportKey(provider, 0, BCRYPT_PUBLIC_KEY_BLOB, nullptr, &key, keyblob.data(), size, 0))
		{
			NCryptFreeObject(provider);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to import key"));
		}

		NCryptFreeObject(provider);

		hx::ExitGCFreeZone();

		return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
	}

	Dynamic DecodePrivateRsaKey(const uint8_t* data, const DWORD length, String pass)
	{
		auto size = DWORD{ 0 };
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, data, length, 0, nullptr, nullptr, &size))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to get the size of the RSA private key blob"));
		}

		auto keyblob = std::vector<uint8_t>(size);
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, data, length, 0, nullptr, keyblob.data(), &size))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to decode the key into a RSA private key blob"));
		}

		auto provider = NCRYPT_PROV_HANDLE();
		if (NCryptOpenStorageProvider(&provider, MS_KEY_STORAGE_PROVIDER, 0))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to open Microsoft key storage provider"));
		}

		auto key = NCRYPT_KEY_HANDLE();
		if (hx::IsNull(pass))
		{
			if (NCryptImportKey(provider, 0, BCRYPT_PRIVATE_KEY_BLOB, nullptr, &key, keyblob.data(), size, 0))
			{
				NCryptFreeObject(provider);

				hx::ExitGCFreeZone();
				hx::Throw(HX_CSTRING("Failed to import key"));
			}
		}
		else
		{
			hx::strbuf passbuf;

			auto password = pass.wchar_str(&passbuf);
			auto buffer = NCryptBuffer();
			buffer.BufferType = NCRYPTBUFFER_PKCS_SECRET;
			buffer.pvBuffer   = const_cast<PWSTR>(password);
			buffer.cbBuffer   = (1UL + wcslen(password)) * sizeof(WCHAR);

			auto desc = NCryptBufferDesc();
			desc.cBuffers  = 1;
			desc.pBuffers  = &buffer;
			desc.ulVersion = NCRYPTBUFFER_VERSION;

			if (NCryptImportKey(provider, 0, BCRYPT_PRIVATE_KEY_BLOB, &desc, &key, keyblob.data(), size, 0))
			{
				NCryptFreeObject(provider);

				hx::ExitGCFreeZone();
				hx::Throw(HX_CSTRING("Failed to import key"));
			}
		}

		NCryptFreeObject(provider);

		hx::ExitGCFreeZone();

		return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
	}

	Dynamic DecodePublicPkcs8Key(const uint8_t* data, const DWORD length)
	{
		auto size = DWORD{ 0 };
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_PUBLIC_KEY_INFO, data, length, CRYPT_DECODE_NOCOPY_FLAG, nullptr, nullptr, &size))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to decode object size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto keyInfoBuffer = std::vector<uint8_t>(size);
		auto keyInfo       = reinterpret_cast<PCERT_PUBLIC_KEY_INFO>(keyInfoBuffer.data());
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_PUBLIC_KEY_INFO, data, length, CRYPT_DECODE_NOCOPY_FLAG, nullptr, keyInfoBuffer.data(), &size))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to decode object size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		// There must be an easier way than round tripping through BCrypt to inspect the algorithm type...

		auto key = BCRYPT_KEY_HANDLE();
		if (!CryptImportPublicKeyInfoEx2(X509_ASN_ENCODING, keyInfo, 0, nullptr, &key))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to import public key : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		if (!BCRYPT_SUCCESS(BCryptGetProperty(key, BCRYPT_ALGORITHM_NAME, nullptr, 0, &size, 0)))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to get export key size"));
		}

		auto name = std::vector<uint8_t>(size);
		if (!BCRYPT_SUCCESS(BCryptGetProperty(key, BCRYPT_ALGORITHM_NAME, name.data(), name.size(), &size, 0)))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to get export key size"));
		}

		auto ident = (const wchar_t*)nullptr;
		if (0 == wcscmp(reinterpret_cast<wchar_t*>(name.data()), BCRYPT_RSA_ALGORITHM))
		{
			ident = BCRYPT_RSAPUBLIC_BLOB;
		}
		if (0 == wcscmp(reinterpret_cast<wchar_t*>(name.data()), BCRYPT_DSA_ALGORITHM))
		{
			ident = BCRYPT_DSA_PUBLIC_BLOB;
		}
		if (0 == wcsncmp(reinterpret_cast<wchar_t*>(name.data()), L"EC", 2))
		{
			ident = BCRYPT_ECCPUBLIC_BLOB;
		}

		if (nullptr == ident)
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Unexpected key type"));
		}

		if (!BCRYPT_SUCCESS(BCryptExportKey(key, nullptr, ident, nullptr, 0, &size, 0)))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to get export key size"));
		}

		auto eccKey = std::vector<uint8_t>(size);
		if (!BCRYPT_SUCCESS(BCryptExportKey(key, nullptr, ident, eccKey.data(), size, &size, 0)))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to export key"));
		}

		auto provider = NCRYPT_PROV_HANDLE();
		if (NCryptOpenStorageProvider(&provider, MS_KEY_STORAGE_PROVIDER, 0))
		{
			BCryptDestroyKey(key);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to open Microsoft key storage provider"));
		}

		auto nkey = NCRYPT_KEY_HANDLE();
		if (NCryptImportKey(provider, 0, ident, nullptr, &nkey, eccKey.data(), size, 0))
		{
			NCryptFreeObject(provider);
			BCryptDestroyKey(key);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to import key"));
		}

		NCryptFreeObject(provider);
		BCryptDestroyKey(key);

		hx::ExitGCFreeZone();

		return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(nkey));
	}

	Dynamic DecodePrivatePkcs8Key(uint8_t* data, const DWORD length)
	{
		auto provider = NCRYPT_PROV_HANDLE();
		if (NCryptOpenStorageProvider(&provider, MS_KEY_STORAGE_PROVIDER, 0))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to open Microsoft key storage provider"));
		}

		auto key = NCRYPT_KEY_HANDLE();
		if (NCryptImportKey(provider, 0, NCRYPT_PKCS8_PRIVATE_KEY_BLOB, nullptr, &key, data, length, 0))
		{
			NCryptFreeObject(provider);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to import private PKCS8 key"));
		}

		NCryptFreeObject(provider);

		hx::ExitGCFreeZone();

		return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
	}

	Dynamic DecodeEncryptedPrivatePkcs8Key(uint8_t* data, const DWORD length, String pass)
	{
		if (hx::IsNull(pass))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Encrypted key was found but password was null"));
		}

		hx::strbuf passbuf;

		auto password = pass.wchar_str(&passbuf);
		auto buffer   = NCryptBuffer();
		buffer.BufferType = NCRYPTBUFFER_PKCS_SECRET;
		buffer.pvBuffer   = const_cast<PWSTR>(password);
		buffer.cbBuffer   = (1UL + wcslen(password)) * sizeof(WCHAR);

		auto desc = NCryptBufferDesc();
		desc.cBuffers  = 1;
		desc.pBuffers  = &buffer;
		desc.ulVersion = NCRYPTBUFFER_VERSION;

		auto provider = NCRYPT_PROV_HANDLE();
		if (NCryptOpenStorageProvider(&provider, MS_KEY_STORAGE_PROVIDER, 0))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to open Microsoft key storage provider"));
		}

		auto key = NCRYPT_KEY_HANDLE();
		if (NCryptImportKey(provider, 0, NCRYPT_PKCS8_PRIVATE_KEY_BLOB, &desc, &key, data, length, 0))
		{
			NCryptFreeObject(provider);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to import private PKCS8 key"));
		}

		NCryptFreeObject(provider);

		hx::ExitGCFreeZone();

		return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
	}
}

Dynamic _hx_ssl_key_from_der(Array<unsigned char> buf, bool pub)
{
	return null();

	/*if (pub)
	{
		hx::EnterGCFreeZone();

		return DecodePublicKey(reinterpret_cast<uint8_t*>(buf->GetBase()), buf->length);
	}
	else
	{
		hx::EnterGCFreeZone();

		return DecodePrivateKey(null(), reinterpret_cast<uint8_t*>(buf->GetBase()), buf->length);
	}*/
}

Dynamic _hx_ssl_key_from_pem(String pem, bool pub, String pass)
{
	hx::strbuf buffer;

	auto result = 0;
	auto string = pem.utf8_str(&buffer);
	auto cb     = DWORD{ 0 };

	hx::EnterGCFreeZone();

	auto derKeyLength = DWORD{ 0 };
	if (!CryptStringToBinaryA(string, 0, CRYPT_STRING_BASE64HEADER, nullptr, &derKeyLength, nullptr, nullptr))
	{
		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto skipped = DWORD{ 0 };
	auto derKey  = std::vector<uint8_t>(derKeyLength);
	if (!CryptStringToBinaryA(string, 0, CRYPT_STRING_BASE64HEADER, derKey.data(), &derKeyLength, &skipped, nullptr))
	{
		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto header = (const char*)nullptr;
	if (pub)
	{
		header = "-----BEGIN RSA PUBLIC KEY-----";
		if (0 == strncmp(string + skipped, header, strlen(header)))
		{
			return DecodePublicRsaKey(derKey.data(), derKeyLength);
		}

		header = "-----BEGIN PUBLIC KEY-----";
		if (0 == strncmp(string + skipped, header, strlen(header)))
		{
			return DecodePublicPkcs8Key(derKey.data(), derKeyLength);
		}

		hx::ExitGCFreeZone();
		hx::Throw(HX_CSTRING("Unsupported public key type in PEM"));
	}
	else
	{
		header = "-----BEGIN RSA PRIVATE KEY-----";
		if (0 == strncmp(string + skipped, header, strlen(header)))
		{
			return DecodePrivateRsaKey(derKey.data(), derKeyLength, pass);
		}

		header = "-----BEGIN PRIVATE KEY-----";
		if (0 == strncmp(string + skipped, header, strlen(header)))
		{
			return DecodePrivatePkcs8Key(derKey.data(), derKeyLength);
		}

		header = "-----BEGIN ENCRYPTED PRIVATE KEY-----";
		if (0 == strncmp(string + skipped, header, strlen(header)))
		{
			return DecodeEncryptedPrivatePkcs8Key(derKey.data(), derKeyLength, pass);
		}

		return null();
	}
}
