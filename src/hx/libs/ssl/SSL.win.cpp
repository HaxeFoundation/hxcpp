#include <hxcpp.h>
#include <hx/OS.h>
#include <winsock2.h>
#include <wincrypt.h>
#include <vector>

HX_DECLARE_CLASS0(SChannelCert);

class SChannelCert_obj : public hx::Object
{
	static void destroy(Dynamic obj)
	{
		auto cert = reinterpret_cast<SChannelCert_obj*>(obj.mPtr);

		CertFreeCertificateContext(cert->ctx);

		cert->ctx = nullptr;
	}

public:
	HX_IS_INSTANCE_OF enum { _hx_classId = hx::clsIdSslCert };

	PCCERT_CONTEXT ctx;

	SChannelCert_obj(PCCERT_CONTEXT inCtx) : ctx(inCtx)
	{
		_hx_set_finalizer(this, destroy);
	}

	String toString()
	{
		return HX_CSTRING("SChannelCert");
	}
};

//

String Win32ErrorToString(DWORD error)
{
	auto messageBuffer = LPSTR{ nullptr };

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&messageBuffer,
		0,
		NULL);

	auto hxStr = String::create(messageBuffer);

	LocalFree(messageBuffer);

	return hxStr;
}

void AddAltNamesFrom(SChannelCert cert, const char* source, Array<String> result)
{
	auto ext = CertFindExtension(source, cert->ctx->pCertInfo->cExtension, cert->ctx->pCertInfo->rgExtension);
	if (nullptr == ext)
	{
		return;
	}

	auto size = DWORD{ 0 };
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, ext->pszObjId, ext->Value.pbData, ext->Value.cbData, 0, nullptr, nullptr, &size))
	{
		hx::Throw(HX_CSTRING("Failed to get the buffer size : ") + Win32ErrorToString(GetLastError()));
	}

	auto buffer = std::vector<uint8_t>(size);
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, ext->pszObjId, ext->Value.pbData, ext->Value.cbData, 0, nullptr, buffer.data(), &size))
	{
		hx::Throw(HX_CSTRING("Failed to decode object : ") + Win32ErrorToString(GetLastError()));
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

//

void _hx_ssl_init()
{
	//
}

Dynamic _hx_ssl_new(Dynamic hconf)
{
	return null();
}

void _hx_ssl_close(Dynamic hssl)
{
	//
}

void _hx_ssl_debug_set(int i)
{
	//
}

void _hx_ssl_handshake(Dynamic handle)
{
	//
}

void _hx_ssl_set_socket(Dynamic hssl, Dynamic hsocket)
{
	//
}

void _hx_ssl_set_hostname(Dynamic hssl, String hostname)
{
	//
}

Dynamic _hx_ssl_get_peer_certificate(Dynamic hssl)
{
	return null();
}

bool _hx_ssl_get_verify_result(Dynamic hssl)
{
	return false;
}

void _hx_ssl_send_char(Dynamic hssl, int v)
{
	//
}

int _hx_ssl_send(Dynamic hssl, Array<unsigned char> buf, int p, int l)
{
	return 0;
}

void _hx_ssl_write(Dynamic hssl, Array<unsigned char> buf)
{
	//
}

int _hx_ssl_recv_char(Dynamic hssl)
{
	return 0;
}

int _hx_ssl_recv(Dynamic hssl, Array<unsigned char> buf, int p, int l)
{
	return 0;
}

Array<unsigned char> _hx_ssl_read(Dynamic hssl)
{
	return null();
}

Dynamic _hx_ssl_conf_new(bool server)
{
	return null();
}

void _hx_ssl_conf_close(Dynamic hconf)
{
	//
}

void _hx_ssl_conf_set_ca(Dynamic hconf, Dynamic hcert)
{
	//
}

void _hx_ssl_conf_set_verify(Dynamic hconf, int mode)
{
	//
}

void _hx_ssl_conf_set_cert(Dynamic hconf, Dynamic hcert, Dynamic hpkey)
{
	//
}

void _hx_ssl_conf_set_servername_callback(Dynamic hconf, Dynamic obj)
{
	//
}

Dynamic _hx_ssl_cert_load_defaults()
{
	return null();
}

Dynamic _hx_ssl_cert_load_file(String file)
{
	auto handle = CreateFileA(file.utf8_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == handle)
	{
		hx::Throw(HX_CSTRING("Invalid file handle : ") + Win32ErrorToString(GetLastError()));
	}

	auto readLength = LARGE_INTEGER{ 0 };
	if (!GetFileSizeEx(handle, &readLength))
	{
		CloseHandle(handle);

		hx::Throw(HX_CSTRING("Failed to read file size : ") + Win32ErrorToString(GetLastError()));
	}

	auto pubKey = std::vector<char>(readLength.QuadPart);
	if (!ReadFile(handle, pubKey.data(), readLength.QuadPart, nullptr, nullptr))
	{
		CloseHandle(handle);

		hx::Throw(HX_CSTRING("Failed to read file : ") + Win32ErrorToString(GetLastError()));
	}

	CloseHandle(handle);

	auto derPubKey = std::vector<uint8_t>(readLength.QuadPart);
	auto derPubKeyLength = static_cast<DWORD>(readLength.QuadPart);
	if (!CryptStringToBinary(pubKey.data(), 0, CRYPT_STRING_BASE64HEADER, derPubKey.data(), &derPubKeyLength, nullptr, nullptr))
	{
		hx::Throw(HX_CSTRING("Failed to parse certificate : ") + Win32ErrorToString(GetLastError()));
	}

	auto cert = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, derPubKey.data(), derPubKeyLength);
	if (nullptr == cert)
	{
		hx::Throw(HX_CSTRING("Failed to parse certificate : ") + Win32ErrorToString(GetLastError()));
	}

	return SChannelCert(new SChannelCert_obj(cert));
}

