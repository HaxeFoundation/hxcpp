#include <hxcpp.h>
#include <Windows.h>

#include "SSL.h"
#include "Utils.h"

namespace
{
	//struct CryptCNGMap
	//{
	//	struct ENCODE_DECODE_PARA {
	//		PFN_CRYPT_ALLOC         pfnAlloc;           // OPTIONAL
	//		PFN_CRYPT_FREE          pfnFree;            // OPTIONAL
	//	};

	//	struct SECRET_APPEND
	//	{
	//		ULONG IterationCount;
	//		ULONG cb;
	//		UCHAR buf[];
	//	};

	//	ULONG version;

	//	NTSTATUS(WINAPI* InitHash)(
	//		_Out_ BCRYPT_ALG_HANDLE* phAlgorithm,
	//		_Out_ ULONG* pcbObjectLength,
	//		_In_ ULONG dwFlags);

	//	NTSTATUS(WINAPI* InitEncrypt)(
	//		_Out_ BCRYPT_ALG_HANDLE* phAlgorithm,
	//		_Out_ ULONG* pcbObjectLength,
	//		_In_ BYTE* pbParams,
	//		_In_ ULONG cbParams
	//		);

	//	NTSTATUS(WINAPI* InitDecrypt)(
	//		_Out_ BCRYPT_ALG_HANDLE* phAlgorithm,
	//		_Out_ ULONG* pcbObjectLength,
	//		_In_ BYTE* pbParams,
	//		_In_ ULONG cbParams
	//		);

	//	NTSTATUS(WINAPI* InitEncryptKey)(
	//		_In_ BCRYPT_ALG_HANDLE hAlgorithm,
	//		_Out_ BCRYPT_KEY_HANDLE* phKey,
	//		_Out_ BYTE* pbKeyObject,
	//		_In_ ULONG cbKeyObject,
	//		_In_ const BYTE* pbSecret,
	//		_In_ ULONG cbSecret,
	//		_Out_ BYTE* pbParams,
	//		_Out_ ULONG cbParams
	//		);

	//	NTSTATUS(WINAPI* InitDecryptKey)(
	//		_In_ BCRYPT_ALG_HANDLE hAlgorithm,
	//		_Out_ BCRYPT_KEY_HANDLE* phKey,
	//		_Out_ BYTE* pbKeyObject,
	//		_In_ ULONG cbKeyObject,
	//		_In_ const BYTE* pbSecret,
	//		_In_ ULONG cbSecret,
	//		_Out_ BYTE* pbParams,
	//		_Out_ ULONG cbParams
	//		);

	//	NTSTATUS(WINAPI* PasswordDeriveKey)(
	//		_In_ CryptCNGMap* map,
	//		_In_ UCHAR SECRET_PREPEND,
	//		_In_ PCWSTR pszPassword,
	//		_In_ ULONG cbPassword, // wcslen(pszPassword)*sizeof(WCHAR)
	//		_In_opt_ SECRET_APPEND* p,
	//		_In_opt_ ULONG cb,
	//		_Out_ PBYTE pbOutput,
	//		_Out_ ULONG cbOutput
	//		);

	//	NTSTATUS(WINAPI* ParamsEncode)(
	//		_In_ const BYTE* pb,
	//		_In_ ULONG cb,
	//		_In_ const ENCODE_DECODE_PARA* pEncodePara,
	//		_Out_ BYTE** ppbEncoded,
	//		_Out_ ULONG* pcbEncoded
	//		);

	//	NTSTATUS(WINAPI* ParamsDecode)(
	//		_In_ const BYTE* pbEncoded,
	//		_In_ ULONG cbEncoded,
	//		_In_ const ENCODE_DECODE_PARA* pDecodePara,
	//		_Out_ BYTE** ppb,
	//		_Out_ ULONG* pcb
	//		);
	//};

	//void* WINAPI PkiAlloc(_In_ size_t cbSize)
	//{
	//	return LocalAlloc(LMEM_FIXED, cbSize);
	//}

	//void WINAPI PkiFree(void* pv)
	//{
	//	LocalFree(pv);
	//}

	//HRESULT DecryptPrivateKey(PCRYPT_ENCRYPTED_PRIVATE_KEY_INFO privateKey, String password)
	//{
	//	using GetPKCS12Map = CryptCNGMap * (WINAPI*)(void);

	//	auto functionSet = HCRYPTOIDFUNCSET();
	//	if (nullptr == (functionSet = CryptInitOIDFunctionSet("CryptCNGPKCS12GetMap", 0)))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to find the \"CryptCNGPKCS12GetMap\" instruction set"));
	//	}

	//	auto pvFuncAddr = GetPKCS12Map{ nullptr };
	//	auto hFuncAddr  = HCRYPTOIDFUNCADDR();
	//	auto result     = 0;

	//	if (!CryptGetOIDFunctionAddress(
	//		functionSet,
	//		X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
	//		privateKey->EncryptionAlgorithm.pszObjId,
	//		CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
	//		reinterpret_cast<void**>(&pvFuncAddr),
	//		&hFuncAddr))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to decode encryption algorithm parameters"));
	//	}

