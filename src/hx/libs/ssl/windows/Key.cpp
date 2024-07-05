#include <hxcpp.h>
#include "SSL.h"
#include "Utils.h"
#include <Windows.h>

namespace
{
	struct CryptCNGMap
	{
		struct ENCODE_DECODE_PARA {
			PFN_CRYPT_ALLOC         pfnAlloc;           // OPTIONAL
			PFN_CRYPT_FREE          pfnFree;            // OPTIONAL
		};

		struct SECRET_APPEND
		{
			ULONG IterationCount;
			ULONG cb;
			UCHAR buf[];
		};

		ULONG version;

		NTSTATUS(WINAPI* InitHash)(
			_Out_ BCRYPT_ALG_HANDLE* phAlgorithm,
			_Out_ ULONG* pcbObjectLength,
			_In_ ULONG dwFlags);

		NTSTATUS(WINAPI* InitEncrypt)(
			_Out_ BCRYPT_ALG_HANDLE* phAlgorithm,
			_Out_ ULONG* pcbObjectLength,
			_In_ BYTE* pbParams,
			_In_ ULONG cbParams
			);

		NTSTATUS(WINAPI* InitDecrypt)(
			_Out_ BCRYPT_ALG_HANDLE* phAlgorithm,
			_Out_ ULONG* pcbObjectLength,
			_In_ BYTE* pbParams,
			_In_ ULONG cbParams
			);

		NTSTATUS(WINAPI* InitEncryptKey)(
			_In_ BCRYPT_ALG_HANDLE hAlgorithm,
			_Out_ BCRYPT_KEY_HANDLE* phKey,
			_Out_ BYTE* pbKeyObject,
			_In_ ULONG cbKeyObject,
			_In_ const BYTE* pbSecret,
			_In_ ULONG cbSecret,
			_Out_ BYTE* pbParams,
			_Out_ ULONG cbParams
			);

		NTSTATUS(WINAPI* InitDecryptKey)(
			_In_ BCRYPT_ALG_HANDLE hAlgorithm,
			_Out_ BCRYPT_KEY_HANDLE* phKey,
			_Out_ BYTE* pbKeyObject,
			_In_ ULONG cbKeyObject,
			_In_ const BYTE* pbSecret,
			_In_ ULONG cbSecret,
			_Out_ BYTE* pbParams,
			_Out_ ULONG cbParams
			);

		NTSTATUS(WINAPI* PasswordDeriveKey)(
			_In_ CryptCNGMap* map,
			_In_ UCHAR SECRET_PREPEND,
			_In_ PCWSTR pszPassword,
			_In_ ULONG cbPassword, // wcslen(pszPassword)*sizeof(WCHAR)
			_In_opt_ SECRET_APPEND* p,
			_In_opt_ ULONG cb,
			_Out_ PBYTE pbOutput,
			_Out_ ULONG cbOutput
			);

		NTSTATUS(WINAPI* ParamsEncode)(
			_In_ const BYTE* pb,
			_In_ ULONG cb,
			_In_ const ENCODE_DECODE_PARA* pEncodePara,
			_Out_ BYTE** ppbEncoded,
			_Out_ ULONG* pcbEncoded
			);

		NTSTATUS(WINAPI* ParamsDecode)(
			_In_ const BYTE* pbEncoded,
			_In_ ULONG cbEncoded,
			_In_ const ENCODE_DECODE_PARA* pDecodePara,
			_Out_ BYTE** ppb,
			_Out_ ULONG* pcb
			);
	};

	void* WINAPI PkiAlloc(_In_ size_t cbSize)
	{
		return LocalAlloc(LMEM_FIXED, cbSize);
	}

	void WINAPI PkiFree(void* pv)
	{
		LocalFree(pv);
	}

