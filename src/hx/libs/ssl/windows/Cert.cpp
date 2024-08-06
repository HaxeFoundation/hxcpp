#include <hxcpp.h>
#include <Windows.h>

#include "SSL.h"
#include "Utils.h"

namespace
{
	void AddAltNamesFrom(hx::ssl::windows::Cert cert, const char* source, Array<String> result)
	{
		auto ext = CertFindExtension(source, cert->ctx->pCertInfo->cExtension, cert->ctx->pCertInfo->rgExtension);
		if (nullptr == ext)
		{
			return;
		}

		auto size = DWORD{ 0 };
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, ext->pszObjId, ext->Value.pbData, ext->Value.cbData, 0, nullptr, nullptr, &size))
		{
			hx::Throw(HX_CSTRING("Failed to get the buffer size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto buffer = std::vector<uint8_t>(size);
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, ext->pszObjId, ext->Value.pbData, ext->Value.cbData, 0, nullptr, buffer.data(), &size))
		{
			hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto info = reinterpret_cast<PCERT_ALT_NAME_INFO>(buffer.data());
		for (auto i = 0; i < info->cAltEntry; i++)
		{
			auto& entry = info->rgAltEntry[i];

			switch (entry.dwAltNameChoice)
			{
			case CERT_ALT_NAME_OTHER_NAME:
			{
				result->push(String::create(reinterpret_cast<char*>(info[i].rgAltEntry->pOtherName->Value.pbData), info[i].rgAltEntry->pOtherName->Value.cbData));
				break;
			}
			case CERT_ALT_NAME_RFC822_NAME:
			{
				result->push(String::create(entry.pwszRfc822Name));
				break;
			}
			case CERT_ALT_NAME_DNS_NAME:
			{
				result->push(String::create(entry.pwszDNSName));
				break;
			}
			case CERT_ALT_NAME_DIRECTORY_NAME:
			{
				result->push(String::create(reinterpret_cast<char*>(info[i].rgAltEntry->DirectoryName.pbData), info[i].rgAltEntry->DirectoryName.cbData));
				break;
			}
			case CERT_ALT_NAME_URL:
			{
				result->push(String::create(entry.pwszURL));
				break;
			}
			case CERT_ALT_NAME_IP_ADDRESS:
			{
				result->push(String::create(reinterpret_cast<char*>(info[i].rgAltEntry->IPAddress.pbData), info[i].rgAltEntry->IPAddress.cbData));
				break;
			}
			case CERT_ALT_NAME_REGISTERED_ID:
			{
				result->push(String::create(entry.pszRegisteredID));
				break;
			}
			default:
				hx::Throw(HX_CSTRING("Unknown alt name choise : ") + Dynamic(entry.dwAltNameChoice));
			}
		}
	}
}

