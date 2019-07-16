#pragma once

#include <map>
#include <vector>
#include <functional>
#include <set>
#include <string>
#include <memory>
#include <algorithm>
#include <bitset>
#include <array>
#include <unordered_map>
#include <sstream>

#include "d3dx11effect.h"
#include <D3D11.h>
#include <D3DX11.h>

#ifdef USING_D3D11_3
#include <d3d11_3.h>
#endif


#define ALIGN16 __declspec(align(16))
#define ALIGN(x,y) (((x + y - 1) & ~(y - 1)) )

using Parameters = std::map<std::string, std::string>;

template<class Container>
class IteratorWrapper
{
public:
	using Iter = typename Container::iterator;


	IteratorWrapper(Iter beg, Iter end):
		mBegin(beg), mEnd(end)
	{
	}

	IteratorWrapper(Container& c)
	{
		mBegin = c.begin();
		mEnd = c.end();
	}

	Iter begin()const
	{
		return mBegin;
	}

	Iter end()const
	{
		return mEnd;
	}


	operator Iter()
	{
		return mBegin;
	}

private:
	Iter mBegin;
	Iter mEnd;
};
class Common
{
public:
	template<class T>
	static size_t hash(const T& value)
	{
		return hash(&value, sizeof(T));
	}

	static size_t hash(const void* data, size_t size)
	{
		std::string d((const char*)data, size);
		std::hash<std::string> hash;
		return hash(d);
	}

	static size_t hash(const std::string& str)
	{
		return hash(str.c_str(), str.size());
	}

	template<class T, class ... Args>
	static std::string format(const T& v, const Args& ... args)
	{
		std::stringstream ss;
		ss << v;
		ss << format(args...);
		return ss.str();
	}

private:
	static std::string format()
	{
		return {};
	}
};


#include "SimpleMath.h"
struct Vector3A : public DirectX::SimpleMath::Vector3
{
	template<class ... Args>
	Vector3A(Args ... args) :Vector3(args...) {}
private:
	float unused;
};
