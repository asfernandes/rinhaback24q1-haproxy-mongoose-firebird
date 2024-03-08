#pragma once

#include "./FbUtil.h"
#include "firebird/Interface.h"
#include "firebird/Message.h"


namespace rinhaback::api
{
	class DatabaseConnection final
	{
	public:
		// clang-format off

		FB_MESSAGE(PostTransactionInMsg, FirebirdStlStatusWrapper,
			(FB_INTEGER, accountId)
			(FB_INTEGER, val)
			(FB_INTL_VARCHAR(10, CS_ASCII), description)
		);

		FB_MESSAGE(PostTransactionOutMsg, FirebirdStlStatusWrapper,
			(FB_INTEGER, statusCode)
			(FB_INTEGER, balance)
			(FB_INTEGER, overdraft)
		);

		FB_MESSAGE(GetTransactionsInMsg, FirebirdStlStatusWrapper,
			(FB_INTEGER, accountId)
		);

		FB_MESSAGE(GetTransactionsOutMsg, FirebirdStlStatusWrapper,
			(FB_INTEGER, balance)
			(FB_INTEGER, overdraft)
			(FB_INTEGER, val)
			(FB_INTL_VARCHAR(10, CS_ASCII), description)
			(FB_TIMESTAMP_TZ, datetime)
		);

		// clang-format on

	public:
		explicit DatabaseConnection();
		~DatabaseConnection();

		DatabaseConnection(const DatabaseConnection&) = delete;
		DatabaseConnection& operator=(const DatabaseConnection&) = delete;

	public:
		void ping() { }

	private:
		void startTransaction();

	public:
		FbUniquePtr<fb::IStatus> status;
		FirebirdStlStatusWrapper statusWrapper;
		FbRef<fb::IAttachment> attachment;
		FbRef<fb::ITransaction> transaction;
		FbRef<fb::IStatement> postTransactionStmt;
		PostTransactionInMsg postTransactionInMsg;
		PostTransactionOutMsg postTransactionOutMsg;
		FbRef<fb::IStatement> getTransactionsStmt;
		GetTransactionsInMsg getTransactionsInMsg;
		GetTransactionsOutMsg getTransactionsOutMsg;
	};

	inline thread_local DatabaseConnection databaseConnection;
}  // namespace rinhaback::api
