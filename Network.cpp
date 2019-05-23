#include "Network.h"
Network::Network()
{
	mContext = decltype(mContext)(new asio::io_context());
	mSocket = decltype(mSocket)(new asio::ip::tcp::socket(*mContext));
}

Network::~Network()
{
	disconnect();
}

void Network::error(const asio::error_code& err)
{
	if (err)
		error(err.message());
}

void Network::error(const std::string& err)
{
	OutputDebugStringA((err + "\n").c_str());
	::MessageBoxA(NULL, err.c_str(), NULL, NULL);
}

void Network::connect(const std::string& ip , USHORT port,Callback callback)
{
	asio::ip::tcp::endpoint ep = { asio::ip::make_address(ip), port };
	mSocket->async_connect(ep, [=](const asio::error_code& err) {
		if (err)
		{
			error(err);
		}
		else
		{
			mSocket->async_read_some(asio::buffer(mReceiveBuffer), std::bind(&Network::receive, this,
				std::placeholders::_1,
				std::placeholders::_2));
		}
		if (callback)
			callback(err);
	});
}


void Network::disconnect()
{
	mSocket->close();
	mContext->stop();
}

void Network::send(const char * data, unsigned int size, Response rep)
{
	if (!mSocket->is_open())
		return;

	mSocket->async_write_some(asio::buffer(&size, 4), [](const asio::error_code& err, size_t size) {
		error(err);
	});
	mSocket->async_write_some(asio::buffer(data, size), [](const asio::error_code& err, size_t size) {
		error(err);
	});
}

void Network::send(const std::string& msg, Response rep )
{
	send(msg.data(), msg.size() , rep);
}

void Network::send(const json & jsonobj, Response rep )
{
	send(jsonobj.dump(),rep);
}


void Network::receive(const asio::error_code & err, size_t size)
{
	if (err)
	{
		error(err);
		disconnect();
	}
	else
	{
		const char* head = mReceiveBuffer.data();
		const char* tail = mReceiveBuffer.data() + size;

		while (head < tail)
		{
			size_t need = mLastSize - mCache.size();
			size_t read = std::min((size_t)(tail - head), need);
			for (size_t i = 0; i < read; ++i)
				mCache.push_back(*head++);

			size_t count = mCache.size();
			if (count == mLastSize)
			{
				if (mParseMsg )
				{
					dispatch(mCache.data(), mLastSize);
					mParseMsg = false;
					mLastSize = 4;
				}
				else
				{
					mLastSize = *((const unsigned int*)mCache.data());
					mParseMsg = true;
				}
					
				mCache.clear();
			}
			else if (count > mLastSize)
				abort();
		}

		mSocket->async_read_some(asio::buffer(mReceiveBuffer), std::bind(&Network::receive, this,
			std::placeholders::_1,
			std::placeholders::_2));
	}
}

void Network::dispatch(const char * data, size_t size)
{
	if (mListener)
		mListener(data, size);
}

void Network::update()
{
	mContext->poll();
}