Dynamic _hx_ssl_cert_load_file(String file)
{
	hx::strbuf buffer;
	auto handle = CreateFileA(file.utf8_str(&buffer), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == handle)
	{
		hx::Throw(HX_CSTRING("Invalid file handle : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto readLength = LARGE_INTEGER{ 0 };
	if (!GetFileSizeEx(handle, &readLength))
	{
		CloseHandle(handle);

		hx::Throw(HX_CSTRING("Failed to read file size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto pubKey = std::vector<char>(readLength.QuadPart);
	if (!ReadFile(handle, pubKey.data(), readLength.QuadPart, nullptr, nullptr))
	{
		CloseHandle(handle);

		hx::Throw(HX_CSTRING("Failed to read file : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	CloseHandle(handle);

	auto derPubKey       = std::vector<uint8_t>(readLength.QuadPart);
	auto derPubKeyLength = static_cast<DWORD>(readLength.QuadPart);
	if (!CryptStringToBinaryA(pubKey.data(), 0, CRYPT_STRING_BASE64HEADER, derPubKey.data(), &derPubKeyLength, nullptr, nullptr))
	{
		hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto cert = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, derPubKey.data(), derPubKeyLength);
	if (nullptr == cert)
	{
		hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	return hx::ssl::windows::Cert(new hx::ssl::windows::Cert_obj(cert));
}

Dynamic _hx_ssl_cert_load_path(String path)
{
	return null();
}

String _hx_ssl_cert_get_subject(Dynamic hcert, String objname)
{
	auto cert = hcert.Cast<hx::ssl::windows::Cert>();
	auto size = DWORD{ 0 };
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_NAME, cert->ctx->pCertInfo->Subject.pbData, cert->ctx->pCertInfo->Subject.cbData, 0, nullptr, nullptr, &size))
	{
		hx::Throw(HX_CSTRING("Failed to get the buffer size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto buffer = std::vector<uint8_t>(size);
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_NAME, cert->ctx->pCertInfo->Subject.pbData, cert->ctx->pCertInfo->Subject.cbData, 0, nullptr, buffer.data(), &size))
	{
		hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto info = reinterpret_cast<PCERT_NAME_INFO>(buffer.data());

	for (auto i = 0; i < info->cRDN; i++)
	{
		auto& entry = info->rgRDN[i];

		for (auto j = 0; j < entry.cRDNAttr; j++)
		{
			auto& attr = entry.rgRDNAttr[j];

			auto oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, attr.pszObjId, CRYPT_RDN_ATTR_OID_GROUP_ID);
			if (nullptr == oidInfo)
			{
				hx::Throw(HX_CSTRING("Failed to find OID info : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			if (String::create(oidInfo->pwszName) == objname)
			{
				switch (attr.dwValueType)
				{
				case CERT_RDN_UTF8_STRING:
				case CERT_RDN_UNICODE_STRING:
					return String::create(reinterpret_cast<wchar_t*>(attr.Value.pbData), attr.Value.cbData / sizeof(uint16_t));
				case CERT_RDN_PRINTABLE_STRING:
				case CERT_RDN_IA5_STRING:
				case CERT_RDN_NUMERIC_STRING:
				case CERT_RDN_OCTET_STRING:
					return String::create(reinterpret_cast<char*>(attr.Value.pbData), attr.Value.cbData / sizeof(uint8_t));
				default:
					hx::Throw(HX_CSTRING("Unsupported RDN attribute data"));
				}
			}
		}
	}

	return null();
}

String _hx_ssl_cert_get_issuer(Dynamic hcert, String objname)
{
	auto cert = hcert.Cast<hx::ssl::windows::Cert>();
	auto size = DWORD{ 0 };
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_NAME, cert->ctx->pCertInfo->Issuer.pbData, cert->ctx->pCertInfo->Issuer.cbData, 0, nullptr, nullptr, &size))
	{
		hx::Throw(HX_CSTRING("Failed to get the buffer size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto buffer = std::vector<uint8_t>(size);
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_NAME, cert->ctx->pCertInfo->Issuer.pbData, cert->ctx->pCertInfo->Issuer.cbData, 0, nullptr, buffer.data(), &size))
	{
		hx::Throw(HX_CSTRING("Failed to decode object : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto info = reinterpret_cast<PCERT_NAME_INFO>(buffer.data());

	for (auto i = 0; i < info->cRDN; i++)
	{
		auto& entry = info->rgRDN[i];

		for (auto j = 0; j < entry.cRDNAttr; j++)
		{
			auto& attr = entry.rgRDNAttr[j];

			auto oidInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, attr.pszObjId, CRYPT_RDN_ATTR_OID_GROUP_ID);
			if (nullptr == oidInfo)
			{
				hx::Throw(HX_CSTRING("Failed to find OID info : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
			}

			if (String::create(oidInfo->pwszName) == objname)
			{
				switch (attr.dwValueType)
				{
				case CERT_RDN_UTF8_STRING:
				case CERT_RDN_UNICODE_STRING:
					return String::create(reinterpret_cast<wchar_t*>(attr.Value.pbData), attr.Value.cbData / sizeof(uint16_t));
				case CERT_RDN_PRINTABLE_STRING:
				case CERT_RDN_IA5_STRING:
				case CERT_RDN_NUMERIC_STRING:
				case CERT_RDN_OCTET_STRING:
					return String::create(reinterpret_cast<char*>(attr.Value.pbData), attr.Value.cbData / sizeof(uint8_t));
				default:
					hx::Throw(HX_CSTRING("Unsupported RDN attribute data"));
				}
			}
		}
	}

	return null();
}

Array<String> _hx_ssl_cert_get_altnames(Dynamic hcert)
{
	auto cert = hcert.Cast<hx::ssl::windows::Cert>();
	auto result = Array<String>(0, 0);

	AddAltNamesFrom(cert, szOID_SUBJECT_ALT_NAME2, result);
	AddAltNamesFrom(cert, szOID_ISSUER_ALT_NAME2, result);

	return result;
}

Array<int> _hx_ssl_cert_get_notbefore(Dynamic hcert)
{
	auto cert = hcert.Cast<hx::ssl::windows::Cert>();
	auto sysTime = SYSTEMTIME();

	if (!FileTimeToSystemTime(&cert->ctx->pCertInfo->NotBefore, &sysTime))
	{
		hx::Throw(HX_CSTRING("Failed to get system time : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto result = Array<int>(6, 6);
	result[0] = sysTime.wYear;
	result[1] = sysTime.wMonth;
	result[2] = sysTime.wDay - 1;
	result[3] = sysTime.wHour;
	result[4] = sysTime.wMinute;
	result[5] = sysTime.wSecond;
	return result;
}

Array<int> _hx_ssl_cert_get_notafter(Dynamic hcert)
{
	auto cert = hcert.Cast<hx::ssl::windows::Cert>();
	auto sysTime = SYSTEMTIME();

	if (!FileTimeToSystemTime(&cert->ctx->pCertInfo->NotAfter, &sysTime))
	{
		hx::Throw(HX_CSTRING("Failed to get system time : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto result = Array<int>(6, 6);
	result[0] = sysTime.wYear;
	result[1] = sysTime.wMonth;
	result[2] = sysTime.wDay;
	result[3] = sysTime.wHour;
	result[4] = sysTime.wMinute;
	result[5] = sysTime.wSecond;
	return result;
}

Dynamic _hx_ssl_cert_get_next(Dynamic hcert)
{
	auto cert  = hcert.Cast<hx::ssl::windows::Cert>();
	auto param = CERT_CHAIN_PARA();
	auto chain = PCCERT_CHAIN_CONTEXT{ nullptr };

	if (!CertGetCertificateChain(nullptr, cert->ctx, nullptr, nullptr, &param, 0, nullptr, &chain))
	{
		hx::Throw(HX_CSTRING("Failed to get certificate chain : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
	}

	auto firstChain = chain->rgpChain[0];

	for (auto i = 0; i < firstChain->cElement; i++)
	{
		if (i + 1 < firstChain->cElement && CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, cert->ctx->pCertInfo, firstChain->rgpElement[i]->pCertContext->pCertInfo))
		{
			CertFreeCertificateChain(chain);

			// I'm not sure of the lifetime of the PCCERT_CONTEXT objects here.
			// The Cert_obj will release the cert when it get finalised but would that cause issues if a cert
			// further up the chain is still alive?

			return new hx::ssl::windows::Cert_obj(chain->rgpChain[0]->rgpElement[i + 1]->pCertContext);
		}
	}

	CertFreeCertificateChain(chain);

	return null();
}

Dynamic _hx_ssl_cert_add_pem(Dynamic hcert, String data)
{
	hx::strbuf buffer;
	auto str    = data.utf8_str(&buffer);

	if (hx::IsNull(hcert))
	{
		hx::EnterGCFreeZone();

		auto derKeyLength = DWORD{ 0 };
		if (!CryptStringToBinaryA(str, 0, CRYPT_STRING_BASE64HEADER, nullptr, &derKeyLength, nullptr, nullptr))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to calculate DER buffer size : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto derKey = std::vector<uint8_t>(derKeyLength);
		if (!CryptStringToBinaryA(str, 0, CRYPT_STRING_BASE64HEADER, derKey.data(), &derKeyLength, nullptr, nullptr))
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to decrypt PEM encoded buffer : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		auto cert = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, derKey.data(), derKeyLength);
		if (nullptr == cert)
		{
			hx::ExitGCFreeZone();
			hx::Throw(HX_CSTRING("Failed to parse certificate : ") + hx::ssl::windows::utils::Win32ErrorToString(GetLastError()));
		}

		hx::ExitGCFreeZone();

		return hx::ssl::windows::Cert(new hx::ssl::windows::Cert_obj(cert));
	}
	
	//auto cert = hcert.Cast<hx::ssl::windows::Cert>();

	hx::ExitGCFreeZone();
	hx::Throw(HX_CSTRING("Not Implemented"));
}

Dynamic _hx_ssl_cert_add_der(Dynamic hcert, Array<unsigned char> buf)
{
	return null();
}
