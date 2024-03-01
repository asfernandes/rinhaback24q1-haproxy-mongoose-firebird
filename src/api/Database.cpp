#include "./Database.h"
#include "./Config.h"
#include <mutex>


namespace rinhaback::api
{
	Connection::Connection()
		: status(FirebirdClient::getInstance().newStatus()),
		  statusWrapper(status.get()),
		  postTransactionInMsg(&statusWrapper, FirebirdClient::getInstance().getMaster()),
		  postTransactionOutMsg(&statusWrapper, FirebirdClient::getInstance().getMaster()),
		  getTransactionsInMsg(&statusWrapper, FirebirdClient::getInstance().getMaster()),
		  getTransactionsOutMsg(&statusWrapper, FirebirdClient::getInstance().getMaster())
	{
		const auto dpb = fbUnique(
			FirebirdClient::getInstance().getUtil()->getXpbBuilder(&statusWrapper, fb::IXpbBuilder::DPB, nullptr, 0));
		dpb->insertTag(&statusWrapper, isc_dpb_no_garbage_collect);

		attachment.reset(FirebirdClient::getInstance().getDispatcher()->attachDatabase(&statusWrapper,
			Config::fbDatabase.c_str(), dpb->getBufferLength(&statusWrapper), dpb->getBuffer(&statusWrapper)));

		startTransaction();

		postTransactionStmt.reset(attachment->prepare(&statusWrapper, transaction.get(), 0,
			R"""(
			execute procedure post_transaction(?, ?, ?)
			)""",
			SQL_DIALECT_CURRENT, 0));

		getTransactionsStmt.reset(attachment->prepare(&statusWrapper, transaction.get(), 0,
			R"""(
			select balance,
			       overdraft,
			       val,
			       description,
			       datetime
			    from transaction
			    where account_id = ?
			    order by seq desc
			    rows 10
			)""",
			SQL_DIALECT_CURRENT, 0));
	}

	Connection::~Connection()
	{
		if (transaction)
		{
			transaction->addRef();
			transaction->rollback(&statusWrapper);
		}

		if (attachment)
		{
			attachment->addRef();
			attachment->detach(&statusWrapper);
		}
	}

	void Connection::startTransaction()
	{
		static constexpr uint8_t TPB[] = {
			isc_tpb_version3,
			isc_tpb_read_committed,
			isc_tpb_read_consistency,
			isc_tpb_nowait,
		};

		if (transaction)
		{
			transaction->addRef();
			transaction->rollback(&statusWrapper);
		}

		transaction.reset(attachment->startTransaction(&statusWrapper, sizeof(TPB), TPB));
	}


	ConnectionPool::ConnectionPool()
	{
		for (unsigned i = 0; i < Config::dbWorkers; ++i)
			connections.emplace(std::make_unique<Connection>());
	}

	ConnectionHolder ConnectionPool::getConnection()
	{
		std::unique_lock lock(connectionsMutex);

		connectionsCondVar.wait(lock, [this] { return !connections.empty(); });
		assert(!connections.empty());

		auto connection = std::move(connections.top());
		connections.pop();

		lock.unlock();

		return {connection.release(), [this](auto connection) { releaseConnection(connection); }};
	}

	void ConnectionPool::releaseConnection(Connection* connection)
	{
		std::unique_lock lock(connectionsMutex);
		connections.emplace(connection);

		lock.unlock();
		connectionsCondVar.notify_one();
	}
}  // namespace rinhaback::api
