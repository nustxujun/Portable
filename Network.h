#pragma once

#include "asio.hpp"
#include <functional>
#include <array>
#include "json.hpp"

using nlohmann::json;

class Network
{
	template<class T>
	using Unique = std::unique_ptr<T>;
public:
	using Ptr = std::shared_ptr<Network>;
	using Callback = std::function<void(const asio::error_code&)>;
	using Response = std::function<void(const char* buffer, size_t size)>;

	Network();
	~Network();

	void connect(const std::string& ip, USHORT port, Callback callback);
	void disconnect();
	void send(const char* data, unsigned int size, Response rep = {});
	void send(const std::string& msg, Response rep = {});
	void send(const json& jsonobj, Response rep = {});
	void update();

	void setListener(std::function<void(const char*, size_t)> f) { mListener = f; }
private:
	static void error(const asio::error_code& err);
	static void error(const std::string& err);

	void receive(const asio::error_code& err, size_t size);
	void dispatch(const char* data, size_t size);
private:
	Unique<asio::io_context> mContext;
	Unique<asio::ip::tcp::socket> mSocket;
	std::array<char, 2048> mReceiveBuffer;
	std::vector<char> mCache;

	size_t mLastSize = 4;
	bool mParseMsg = false;
	std::function<void(const char*, size_t)> mListener;
};