Dynamic _hx_ssl_cert_load_path(String path)
{
	return null();
}

String _hx_ssl_cert_get_subject(Dynamic hcert, String objname)
{
	auto cert = hcert.Cast<SChannelCert>();
	auto size = DWORD{ 0 };
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_NAME, cert->ctx->pCertInfo->Subject.pbData, cert->ctx->pCertInfo->Subject.cbData, 0, nullptr, nullptr, &size))
	{
		hx::Throw(HX_CSTRING("Failed to get the buffer size : ") + Win32ErrorToString(GetLastError()));
	}

	auto buffer = std::vector<uint8_t>(size);
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_NAME, cert->ctx->pCertInfo->Subject.pbData, cert->ctx->pCertInfo->Subject.cbData, 0, nullptr, buffer.data(), &size))
	{
		hx::Throw(HX_CSTRING("Failed to decode object : ") + Win32ErrorToString(GetLastError()));
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
				hx::Throw(HX_CSTRING("Failed to find OID info : ") + Win32ErrorToString(GetLastError()));
			}

			if (String::create(oidInfo->pwszName) == objname)
			{
				return String::create(reinterpret_cast<char*>(attr.Value.pbData), attr.Value.cbData);
			}
		}
	}

	hx::Throw(HX_CSTRING("Failed to find subject attribute"));

	return null();
}

String _hx_ssl_cert_get_issuer(Dynamic hcert, String objname)
{
	auto cert = hcert.Cast<SChannelCert>();
	auto size = DWORD{ 0 };
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_NAME, cert->ctx->pCertInfo->Issuer.pbData, cert->ctx->pCertInfo->Issuer.cbData, 0, nullptr, nullptr, &size))
	{
		hx::Throw(HX_CSTRING("Failed to get the buffer size : ") + Win32ErrorToString(GetLastError()));
	}

	auto buffer = std::vector<uint8_t>(size);
	if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_NAME, cert->ctx->pCertInfo->Issuer.pbData, cert->ctx->pCertInfo->Issuer.cbData, 0, nullptr, buffer.data(), &size))
	{
		hx::Throw(HX_CSTRING("Failed to decode object : ") + Win32ErrorToString(GetLastError()));
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
				hx::Throw(HX_CSTRING("Failed to find OID info : ") + Win32ErrorToString(GetLastError()));
			}

			if (String::create(oidInfo->pwszName) == objname)
			{
				return String::create(reinterpret_cast<char*>(attr.Value.pbData), attr.Value.cbData);
			}
		}
	}

	hx::Throw(HX_CSTRING("Failed to find issuer attribute"));

	return null();
}

Array<String> _hx_ssl_cert_get_altnames(Dynamic hcert)
{
	auto cert   = hcert.Cast<SChannelCert>();
	auto result = Array<String>(0, 0);

	AddAltNamesFrom(cert, szOID_SUBJECT_ALT_NAME2, result);
	AddAltNamesFrom(cert, szOID_ISSUER_ALT_NAME2, result);

	return result;
}

Array<int> _hx_ssl_cert_get_notbefore(Dynamic hcert)
{
	auto cert    = hcert.Cast<SChannelCert>();
	auto sysTime = SYSTEMTIME();

	if (!FileTimeToSystemTime(&cert->ctx->pCertInfo->NotBefore, &sysTime))
	{
		hx::Throw(HX_CSTRING("Failed to get system time : ") + Win32ErrorToString(GetLastError()));
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

Array<int> _hx_ssl_cert_get_notafter(Dynamic hcert)
{
	auto cert    = hcert.Cast<SChannelCert>();
	auto sysTime = SYSTEMTIME();

	if (!FileTimeToSystemTime(&cert->ctx->pCertInfo->NotAfter, &sysTime))
	{
		hx::Throw(HX_CSTRING("Failed to get system time : ") + Win32ErrorToString(GetLastError()));
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
	return null();
}

Dynamic _hx_ssl_cert_add_pem(Dynamic hcert, String data)
{
	return null();
}

Dynamic _hx_ssl_cert_add_der(Dynamic hcert, Array<unsigned char> buf)
{
	return null();
}

Dynamic _hx_ssl_key_from_der(Array<unsigned char> buf, bool pub)
{
	return null();
}

Dynamic _hx_ssl_key_from_pem(String data, bool pub, String pass)
{
	return null();
}

Array<unsigned char> _hx_ssl_dgst_make(Array<unsigned char> buf, String alg)
{
	return null();
}

Array<unsigned char> _hx_ssl_dgst_sign(Array<unsigned char> buf, Dynamic hpkey, String alg)
{
	return null();
}

bool _hx_ssl_dgst_verify(Array<unsigned char> buf, Array<unsigned char> sign, Dynamic hpkey, String alg)
{
	return false;
}
