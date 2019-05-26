#pragma once

#include <string>
#include <vector>
#include <map>
#include "json.hpp"
#include <set>
#include <memory>
class Setting
{
	friend class Modifier;
public:
	using Ptr = std::shared_ptr<Setting>;
	struct Modifier
	{
		friend class Setting;
	public:
		~Modifier() {
			if (mSetting)
				mSetting->mModifiers.erase(this);
		}

		template<class T>
		void setValue(const std::string& key, const T& value)
		{
			if (!mSetting) return;

			mSetting->set(key, { {"value", value} });
			mSetting->notify(key, this);
		}

		void set(const std::string& key, const nlohmann::json& j)
		{
			if (!mSetting) return;

			mSetting->set(key, j);
			mSetting->notify(key, this);
		}

		template<class T>
		T getValue(const std::string& key)
		{
			if (!mSetting) 
				return T(0);

			auto& j = mSetting->get(key);
			if (j.type() == nlohmann::json::value_t::object)
				return j["value"];
			else
				return T(0);
		}

		bool has(const std::string& key)
		{
			return mSetting->mValues.find(key) != mSetting->mValues.end();
		}

		const nlohmann::json& get(const std::string& key)
		{
			if (!mSetting)
				return {};
			return mSetting->get(key);
		}

		void setSetting(Setting::Ptr s)
		{
			mSetting = s;
			s->mModifiers.insert(this);
		}

		virtual void onChanged(const std::string& key, const nlohmann::json::value_type& value) = 0;
	
	private:
		Setting::Ptr mSetting ;
	};
public:
private:
	//template<class A, class B, class C, class D>
	//void set(const std::string& key, const A& value, const B& min, const C& max,const D& interval)
	//{
	//	auto& j = mValues[key];
	//	j["value"] = value;
	//	j["min"] = min;
	//	j["max"] = max;
	//	j["interval"] = interval;
	//}

	//template<class T>
	//void set(const std::string& key, const T& value)
	//{
	//	auto& j = mValues[key];
	//	j["value"] = value;
	//}

	void set(const std::string& key, const nlohmann::json& j)
	{
		auto i = j.begin();
		auto endi = j.end();

		auto& v = mValues[key];

		for (; i != endi; ++i)
			v[i.key()] = i.value();
	}


	const nlohmann::json& get(const std::string& key)
	{
		return mValues[key];
	}

	void notify(const std::string& key,  Modifier* m)
	{
		auto value = mValues[key];
		for (auto& i : mModifiers)
			if (i != m)
				i->onChanged(key, value);
	}

private:
	std::map<std::string,nlohmann::json> mValues;
	std::set<Modifier*> mModifiers;
};