	HRESULT DecryptPrivateKey(PCRYPT_ENCRYPTED_PRIVATE_KEY_INFO pepki, String password)
	{
		using GetPKCS12Map = CryptCNGMap * (WINAPI*)(void);

		auto hFuncSet = HCRYPTOIDFUNCSET();
		if (nullptr == (hFuncSet = CryptInitOIDFunctionSet("CryptCNGPKCS12GetMap", 0)))
		{
			hx::Throw(HX_CSTRING("Failed to find the \"CryptCNGPKCS12GetMap\" instruction set"));
		}

		auto pvFuncAddr = GetPKCS12Map{ nullptr };
		auto hFuncAddr = HCRYPTOIDFUNCADDR();
		auto result = 0;

		if (!CryptGetOIDFunctionAddress(
			hFuncSet,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			pepki->EncryptionAlgorithm.pszObjId,
			CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
			reinterpret_cast<void**>(&pvFuncAddr),
			&hFuncAddr))
		{
			hx::Throw(HX_CSTRING("Failed to decode encryption algorithm parameters : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto map = pvFuncAddr();
		auto cdp = CryptCNGMap::ENCODE_DECODE_PARA{ PkiAlloc, PkiFree };

		if (FAILED(result = map->ParamsDecode(
			pepki->EncryptionAlgorithm.Parameters.pbData,
			pepki->EncryptionAlgorithm.Parameters.cbData,
			&cdp,
			&pepki->EncryptionAlgorithm.Parameters.pbData,
			&pepki->EncryptionAlgorithm.Parameters.cbData)))
		{
			CryptFreeOIDFunctionAddress(hFuncAddr, 0);

			hx::Throw(HX_CSTRING("Failed to decode encryption algorithm parameters : "));
		}

		auto cb = ULONG{ 0 };
		auto hAlgorithm = BCRYPT_ALG_HANDLE();

		if (FAILED(result = map->InitDecrypt(
			&hAlgorithm,
			&cb,
			pepki->EncryptionAlgorithm.Parameters.pbData,
			pepki->EncryptionAlgorithm.Parameters.cbData)))
		{
			CryptFreeOIDFunctionAddress(hFuncAddr, 0);

			hx::Throw(HX_CSTRING("Failed to init decryption"));
		}

		struct CRYPT_PKCS12_PBES2_PARAMS
		{
			/*00*/BOOL bUTF8;
			/*04*/CHAR pszObjId[0x20];//szOID_PKCS_5_PBKDF2 "1.2.840.113549.1.5.12"
			/*24*/ULONG cbSalt;
			/*28*/UCHAR pbSalt[0x20];
			/*48*/ULONG cIterations;
			/*4c*/ULONG pad;
			/*50*/CHAR pszHMACAlgorithm[0x20];//PKCS12_PBKDF2_ID_HMAC_SHAxxx -> BCRYPT_HMAC_SHAxxx_ALG_HANDLE
			/*70*/CHAR pszKeyAlgorithm[0x20];//szOID_NIST_AESxxx_CBC
			/*90*/ULONG cbIV;
			/*94*/UCHAR pbIV[0x20];
			/*b4*/
		};

		auto hKey = BCRYPT_KEY_HANDLE();
		auto pbIV = PBYTE{ 0 };
		auto cbIV = ULONG{ 0 };
		auto bUTF8 = false;
		auto py = reinterpret_cast<CRYPT_PKCS12_PBES2_PARAMS*>(pepki->EncryptionAlgorithm.Parameters.pbData);

		if (sizeof(CRYPT_PKCS12_PBES2_PARAMS) == pepki->EncryptionAlgorithm.Parameters.cbData && !strcmp(szOID_PKCS_5_PBKDF2, py->pszObjId))
		{
			pbIV = py->pbIV;
			cbIV = py->cbIV;
			if (cbIV > sizeof(py->pbIV))
			{
				CryptFreeOIDFunctionAddress(hFuncAddr, 0);

				hx::Throw(HX_CSTRING("Bad data"));
			}
		}

		if (py->bUTF8)
		{
			auto str = const_cast<char*>(password.utf8_str());

			if (FAILED(result = map->InitDecryptKey(hAlgorithm, &hKey,
				(PBYTE)alloca(cb), cb, reinterpret_cast<PBYTE>(str), strlen(str),
				pepki->EncryptionAlgorithm.Parameters.pbData,
				pepki->EncryptionAlgorithm.Parameters.cbData
			)))
			{
				CryptFreeOIDFunctionAddress(hFuncAddr, 0);

				hx::Throw(HX_CSTRING("Failed to init decryption key"));
			}
		}
		else
		{
			auto str = const_cast<wchar_t*>(password.wchar_str());

			if (FAILED(result = map->InitDecryptKey(hAlgorithm, &hKey,
				(PBYTE)alloca(cb), cb, reinterpret_cast<PBYTE>(str), wcslen(str),
				pepki->EncryptionAlgorithm.Parameters.pbData,
				pepki->EncryptionAlgorithm.Parameters.cbData
			)))
			{
				CryptFreeOIDFunctionAddress(hFuncAddr, 0);

				hx::Throw(HX_CSTRING("Failed to init decryption key"));
			}
		}

		BCryptCloseAlgorithmProvider(hAlgorithm, 0);

		if (FAILED(result = BCryptDecrypt(hKey,
			pepki->EncryptedPrivateKey.pbData,
			pepki->EncryptedPrivateKey.cbData,
			0, pbIV, cbIV,
			pepki->EncryptedPrivateKey.pbData,
			pepki->EncryptedPrivateKey.cbData,
			&pepki->EncryptedPrivateKey.cbData, 0)))
		{
			hx::Throw(HX_CSTRING("Failed to decrypt key"));
		}

		BCryptDestroyKey(hKey);
		CryptFreeOIDFunctionAddress(hFuncAddr, 0);

		return 0;
	}
}

Dynamic _hx_ssl_key_from_der(Array<unsigned char> buf, bool pub)
{
	return null();
}

Dynamic _hx_ssl_key_from_pem(String data, bool pub, String pass)
{
	if (pub)
	{
		auto result = 0;
		auto string = data.utf8_str();
		auto cb = DWORD{ 0 };

		auto derKeyLength = DWORD{ 0 };
		if (!CryptStringToBinaryA(string, 0, CRYPT_STRING_BASE64HEADER, nullptr, &derKeyLength, nullptr, nullptr))
		{
			hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto derKey = std::vector<uint8_t>(derKeyLength);
		if (!CryptStringToBinaryA(string, 0, CRYPT_STRING_BASE64HEADER, derKey.data(), &derKeyLength, nullptr, nullptr))
		{
			hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_CERT_TO_BE_SIGNED, derKey.data(), derKey.size(), CRYPT_DECODE_NOCOPY_FLAG, nullptr, nullptr, &cb))
		{
			hx::Throw(HX_CSTRING("Failed to decode object size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto keyInfoBuffer = std::vector<uint8_t>(cb);
		auto keyInfo = reinterpret_cast<CERT_INFO*>(keyInfoBuffer.data());
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_CERT_TO_BE_SIGNED, derKey.data(), derKey.size(), CRYPT_DECODE_NOCOPY_FLAG, nullptr, keyInfoBuffer.data(), &cb))
		{
			hx::Throw(HX_CSTRING("Failed to decode object size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PUBLIC_KEY_BLOB, keyInfo->SubjectPublicKeyInfo.PublicKey.pbData, keyInfo->SubjectPublicKeyInfo.PublicKey.cbData, 0, nullptr, nullptr, &cb))
		{
			hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto rsaKeyBuffer = std::vector<uint8_t>(cb);
		auto rsaKey = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(rsaKeyBuffer.data());
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PUBLIC_KEY_BLOB, keyInfo->SubjectPublicKeyInfo.PublicKey.pbData, keyInfo->SubjectPublicKeyInfo.PublicKey.cbData, 0, nullptr, rsaKeyBuffer.data(), &cb))
		{
			hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto algorithm = BCRYPT_ALG_HANDLE();
		if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_RSA_ALGORITHM, nullptr, 0)))
		{
			hx::Throw(HX_CSTRING("Failed to open RSA provider"));
		}

		auto key = BCRYPT_KEY_HANDLE();
		if (!BCRYPT_SUCCESS(result = BCryptImportKeyPair(algorithm, nullptr, BCRYPT_RSAPUBLIC_BLOB, &key, reinterpret_cast<PUCHAR>(rsaKeyBuffer.data()), rsaKeyBuffer.size(), BCRYPT_NO_KEY_VALIDATION)))
		{
			hx::Throw(HX_CSTRING("Failed to import private key"));
		}

		return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
	}
	else
	{
		auto result = 0;
		auto string = data.utf8_str();
		auto cb = DWORD{ 0 };

		auto derKeyLength = DWORD{ 0 };
		if (!CryptStringToBinaryA(string, 0, CRYPT_STRING_BASE64HEADER, nullptr, &derKeyLength, nullptr, nullptr))
		{
			hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto derKey = std::vector<uint8_t>(derKeyLength);
		if (!CryptStringToBinaryA(string, 0, CRYPT_STRING_BASE64HEADER, derKey.data(), &derKeyLength, nullptr, nullptr))
		{
			hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		if (hx::IsNotNull(pass))
		{
			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_ENCRYPTED_PRIVATE_KEY_INFO, derKey.data(), derKey.size(), 0, nullptr, nullptr, &cb))
			{
				hx::Throw(HX_CSTRING("Failed to get encrypted key size : ") + hx::ssl::windows::utils::Win32ErrorToString(result));
			}

			auto encryptionInfoBuffer = std::vector<uint8_t>(cb);
			auto encryptionInfo = reinterpret_cast<PCRYPT_ENCRYPTED_PRIVATE_KEY_INFO>(encryptionInfoBuffer.data());
			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_ENCRYPTED_PRIVATE_KEY_INFO, derKey.data(), derKey.size(), 0, nullptr, encryptionInfoBuffer.data(), &cb))
			{
				hx::Throw(HX_CSTRING("Failed to get encrypted key info : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			if (FAILED(DecryptPrivateKey(encryptionInfo, pass)))
			{
				hx::Throw(HX_CSTRING("Failed to get decrypt key : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_PRIVATE_KEY_INFO, encryptionInfo->EncryptedPrivateKey.pbData, encryptionInfo->EncryptedPrivateKey.cbData, CRYPT_DECODE_NOCOPY_FLAG, nullptr, nullptr, &cb))
			{
				hx::Throw(HX_CSTRING("Failed to decode private key size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			auto keyInfoBuffer = std::vector<uint8_t>(cb);
			auto keyInfo = reinterpret_cast<PCRYPT_PRIVATE_KEY_INFO>(keyInfoBuffer.data());
			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_PRIVATE_KEY_INFO, encryptionInfo->EncryptedPrivateKey.pbData, encryptionInfo->EncryptedPrivateKey.cbData, CRYPT_DECODE_NOCOPY_FLAG, nullptr, keyInfoBuffer.data(), &cb))
			{
				hx::Throw(HX_CSTRING("Failed to decode private key: ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, keyInfo->PrivateKey.pbData, keyInfo->PrivateKey.cbData, 0, nullptr, nullptr, &cb))
			{
				hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			auto rsaKeyBuffer = std::vector<uint8_t>(cb);
			auto rsaKey = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(rsaKeyBuffer.data());
			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, keyInfo->PrivateKey.pbData, keyInfo->PrivateKey.cbData, 0, nullptr, rsaKeyBuffer.data(), &cb))
			{
				hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			auto algorithm = BCRYPT_ALG_HANDLE();
			if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_RSA_ALGORITHM, nullptr, 0)))
			{
				hx::Throw(HX_CSTRING("Failed to open RSA provider"));
			}

			auto key = BCRYPT_KEY_HANDLE();
			if (!BCRYPT_SUCCESS(result = BCryptImportKeyPair(algorithm, nullptr, BCRYPT_RSAPRIVATE_BLOB, &key, reinterpret_cast<PUCHAR>(rsaKeyBuffer.data()), rsaKeyBuffer.size(), BCRYPT_NO_KEY_VALIDATION)))
			{
				hx::Throw(HX_CSTRING("Failed to import private key"));
			}

			return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
		}
		else
		{
			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_PRIVATE_KEY_INFO, derKey.data(), derKey.size(), CRYPT_DECODE_NOCOPY_FLAG, nullptr, nullptr, &cb))
			{
				hx::Throw(HX_CSTRING("Failed to decode private key size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			auto keyInfoBuffer = std::vector<uint8_t>(cb);
			auto keyInfo = reinterpret_cast<PCRYPT_PRIVATE_KEY_INFO>(keyInfoBuffer.data());
			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_PRIVATE_KEY_INFO, derKey.data(), derKey.size(), CRYPT_DECODE_NOCOPY_FLAG, nullptr, keyInfoBuffer.data(), &cb))
			{
				hx::Throw(HX_CSTRING("Failed to decode private key: ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, keyInfo->PrivateKey.pbData, keyInfo->PrivateKey.cbData, 0, nullptr, nullptr, &cb))
			{
				hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			auto rsaKeyBuffer = std::vector<uint8_t>(cb);
			auto rsaKey = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(rsaKeyBuffer.data());
			if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, CNG_RSA_PRIVATE_KEY_BLOB, keyInfo->PrivateKey.pbData, keyInfo->PrivateKey.cbData, 0, nullptr, rsaKeyBuffer.data(), &cb))
			{
				hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			auto algorithm = BCRYPT_ALG_HANDLE();
			if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_RSA_ALGORITHM, nullptr, 0)))
			{
				hx::Throw(HX_CSTRING("Failed to open RSA provider"));
			}

			auto key = BCRYPT_KEY_HANDLE();
			if (!BCRYPT_SUCCESS(result = BCryptImportKeyPair(algorithm, nullptr, BCRYPT_RSAPRIVATE_BLOB, &key, reinterpret_cast<PUCHAR>(rsaKeyBuffer.data()), rsaKeyBuffer.size(), BCRYPT_NO_KEY_VALIDATION)))
			{
				hx::Throw(HX_CSTRING("Failed to import private key"));
			}

			return hx::ssl::windows::Key(new hx::ssl::windows::Key_obj(key));
		}
	}
}
