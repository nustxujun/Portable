#include "Network.h"

Network::Network()
{
	mContext = decltype(mContext)(new asio::io_context());
	mSocket = decltype(mSocket)(new asio::ip::tcp::socket(*mContext));
}

Network::~Network()
{

}


void Network::connect(const std::string& ip , USHORT port,Callback callback)
{
	asio::ip::tcp::endpoint ep = { asio::ip::address::from_string(ip), port };
	asio::async_connect(*mSocket, ep, [](const asio::error_code& ec,
		asio::ip::tcp::resolver::iterator iterator)
	{
	});
}

