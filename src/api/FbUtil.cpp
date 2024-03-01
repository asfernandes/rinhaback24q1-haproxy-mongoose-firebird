#include "./FbUtil.h"
#include <exception>
#include <string>
#include "firebird/Interface.h"

using std::string;


namespace rinhaback::api
{
	void FirebirdStlException::saveMessage()
	{
		char textBuffer[1024];
		const auto textLen =
			FirebirdClient::getInstance().getUtil()->formatStatus(textBuffer, sizeof(textBuffer), status);
		textMessage = string(textBuffer, textLen);
	}


	FirebirdClient::FirebirdClient()
	{
		handle = dlopen("libfbclient.so", RTLD_NOW);
		if (!handle)
			throw std::runtime_error(dlerror());

		const auto masterFunc =
			reinterpret_cast<decltype(fb::fb_get_master_interface)*>(dlsym(handle, "fb_get_master_interface"));

		master = masterFunc();
		util = master->getUtilInterface();
		dispatcher.reset(master->getDispatcher());
	}

	FirebirdClient::~FirebirdClient()
	{
		if (dispatcher)
		{
			const auto status = newStatus();
			FirebirdStlStatusWrapper statusWrapper(status.get());
			dispatcher->shutdown(&statusWrapper, 0, fb_shutrsn_app_stopped);
			dispatcher.reset();
		}

		if (handle)
			dlclose(handle);
	}
}  // namespace rinhaback::api