	//	auto map = pvFuncAddr();
	//	auto cdp = CryptCNGMap::ENCODE_DECODE_PARA{ PkiAlloc, PkiFree };

	//	if (FAILED(result = map->ParamsDecode(
	//		privateKey->EncryptionAlgorithm.Parameters.pbData,
	//		privateKey->EncryptionAlgorithm.Parameters.cbData,
	//		&cdp,
	//		&privateKey->EncryptionAlgorithm.Parameters.pbData,
	//		&privateKey->EncryptionAlgorithm.Parameters.cbData)))
	//	{
	//		CryptFreeOIDFunctionAddress(hFuncAddr, 0);

	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to decode encryption algorithm parameters"));
	//	}

	//	auto cb         = ULONG{ 0 };
	//	auto hAlgorithm = BCRYPT_ALG_HANDLE();

	//	if (FAILED(result = map->InitDecrypt(
	//		&hAlgorithm,
	//		&cb,
	//		privateKey->EncryptionAlgorithm.Parameters.pbData,
	//		privateKey->EncryptionAlgorithm.Parameters.cbData)))
	//	{
	//		CryptFreeOIDFunctionAddress(hFuncAddr, 0);

	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to init decryption"));
	//	}

	//	struct CRYPT_PKCS12_PBES2_PARAMS
	//	{
	//		/*00*/BOOL bUTF8;
	//		/*04*/CHAR pszObjId[0x20];//szOID_PKCS_5_PBKDF2 "1.2.840.113549.1.5.12"
	//		/*24*/ULONG cbSalt;
	//		/*28*/UCHAR pbSalt[0x20];
	//		/*48*/ULONG cIterations;
	//		/*4c*/ULONG pad;
	//		/*50*/CHAR pszHMACAlgorithm[0x20];//PKCS12_PBKDF2_ID_HMAC_SHAxxx -> BCRYPT_HMAC_SHAxxx_ALG_HANDLE
	//		/*70*/CHAR pszKeyAlgorithm[0x20];//szOID_NIST_AESxxx_CBC
	//		/*90*/ULONG cbIV;
	//		/*94*/UCHAR pbIV[0x20];
	//		/*b4*/
	//	};

	//	auto hKey  = BCRYPT_KEY_HANDLE();
	//	auto pbIV  = PBYTE{ 0 };
	//	auto cbIV  = ULONG{ 0 };
	//	auto bUTF8 = false;
	//	auto py    = reinterpret_cast<CRYPT_PKCS12_PBES2_PARAMS*>(privateKey->EncryptionAlgorithm.Parameters.pbData);

	//	if (sizeof(CRYPT_PKCS12_PBES2_PARAMS) == privateKey->EncryptionAlgorithm.Parameters.cbData && !strcmp(szOID_PKCS_5_PBKDF2, py->pszObjId))
	//	{
	//		pbIV = py->pbIV;
	//		cbIV = py->cbIV;
	//		if (cbIV > sizeof(py->pbIV))
	//		{
	//			CryptFreeOIDFunctionAddress(hFuncAddr, 0);

	//			hx::ExitGCFreeZone();
	//			hx::Throw(HX_CSTRING("Bad data"));
	//		}
	//	}

	//	if (py->bUTF8)
	//	{
	//		hx::strbuf buffer;
	//		auto str    = const_cast<char*>(password.utf8_str(&buffer));

	//		if (FAILED(result = map->InitDecryptKey(hAlgorithm, &hKey,
	//			(PBYTE)alloca(cb), cb, reinterpret_cast<PBYTE>(str), strlen(str),
	//			privateKey->EncryptionAlgorithm.Parameters.pbData,
	//			privateKey->EncryptionAlgorithm.Parameters.cbData
	//		)))
	//		{
	//			CryptFreeOIDFunctionAddress(hFuncAddr, 0);

	//			hx::ExitGCFreeZone();
	//			hx::Throw(HX_CSTRING("Failed to init decryption key"));
	//		}
	//	}
	//	else
	//	{
	//		hx::strbuf buffer;
	//		auto str    = const_cast<wchar_t*>(password.wchar_str(&buffer));

	//		if (FAILED(result = map->InitDecryptKey(hAlgorithm, &hKey,
	//			(PBYTE)alloca(cb), cb, reinterpret_cast<PBYTE>(str), wcslen(str),
	//			privateKey->EncryptionAlgorithm.Parameters.pbData,
	//			privateKey->EncryptionAlgorithm.Parameters.cbData
	//		)))
	//		{
	//			CryptFreeOIDFunctionAddress(hFuncAddr, 0);

	//			hx::ExitGCFreeZone();
	//			hx::Throw(HX_CSTRING("Failed to init decryption key"));
	//		}
	//	}

	//	BCryptCloseAlgorithmProvider(hAlgorithm, 0);

	//	if (FAILED(result = BCryptDecrypt(hKey,
	//		privateKey->EncryptedPrivateKey.pbData,
	//		privateKey->EncryptedPrivateKey.cbData,
	//		0, pbIV, cbIV,
	//		privateKey->EncryptedPrivateKey.pbData,
	//		privateKey->EncryptedPrivateKey.cbData,
	//		&privateKey->EncryptedPrivateKey.cbData, 0)))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to decrypt key"));
	//	}

