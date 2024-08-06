#include <hxcpp.h>
#include <Windows.h>
#include <sstream>
#include <iostream>

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

	Dynamic DecodePrivateRsaKey(const uint8_t* data, const DWORD length)
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
		if (NCryptImportKey(provider, 0, BCRYPT_PRIVATE_KEY_BLOB, nullptr, &key, keyblob.data(), size, 0))
		{
			NCryptFreeObject(provider);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to import key"));
		}

		NCryptFreeObject(provider);

		hx::ExitGCFreeZone();

		return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
	}

	Dynamic DecodeEncryptedPrivateRsaKey(std::vector<uint8_t>& encrypted, std::string& iv64, const wchar_t* algorithm, const int keySize, String pass)
	{
		auto cb     = DWORD{ 0 };
		auto result = DWORD{ 0 };
		auto size   = DWORD{ 0 };

		if (!CryptStringToBinaryA(iv64.c_str(), 0, CRYPT_STRING_HEX, nullptr, &size, nullptr, nullptr))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		auto iv = std::vector<uint8_t>(size);
		if (!CryptStringToBinaryA(iv64.c_str(), 0, CRYPT_STRING_HEX, iv.data(), &size, nullptr, nullptr))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		auto md5 = BCRYPT_ALG_HANDLE();
		if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&md5, BCRYPT_MD5_ALGORITHM, nullptr, BCRYPT_HASH_REUSABLE_FLAG)))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		auto objectLength = DWORD{ 0 };
		if (!BCRYPT_SUCCESS(result = BCryptGetProperty(md5, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(DWORD), &cb, 0)))
		{
			BCryptCloseAlgorithmProvider(md5, 0);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		auto object = std::vector<uint8_t>(objectLength);
		auto hash   = BCRYPT_HASH_HANDLE();
		if (!BCRYPT_SUCCESS(result = BCryptCreateHash(md5, &hash, object.data(), object.size(), nullptr, 0, BCRYPT_HASH_REUSABLE_FLAG)))
		{
			BCryptCloseAlgorithmProvider(md5, 0);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		auto concat = std::vector<uint8_t>();

		hx::strbuf passbuffer;

		int passlength;
		auto password = pass.utf8_str(&passbuffer, true, &passlength);

		while (concat.size() < keySize)
		{
			if (!concat.empty())
			{
				if (!BCRYPT_SUCCESS(result = BCryptHashData(hash, reinterpret_cast<PUCHAR>(concat.data()), concat.size(), 0)))
				{
					BCryptDestroyHash(hash);
					BCryptCloseAlgorithmProvider(md5, 0);

					hx::ExitGCFreeZone();
					hx::Throw(HX_CSTRING("Failure"));
				}
			}

			if (!BCRYPT_SUCCESS(result = BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(password)), passlength, 0)))
			{
				BCryptDestroyHash(hash);
				BCryptCloseAlgorithmProvider(md5, 0);

				hx::ExitGCFreeZone();
				hx::Throw(HX_CSTRING("Failure"));
			}
			if (!BCRYPT_SUCCESS(result = BCryptHashData(hash, reinterpret_cast<PUCHAR>(iv.data()), 8, 0)))
			{
				BCryptDestroyHash(hash);
				BCryptCloseAlgorithmProvider(md5, 0);

				hx::ExitGCFreeZone();
				hx::Throw(HX_CSTRING("Failure"));
			}

			auto hashLength = DWORD{ 0 };
			if (!BCRYPT_SUCCESS(result = BCryptGetProperty(hash, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(DWORD), &cb, 0)))
			{
				BCryptDestroyHash(hash);
				BCryptCloseAlgorithmProvider(md5, 0);

				hx::ExitGCFreeZone();
				hx::Throw(HX_CSTRING("Failure"));
			}

			auto digest = std::vector<uint8_t>(hashLength);
			if (!BCRYPT_SUCCESS(result = BCryptFinishHash(hash, digest.data(), digest.size(), 0)))
			{
				BCryptDestroyHash(hash);
				BCryptCloseAlgorithmProvider(md5, 0);

				hx::ExitGCFreeZone();
				hx::Throw(HX_CSTRING("Failure"));
			}

			concat.insert(concat.end(), digest.begin(), digest.end());
		}

		auto aes = BCRYPT_ALG_HANDLE();
		if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&aes, algorithm, nullptr, 0)))
		{
			BCryptDestroyHash(hash);
			BCryptCloseAlgorithmProvider(md5, 0);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		if (!BCRYPT_SUCCESS(result = BCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&size), sizeof(DWORD), &cb, 0)))
		{
			BCryptDestroyHash(hash);
			BCryptCloseAlgorithmProvider(md5, 0);
			BCryptCloseAlgorithmProvider(aes, 0);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		auto aeskey = std::vector<uint8_t>(size);

		auto blockLength = DWORD{ 0 };
		if (!BCRYPT_SUCCESS(result = BCryptGetProperty(aes, BCRYPT_BLOCK_LENGTH, reinterpret_cast<PUCHAR>(&blockLength), sizeof(DWORD), &cb, 0)))
		{
			BCryptDestroyHash(hash);
			BCryptCloseAlgorithmProvider(md5, 0);
			BCryptCloseAlgorithmProvider(aes, 0);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		if (!BCRYPT_SUCCESS(result = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(BCRYPT_CHAIN_MODE_CBC), wcslen(BCRYPT_CHAIN_MODE_CBC), 0)))
		{
			BCryptDestroyHash(hash);
			BCryptCloseAlgorithmProvider(md5, 0);
			BCryptCloseAlgorithmProvider(aes, 0);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		auto outputkey = BCRYPT_KEY_HANDLE();
		if (!BCRYPT_SUCCESS(result = BCryptGenerateSymmetricKey(aes, &outputkey, aeskey.data(), aeskey.size(), concat.data(), keySize, 0)))
		{
			BCryptDestroyHash(hash);
			BCryptCloseAlgorithmProvider(md5, 0);
			BCryptCloseAlgorithmProvider(aes, 0);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failure"));
		}

		if (!BCRYPT_SUCCESS(result = BCryptDecrypt(outputkey, encrypted.data(), encrypted.size(), nullptr, iv.data(), iv.size(), nullptr, 0, &size, BCRYPT_BLOCK_PADDING)))
		{
			BCryptDestroyHash(hash);
			BCryptCloseAlgorithmProvider(md5, 0);
			BCryptCloseAlgorithmProvider(aes, 0);
			BCryptDestroyKey(outputkey);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to get decrypted key size : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
		}

		auto decrypted = std::vector<uint8_t>(size);
		if (!BCRYPT_SUCCESS(result = BCryptDecrypt(outputkey, encrypted.data(), encrypted.size(), nullptr, iv.data(), iv.size(), decrypted.data(), decrypted.size(), &size, BCRYPT_BLOCK_PADDING)))
		{
			BCryptDestroyHash(hash);
			BCryptCloseAlgorithmProvider(md5, 0);
			BCryptCloseAlgorithmProvider(aes, 0);
			BCryptDestroyKey(outputkey);

			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to decrypt key : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
		}

		BCryptDestroyHash(hash);
		BCryptCloseAlgorithmProvider(md5, 0);
		BCryptCloseAlgorithmProvider(aes, 0);
		BCryptDestroyKey(outputkey);

		return DecodePrivateRsaKey(decrypted.data(), decrypted.size());
	}

	Dynamic DecodeEncryptedPrivateRsaKey(const char* pem, String pass)
	{
		auto stream = std::istringstream(pem);
		auto line = std::string();

		auto buffer = std::string();
		auto blockSize = int{ 0 };
		auto cipher = (const wchar_t*)nullptr;
		auto iv = std::string();

		while (std::getline(stream, line))
		{
			if (line.at(line.length() - 1) == '\r')
			{
				line.pop_back();
			}

			if (line.find("-----BEGIN") != std::string::npos)
			{
				continue;
			}
			if (line.find("-----END") != std::string::npos)
			{
				continue;
			}
			if (line.find("Proc-Type") != std::string::npos)
			{
				continue;
			}
			if (line.find("DEK-Info") != std::string::npos)
			{
				line.erase(0, line.find(':') + sizeof(char));

				if (line.find("AES-256-CBC") != std::string::npos)
				{
					blockSize = 32;
					cipher = BCRYPT_AES_ALGORITHM;
				}
				if (line.find("AES-192-CBC") != std::string::npos)
				{
					blockSize = 24;
					cipher = BCRYPT_AES_ALGORITHM;
				}
				if (line.find("AES-128-CBC") != std::string::npos)
				{
					blockSize = 16;
					cipher = BCRYPT_AES_ALGORITHM;
				}
				if (line.find("DES-EDE3-CBC") != std::string::npos)
				{
					blockSize = 24;
					cipher = BCRYPT_3DES_ALGORITHM;
				}
				if (line.find("DES-CBC") != std::string::npos)
				{
					blockSize = 8;
					cipher = BCRYPT_DES_ALGORITHM;
				}

				if (nullptr == cipher)
				{
					hx::ExitGCFreeZone();
					hx::Throw(HX_CSTRING("Private key contained unsupported encryption cipher"));
				}

				line.erase(0, line.find(',') + sizeof(char));

				iv.assign(line);

				continue;
			}

			buffer.append(line);
		}

		auto derKeyLength = DWORD{ 0 };
		if (!CryptStringToBinaryA(buffer.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &derKeyLength, nullptr, nullptr))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto derKey = std::vector<uint8_t>(derKeyLength);
		if (!CryptStringToBinaryA(buffer.c_str(), 0, CRYPT_STRING_BASE64, derKey.data(), &derKeyLength, nullptr, nullptr))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		return DecodeEncryptedPrivateRsaKey(derKey, iv, cipher, blockSize, pass);
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
			if (hx::IsNull(pass))
			{
				return DecodePrivateRsaKey(derKey.data(), derKeyLength);
			}
			else
			{
				return DecodeEncryptedPrivateRsaKey(string, pass);
			}
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
