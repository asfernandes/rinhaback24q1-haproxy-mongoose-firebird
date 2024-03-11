#include "./BankService.h"
#include "./Config.h"
#include "./Database.h"
#include "./Util.h"
#include <string>
#include <string_view>

using std::string;
using std::string_view;


namespace rinhaback::api
{
	int BankService::postTransaction(
		PostTransactionResponse* response, int accountId, int value, string_view description)
	{
		auto& postTransactionInMsg = databaseConnection.postTransactionInMsg;
		auto postTransactionInData = postTransactionInMsg.getData();
		postTransactionInData->accountIdNull = FB_FALSE;
		postTransactionInData->accountId = accountId;
		postTransactionInData->valNull = FB_FALSE;
		postTransactionInData->val = value;
		postTransactionInData->descriptionNull = FB_FALSE;
		postTransactionInData->description.set(&description.front(), description.length());

		auto& postTransactionOutMsg = databaseConnection.postTransactionOutMsg;
		auto postTransactionOutData = postTransactionOutMsg.getData();

		databaseConnection.postTransactionStmt->execute(&databaseConnection.statusWrapper,
			databaseConnection.transaction.get(), postTransactionInMsg.getMetadata(), postTransactionInData,
			postTransactionOutMsg.getMetadata(), postTransactionOutData);

		if (!postTransactionOutData->statusCodeNull && postTransactionOutData->statusCode != HTTP_STATUS_OK)
			return postTransactionOutData->statusCode;

		response->balance = postTransactionOutData->balance;
		response->overdraft = -postTransactionOutData->overdraft;

		return HTTP_STATUS_OK;
	}

	int BankService::getStatement(GetStatementResponse* response, int accountId)
	{
		auto& getTransactionsInMsg = databaseConnection.getTransactionsInMsg;
		auto getTransactionsInData = getTransactionsInMsg.getData();
		getTransactionsInData->accountIdNull = FB_FALSE;
		getTransactionsInData->accountId = accountId;

		auto& getTransactionsOutMsg = databaseConnection.getTransactionsOutMsg;
		auto getTransactionsOutData = getTransactionsOutMsg.getData();

		FbRef<fb::IResultSet> cursor(databaseConnection.getTransactionsStmt->openCursor(
			&databaseConnection.statusWrapper, databaseConnection.transaction.get(), getTransactionsInMsg.getMetadata(),
			getTransactionsInData, getTransactionsOutMsg.getMetadata(), 0));

		if (cursor->fetchNext(&databaseConnection.statusWrapper, getTransactionsOutData) != fb::IStatus::RESULT_OK)
		{
			cursor->addRef();
			cursor->close(&databaseConnection.statusWrapper);

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
						&databaseConnection.statusWrapper, getTransactionsOutData->datetime));

				if (cursor->fetchNext(&databaseConnection.statusWrapper, getTransactionsOutData) !=
					fb::IStatus::RESULT_OK)
				{
					break;
				}
			} while (getTransactionsOutData->val);
		}

		cursor->addRef();
		cursor->close(&databaseConnection.statusWrapper);

		return HTTP_STATUS_OK;
	}
}  // namespace rinhaback::api