	//	BCryptDestroyKey(hKey);
	//	CryptFreeOIDFunctionAddress(hFuncAddr, 0);

	//	return 0;
	//}

	//Dynamic DecodePublicKey(uint8_t* derKey, int derKeySize)
	//{
	//	auto cb = DWORD{ 0 };

	//	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_PUBLIC_KEY_INFO, derKey, derKeySize, CRYPT_DECODE_NOCOPY_FLAG, nullptr, nullptr, &cb))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to decode object size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//	}

	//	auto keyInfoBuffer = std::vector<uint8_t>(cb);
	//	auto keyInfo       = reinterpret_cast<PCERT_PUBLIC_KEY_INFO>(keyInfoBuffer.data());
	//	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_PUBLIC_KEY_INFO, derKey, derKeySize, CRYPT_DECODE_NOCOPY_FLAG, nullptr, keyInfoBuffer.data(), &cb))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to decode object size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//	}

	//	auto key = BCRYPT_KEY_HANDLE();
	//	if (!CryptImportPublicKeyInfoEx2(X509_ASN_ENCODING, keyInfo, 0, nullptr, &key))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to import public key : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//	}

	//	hx::ExitGCFreeZone();

	//	return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
	//}

	//Dynamic DecodeUnencryptedPrivateKey(uint8_t* derKey, int derKeySize)
	//{
	//	auto result = 0;
	//	auto cb     = DWORD{ 0 };

	//	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, derKey, derKeySize, CRYPT_DECODE_NOCOPY_FLAG, nullptr, nullptr, &cb))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to decode private key size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//	}

	//	auto keyInfoBuffer = std::vector<uint8_t>(cb);
	//	auto keyInfo       = reinterpret_cast<PCRYPT_PRIVATE_KEY_INFO>(keyInfoBuffer.data());
	//	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, derKey, derKeySize, CRYPT_DECODE_NOCOPY_FLAG, nullptr, keyInfoBuffer.data(), &cb))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to decode private key: ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//	}

	//	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, keyInfo->PrivateKey.pbData, keyInfo->PrivateKey.cbData, 0, nullptr, nullptr, &cb))
	//	{
	//		hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//	}

	//	auto rsaKeyBuffer = std::vector<uint8_t>(cb);
	//	auto rsaKey = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(rsaKeyBuffer.data());
	//	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, keyInfo->PrivateKey.pbData, keyInfo->PrivateKey.cbData, 0, nullptr, rsaKeyBuffer.data(), &cb))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//	}

	//	auto algorithm = BCRYPT_ALG_HANDLE();
	//	if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_RSA_ALGORITHM, nullptr, 0)))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to open RSA provider : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	//	}

	//	auto key = BCRYPT_KEY_HANDLE();
	//	if (!BCRYPT_SUCCESS(result = BCryptImportKeyPair(algorithm, nullptr, BCRYPT_RSAPRIVATE_BLOB, &key, reinterpret_cast<PUCHAR>(rsaKeyBuffer.data()), rsaKeyBuffer.size(), BCRYPT_NO_KEY_VALIDATION)))
	//	{
	//		hx::ExitGCFreeZone();
	//		hx::Throw(HX_CSTRING("Failed to import private key : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
	//	}

	//	hx::ExitGCFreeZone();

	//	return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
	//}

	//Dynamic DecodePrivateKey(String pass, uint8_t* derKey, int derKeySize)
	//{
	//	auto cb     = DWORD{ 0 };
	//	auto result = 0;

	//	if (hx::IsNotNull(pass))
	//	{
	//		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_ENCRYPTED_PRIVATE_KEY_INFO, derKey, derKeySize, 0, nullptr, nullptr, &cb))
	//		{
	//			hx::ExitGCFreeZone();
	//			hx::Throw(HX_CSTRING("Failed to get encrypted key size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//		}

	//		auto encryptionInfoBuffer = std::vector<uint8_t>(cb);
	//		auto encryptionInfo       = reinterpret_cast<PCRYPT_ENCRYPTED_PRIVATE_KEY_INFO>(encryptionInfoBuffer.data());
	//		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_ENCRYPTED_PRIVATE_KEY_INFO, derKey, derKeySize, 0, nullptr, encryptionInfoBuffer.data(), &cb))
	//		{
	//			hx::ExitGCFreeZone();
	//			hx::Throw(HX_CSTRING("Failed to get encrypted key info : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//		}

	//		if (FAILED(DecryptPrivateKey(encryptionInfo, pass)))
	//		{
	//			hx::ExitGCFreeZone();
	//			hx::Throw(HX_CSTRING("Failed to get decrypt key : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	//		}

	//		return DecodeUnencryptedPrivateKey(encryptionInfo->EncryptedPrivateKey.pbData, encryptionInfo->EncryptedPrivateKey.cbData);
	//	}
	//	else
	//	{
	//		return DecodeUnencryptedPrivateKey(derKey, derKeySize);
	//	}
	//}

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
			return null();
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
