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
		void set(const std::string& key, const T& value)
		{
			if (!mSetting) return;

			mSetting->set(key, value);
			mSetting->notify(key, this);
		}

		template<class T>
		T get(const std::string& key)
		{
			if (!mSetting) return;

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
	const nlohmann::json& getAll() const { return mValues; }
private:
	template<class T>
	void set(const std::string& key, const T& value)
	{
		mValues[key] = value;
	}

	template<class T>
	T get(const std::string& key)
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
	nlohmann::json mValues;
	std::set<Modifier*> mModifiers;
};