#pragma once

#include "./FbUtil.h"
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stack>
#include "firebird/Interface.h"
#include "firebird/Message.h"


namespace rinhaback::api
{
	class Connection final
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
		explicit Connection();
		~Connection();

		Connection(const Connection&) = delete;
		Connection& operator=(const Connection&) = delete;

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

	using ConnectionHolder = std::unique_ptr<Connection, std::function<void(Connection*)>>;

	class ConnectionPool final
	{
	public:
		explicit ConnectionPool();

		ConnectionPool(const ConnectionPool&) = delete;
		ConnectionPool& operator=(const ConnectionPool&) = delete;

		ConnectionHolder getConnection();

	private:
		void releaseConnection(Connection* connection);

	private:
		std::mutex connectionsMutex;
		std::condition_variable connectionsCondVar;
		std::stack<std::unique_ptr<Connection>> connections;
	};
}  // namespace rinhaback::api
