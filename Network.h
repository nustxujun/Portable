#pragma once

#include "asio.hpp"
#include <functional>

class Network
{
	template<class T>
	using Unique = std::unique_ptr<T>;
public:
	using Ptr = std::shared_ptr<Network>;
	using Callback = std::function<void(const asio::error_code&)>;

	Network();
	~Network();

	void connect(const std::string& ip, USHORT port, Callback callback);
private:
	Unique<asio::io_context> mContext;
	Unique<asio::ip::tcp::socket> mSocket;
	

};