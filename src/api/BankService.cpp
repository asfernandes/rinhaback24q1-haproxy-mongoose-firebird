#include "./BankService.h"
#include "./Config.h"
#include "./Database.h"
#include "./Util.h"
#include <string>
#include <string_view>
#include <experimental/scope>

using std::string;
using std::string_view;


namespace rinhaback::api
{
	static ConnectionPool connectionPool;

	int BankService::postTransaction(
		PostTransactionResponse* response, int accountId, int value, string_view description)
	{
		auto connectionHolder = connectionPool.getConnection();

		auto& postTransactionInMsg = connectionHolder->postTransactionInMsg;
		auto postTransactionInData = postTransactionInMsg.getData();
		postTransactionInData->accountIdNull = FB_FALSE;
		postTransactionInData->accountId = accountId;
		postTransactionInData->valNull = FB_FALSE;
		postTransactionInData->val = value;
		postTransactionInData->descriptionNull = FB_FALSE;
		postTransactionInData->description.set(&description.front(), description.length());

		auto& postTransactionOutMsg = connectionHolder->postTransactionOutMsg;
		auto postTransactionOutData = postTransactionOutMsg.getData();

		connectionHolder->postTransactionStmt->execute(&connectionHolder->statusWrapper,
			connectionHolder->transaction.get(), postTransactionInMsg.getMetadata(), postTransactionInData,
			postTransactionOutMsg.getMetadata(), postTransactionOutData);

		if (postTransactionOutData->statusCode != HTTP_STATUS_OK)
			return postTransactionOutData->statusCode;

		response->balance = postTransactionOutData->balance;
		response->overdraft = -postTransactionOutData->overdraft;

		return HTTP_STATUS_OK;
	}

	int BankService::getStatement(GetStatementResponse* response, int accountId)
	{
		auto connectionHolder = connectionPool.getConnection();

		auto& getTransactionsInMsg = connectionHolder->getTransactionsInMsg;
		auto getTransactionsInData = getTransactionsInMsg.getData();
		getTransactionsInData->accountIdNull = FB_FALSE;
		getTransactionsInData->accountId = accountId;

		auto& getTransactionsOutMsg = connectionHolder->getTransactionsOutMsg;
		auto getTransactionsOutData = getTransactionsOutMsg.getData();

		FbRef<fb::IResultSet> cursor(connectionHolder->getTransactionsStmt->openCursor(&connectionHolder->statusWrapper,
			connectionHolder->transaction.get(), getTransactionsInMsg.getMetadata(), getTransactionsInData,
			getTransactionsOutMsg.getMetadata(), 0));

		if (cursor->fetchNext(&connectionHolder->statusWrapper, getTransactionsOutData) != fb::IStatus::RESULT_OK)
		{
			cursor->addRef();
			cursor->close(&connectionHolder->statusWrapper);

			return HTTP_STATUS_NOT_FOUND;
		}

		response->balance = getTransactionsOutData->balance;
		response->overdraft = -getTransactionsOutData->overdraft;
		response->date = getCurrentDateTimeAsString();

		if (getTransactionsOutData->val)
		{
			response->lastTransactions.reserve(10);

			do
			{
				response->lastTransactions.emplace_back(getTransactionsOutData->val,
					string(getTransactionsOutData->description.str, getTransactionsOutData->description.length),
					FirebirdClient::getInstance().formatTimestampTz(
						&connectionHolder->statusWrapper, getTransactionsOutData->datetime));

				if (cursor->fetchNext(&connectionHolder->statusWrapper, getTransactionsOutData) !=
					fb::IStatus::RESULT_OK)
				{
					break;
				}
			} while (getTransactionsOutData->val);
		}

		cursor->addRef();
		cursor->close(&connectionHolder->statusWrapper);

		return HTTP_STATUS_OK;
	}
}  // namespace rinhaback::api
