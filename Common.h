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

using Parameters = std::map<std::string, std::string>;

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
