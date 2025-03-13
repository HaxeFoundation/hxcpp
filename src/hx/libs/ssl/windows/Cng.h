#pragma once

#include <memory>
#include "Utils.h"
#include "bcrypt.h"

namespace hx
{
	namespace ssl
	{
		namespace windows
		{
			struct CngObject
			{
				BCRYPT_HANDLE handle;

				CngObject(BCRYPT_HANDLE inHandle) : handle(inHandle) {}

				template<class T>
				T getProperty(const wchar_t* prop) const
				{
					hx::EnterGCFreeZone();

					auto result = DWORD{ 0 };
					auto cb     = ULONG{ 0 };

					auto buffer = T();
					if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, prop, reinterpret_cast<PUCHAR>(&buffer), sizeof(T), &cb, 0)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to get property : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();

					return buffer;
				}

				void setProperty(const wchar_t* prop, PUCHAR input, const ULONG length) const
				{
					hx::EnterGCFreeZone();

					auto result = DWORD{ 0 };

					if (!BCRYPT_SUCCESS(result = BCryptSetProperty(handle, prop, input, length, 0)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failure"));
					}

					hx::ExitGCFreeZone();
				}
			};

			struct CngHash : CngObject
			{
				std::unique_ptr<std::vector<uint8_t>> object;

				CngHash(BCRYPT_HASH_HANDLE inHandle, std::unique_ptr<std::vector<uint8_t>> inObject)
					: CngObject(inHandle)
					, object(std::move(inObject))
				{
					//
				}

				~CngHash()
				{
					BCryptDestroyHash(handle);
				}

				void hash(const Array<uint8_t>& input) const
				{
					hx::EnterGCFreeZone();

					auto result = DWORD{ 0 };
					if (!BCRYPT_SUCCESS(result = BCryptHashData(handle, reinterpret_cast<PUCHAR>(input->getBase()), input->length, 0)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to hash data : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();
				}

				void hash(const PUCHAR input, const ULONG length) const
				{
					hx::EnterGCFreeZone();

					auto result = DWORD{ 0 };
					if (!BCRYPT_SUCCESS(result = BCryptHashData(handle, input, length, 0)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to hash data : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();
				}

				void finish(PUCHAR output, const ULONG length) const
				{
					hx::EnterGCFreeZone();

					auto result = DWORD{ 0 };
					if (!BCRYPT_SUCCESS(result = BCryptFinishHash(handle, output, length, 0)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to finish hash : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();
				}

				Array<uint8_t> finish() const
				{
					auto result = DWORD{ 0 };
					auto length = getProperty<DWORD>(BCRYPT_HASH_LENGTH);
					auto output = Array<uint8_t>(length, length);

					hx::EnterGCFreeZone();

					if (!BCRYPT_SUCCESS(result = BCryptFinishHash(handle, reinterpret_cast<PUCHAR>(output->getBase()), output->length, 0)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to finish hash : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();

					return output;
				}
			};

			struct CngAlgorithm : CngObject
			{
				ULONG flags;

				CngAlgorithm(BCRYPT_ALG_HANDLE inHandle, ULONG inFlags) : CngObject(inHandle), flags(inFlags) {}

			public:
				~CngAlgorithm()
				{
					BCryptCloseAlgorithmProvider(handle, 0);
				}

				std::unique_ptr<CngHash> hash() const
				{
					auto result = DWORD{ 0 };
					auto hHash  = BCRYPT_HASH_HANDLE();
					auto object = std::make_unique<std::vector<uint8_t>>(getProperty<DWORD>(BCRYPT_OBJECT_LENGTH));

					hx::EnterGCFreeZone();

					if (!BCRYPT_SUCCESS(result = BCryptCreateHash(handle, &hHash, object->data(), object->size(), nullptr, 0, flags)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to create hash object : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();

					return std::make_unique<CngHash>(hHash, std::move(object));
				}

				static std::unique_ptr<CngAlgorithm> create(const wchar_t* algId, ULONG flags)
				{
					auto result = DWORD{ 0 };
					auto handle = BCRYPT_ALG_HANDLE();

					hx::EnterGCFreeZone();

					if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&handle, algId, nullptr, flags)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to open algorithm provider : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();

					return std::make_unique<CngAlgorithm>(handle, flags);
				}
			};
		}
	}
}
