#pragma once

#include <exception>
#include <format>
#include <memory>
#include <string>
#include <dlfcn.h>
#include "firebird/Interface.h"


namespace rinhaback::api
{
	namespace fb = Firebird;

	constexpr int CS_ASCII = 2;

	namespace impl
	{
		struct FbDeleter final
		{
			void operator()(fb::IDisposable* obj) noexcept
			{
				obj->dispose();
			}
		};
	}  // namespace impl

	template <typename T>
	using FbUniquePtr = std::unique_ptr<T, impl::FbDeleter>;

	template <typename T>
	FbUniquePtr<T> fbUnique(T* obj)
	{
		return FbUniquePtr<T>(obj);
	}

	template <typename T>
	class FbRef final
	{
		template <typename Y>
		friend class FbRef;

	public:
		FbRef() noexcept
			: ptr{nullptr}
		{
		}

		FbRef(std::nullptr_t) noexcept
			: ptr{nullptr}
		{
		}

		explicit FbRef(T* p) noexcept
			: ptr{p}
		{
		}

		template <typename Y>
		explicit FbRef(Y* p) noexcept
			: ptr{p}
		{
		}

		FbRef(FbRef& r) noexcept
			: ptr{nullptr}
		{
			assign(r.ptr, true);
		}

		template <typename Y>
		FbRef(FbRef<Y>& r) noexcept
			: ptr{nullptr}
		{
			assign(r.ptr, true);
		}

		FbRef(FbRef&& r) noexcept
			: ptr{r.ptr}
		{
			r.ptr = nullptr;
		}

		~FbRef() noexcept
		{
			if (ptr)
				ptr->release();
		}

		void reset(T* p = nullptr) noexcept
		{
			assign(p, false);
		}

		FbRef& operator=(FbRef& r) noexcept
		{
			assign(r.ptr, true);
			return *this;
		}

		template <typename Y>
		FbRef& operator=(FbRef<Y>& r) noexcept
		{
			assign(r.ptr, true);
			return *this;
		}

		FbRef& operator=(FbRef&& r) noexcept
		{
			if (ptr != r.ptr)
			{
				if (ptr)
					assign(nullptr);

				ptr = r.ptr;
				r.ptr = nullptr;
			}

			return *this;
		}

		T* operator->() noexcept
		{
			return ptr;
		}

		const T* operator->() const noexcept
		{
			return ptr;
		}

		explicit operator bool() const noexcept
		{
			return ptr != nullptr;
		}

		bool operator!() const noexcept
		{
			return ptr == nullptr;
		}

		bool operator==(const FbRef& r) const noexcept
		{
			return ptr == r.ptr;
		}

		bool operator!=(const FbRef& r) const noexcept
		{
			return ptr != r.ptr;
		}

		T* get() noexcept
		{
			return ptr;
		}

		const T* get() const noexcept
		{
			return ptr;
		}

	private:
		T* assign(T* const p, bool addRef) noexcept
		{
			if (ptr != p)
			{
				if (p && addRef)
					p->addRef();

				T* tmp = ptr;
				ptr = p;

				if (tmp)
					tmp->release();
			}

			return p;
		}

	private:
		T* ptr;
	};

	class FirebirdStlException final : public std::exception
	{
	public:
		FirebirdStlException(fb::IStatus* aStatus, const ISC_STATUS* vector)
		{
			aStatus->setErrors(vector);
			status = aStatus->clone();
			saveMessage();
		}

		explicit FirebirdStlException(fb::IStatus* aStatus)
			: status(aStatus->clone())
		{
			saveMessage();
		}

		FirebirdStlException(const FirebirdStlException& copy)
			: status(copy.status->clone()),
			  textMessage(copy.textMessage)
		{
		}

		FirebirdStlException& operator=(const FirebirdStlException& copy)
		{
			status->dispose();
			status = copy.status->clone();
			textMessage = copy.textMessage;
			return *this;
		}

		~FirebirdStlException() override
		{
			status->dispose();
		}

	public:
		const char* what() const noexcept override
		{
			return textMessage.c_str();
		}

	public:
		static void check(ISC_STATUS code, fb::IStatus* status, const ISC_STATUS* vector)
		{
			if (code != 0 && vector[1])
				throw FirebirdStlException(status, vector);
		}

	public:
		fb::IStatus* getStatus() const
		{
			return status;
		}

	private:
		void saveMessage();

	private:
		fb::IStatus* status;
		std::string textMessage;
	};

	class FirebirdStlStatusWrapper final : public fb::BaseStatusWrapper<FirebirdStlStatusWrapper>
	{
	public:
		explicit FirebirdStlStatusWrapper(fb::IStatus* aStatus)
			: BaseStatusWrapper(aStatus)
		{
		}

	public:
		static void checkException(FirebirdStlStatusWrapper* status)
		{
			if (status->dirty && (status->getState() & IStatus::STATE_ERRORS))
				throw FirebirdStlException(status->status);
		}
	};

	class FirebirdClient final
	{
	private:
		explicit FirebirdClient();

	public:
		~FirebirdClient();

		FirebirdClient(const FirebirdClient&) = delete;
		FirebirdClient& operator=(const FirebirdClient&) = delete;

		static FirebirdClient& getInstance()
		{
			static FirebirdClient instance;
			return instance;
		}

	public:
		auto getMaster()
		{
			return master;
		}

		auto getUtil()
		{
			return util;
		}

		auto getDispatcher()
		{
			return dispatcher.get();
		}

		auto newStatus()
		{
			return fbUnique(master->getStatus());
		}

		std::string formatTimestampTz(FirebirdStlStatusWrapper* statusWrapper, const ISC_TIMESTAMP_TZ& timeStampTz)
		{
			unsigned year, month, day, hours, minutes, seconds, fractions;

			util->decodeTimeStampTz(
				statusWrapper, &timeStampTz, &year, &month, &day, &hours, &minutes, &seconds, &fractions, 0, nullptr);

			return std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}.{:03}Z", year, month, day, hours, minutes, seconds,
				fractions / 10);
		}

	private:
		void* handle = nullptr;
		fb::IMaster* master = nullptr;
		fb::IUtil* util = nullptr;
		FbRef<fb::IProvider> dispatcher;
	};
}  // namespace rinhaback::api
