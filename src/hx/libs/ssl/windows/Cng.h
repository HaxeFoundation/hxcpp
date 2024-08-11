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
			template<class T>
			T getProperty(BCRYPT_HANDLE handle, const wchar_t* prop)
			{
				hx::EnterGCFreeZone();

				auto result = DWORD{ 0 };
				auto size   = ULONG{ 0 };
				if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, prop, nullptr, 0, &size, 0)))
				{
					hx::ExitGCFreeZone();
					hx::Throw(HX_CSTRING("Failed to get property size : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
				}

				auto buffer = std::vector<UCHAR>(size);
				if (!BCRYPT_SUCCESS(result = BCryptGetProperty(handle, prop, buffer.data(), buffer.size(), &size, 0)))
				{
					hx::ExitGCFreeZone();
					hx::Throw(HX_CSTRING("Failed to get property : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
				}

				hx::ExitGCFreeZone();

				return static_cast<T>(buffer[0]);
			}

			struct CngHash
			{
				BCRYPT_HASH_HANDLE handle;
				std::unique_ptr<std::vector<uint8_t>> object;

				CngHash(BCRYPT_HASH_HANDLE inHandle, std::unique_ptr<std::vector<uint8_t>> inObject)
					: handle(inHandle)
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
					auto length = getProperty<DWORD>(handle, BCRYPT_HASH_LENGTH);
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

			struct CngAlgorithm
			{
				CngAlgorithm(BCRYPT_ALG_HANDLE inHandle) : handle(inHandle) {}

			public:
				BCRYPT_ALG_HANDLE handle;

				~CngAlgorithm()
				{
					BCryptCloseAlgorithmProvider(handle, 0);
				}

				std::unique_ptr<CngHash> hash()
				{
					hx::EnterGCFreeZone();

					auto result = DWORD{ 0 };
					auto hHash  = BCRYPT_HASH_HANDLE();
					auto object = std::make_unique<std::vector<uint8_t>>(getProperty<DWORD>(handle, BCRYPT_OBJECT_LENGTH));

					if (!BCRYPT_SUCCESS(result = BCryptCreateHash(handle, &hHash, object->data(), object->size(), nullptr, 0, BCRYPT_HASH_REUSABLE_FLAG)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to create hash object : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();

					return std::make_unique<CngHash>(hHash, std::move(object));
				}

				static std::unique_ptr<CngAlgorithm> create(const wchar_t* algId)
				{
					hx::EnterGCFreeZone();

					auto result = DWORD{ 0 };
					auto handle = BCRYPT_ALG_HANDLE();

					if (!BCRYPT_SUCCESS(result = BCryptOpenAlgorithmProvider(&handle, algId, nullptr, BCRYPT_HASH_REUSABLE_FLAG)))
					{
						hx::ExitGCFreeZone();
						hx::Throw(HX_CSTRING("Failed to open algorithm provider : ") + hx::ssl::windows::utils::NTStatusErrorToString(result));
					}

					hx::ExitGCFreeZone();

					return std::make_unique<CngAlgorithm>(handle);
				}
			};
		}
	}
}
