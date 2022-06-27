#pragma once

#include <string>
#include <cassert>
#include <map>
#include <set>
#include <utility>
#include <array>
#include <sstream>
#include <functional>
#include <iostream>

namespace ccli
{
	class log {
	public:
		static void warning(const std::string& aString) { funcWarning()(aString); }
		static std::function<void(const std::string&)> funcWarning(const std::function<void(const std::string&)>& aCallback = {})
		{
			static std::function<void(const std::string&)> func = [](const std::string& aString) { std::cout << aString << std::endl; };
			if (aCallback) func = aCallback;
			return func;
		}
	};
}

class var_base;
// #todo: use templates?
class var_system
{
public:
	// get static list containing all vars
	static std::map<std::string, var_base*>& getLongNameMap();
	static std::map<std::string, var_base*>& getShortNameMap();

	static var_base*			findVarByLongName(const std::string& aLongName);
	static var_base*			findVarByShortName(const std::string& aShortName);
	static void					executeCallbacks();

	static void					parseArgs(int aArgc, char* const aArgv[]);
private:
	friend var_base;

	static void					addToList(const std::string& aLongName, const std::string& aShortName, var_base* const aVar)
								{
									std::map<std::string, var_base*>& mapLong = getLongNameMap();
									std::map<std::string, var_base*>& mapShort = getShortNameMap();
									if (!aLongName.empty()) {
										const bool inserted = mapLong.insert(std::pair<std::string, var_base*>(aLongName, aVar)).second;
										if (!inserted) ccli::log::warning("Long identifier '--" + aLongName + "' already exists");
									}
									if (!aShortName.empty()) {
										const bool inserted = mapShort.insert(std::pair<std::string, var_base*>(aShortName, aVar)).second;
										if (!inserted) ccli::log::warning("Short identifier '-" + aShortName + "' already exists");
									}
								}
	static void					removeFromList(const std::string& aLongName, const std::string& aShortName, var_base* const aVar)
								{
									std::map<std::string, var_base*>& mapLong = getLongNameMap();
									std::map<std::string, var_base*>& mapShort = getShortNameMap();
									
									if (!aLongName.empty()) {
										const auto itr = mapLong.find(aLongName);
										if (itr != mapLong.end() && itr->second == aVar) mapLong.erase(itr);
									}
									if (!aShortName.empty()) {
										const auto itr = mapShort.find(aShortName);
										if (itr != mapShort.end() && itr->second == aVar) mapShort.erase(itr);
									}
								}

	static std::set<var_base*>&	getCallbackSet();
	static void					addToCallbackSet(var_base* aVar);
	static void					removeFromCallbackSet(var_base* aVar);
};

class var_base
{
public:
								var_base(std::string aLongName, std::string aShortName, std::string aDescription, const bool aHasCallback) :
								mLongName(std::move(aLongName)), mShortName(std::move(aShortName)),
								mDescription(std::move(aDescription)), mHasCallback(aHasCallback)
								{
									assert(!mLongName.empty() || !mShortName.empty());
									var_system::addToList(mLongName, mShortName, this);
									if(mHasCallback) var_system::addToCallbackSet(this);
								}

	virtual						~var_base()
								{
									var_system::removeFromList(mLongName, mShortName, this);
									if (mHasCallback) var_system::removeFromCallbackSet(this);
								}

	const std::string&			getLongName() const { return mLongName; }
	const std::string&			getShortName() const { return mShortName; }
	const std::string&			getDescription() const { return mDescription; }

	virtual void				setValueString(const std::string& aString) = 0;
	virtual std::string			getValueString() = 0;
	
	bool						hasCallback() const { return mHasCallback; }
	virtual bool				executeCallback() = 0;

	virtual bool				isConfigRead() const = 0;
	virtual bool				isConfigReadWrite() const = 0;

protected:

	const std::string			mLongName;
	const std::string			mShortName;
	const std::string			mDescription;
	const bool					mHasCallback;
};

enum VarFlag {
	NONE		= (0 << 0),
	INIT		= (1 << 0),		// can only be set from the command-line
	ROM			= (1 << 1),		// display only, cannot be set at all
	CONFIG_RD	= (1 << 2),		// load variable from config file
	CONFIG_RDWR = (3 << 2),		// load variable from config file and save changes back to config file when application is closed
	MANUAL_EXEC = (1 << 4)		// execute callback not immediately after value is set but when execute executeCallback/executeCallbacks is called
};

template <class T, size_t S = 1, uint32_t F = NONE>
class var final : public var_base {
	static_assert(std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, bool> || std::is_same_v<T, std::string>, "Type must be bool, int, float or string");
public:
						var(const std::string& aLongName, const std::string& aShortName, const std::string& aDescription, const std::array<T, S>& aValue = {}, std::function<void(const std::array<T, S>&)> aCallback = {})
							: var_base(aLongName, aShortName, aDescription, aCallback != nullptr), mValue(aValue), mCallbackCharged(false), mCallback(aCallback) {}

						~var() override = default;

	std::array<T, S>&	getValue() { return mValue; }
	void				setValue(const std::array<T, S>& aValue)
						{
							mValue = aValue; mCallbackCharged = true;
							if (isCbAutoExec()) executeCallback();
						}

	void				chargeCallback() { mCallbackCharged = true; }
	bool				executeCallback() override
						{
							if (hasCallback() && mCallbackCharged) {
								mCallback(mValue);
								mCallbackCharged = false;
								return true;
							}
							return false;
						}

	bool				isConfigRead() const override { return !mLongName.empty() && F & CONFIG_RD; }
	bool				isConfigReadWrite() const override { return !mLongName.empty() && F & CONFIG_RDWR; }
	bool				isCbAutoExec() const { return !(F & MANUAL_EXEC); }

	void				setValueString(const std::string& aString) override
						{
							if (mValue.size() == 0) return;

							size_t count = 0;
							size_t current = 0;
							size_t pos;
							do {
								pos = aString.find(mDelimiter, current);
								std::string token = aString.substr(current, pos - current);
								current = pos + 1;

								if (count > mValue.size() - 1) break;

								if constexpr (std::is_same_v<T, float>) mValue.at(count) = std::stof(token);
								else if constexpr (std::is_same_v<T, int>) mValue.at(count) = std::stoi(token);
								else if constexpr (std::is_same_v<T, std::string>) mValue.at(count) = token;
								else if constexpr (std::is_same_v<T, bool>) {
									if (token.empty()) mValue.at(count) = true;
									else {
										bool b;
										std::istringstream(token) >> std::boolalpha >> b;
										mValue.at(count) = b;
									}
								}
								count++;
							} while (pos != std::string::npos);
		
							mCallbackCharged = true;
							if (isCbAutoExec()) executeCallback();
						}
	std::string			getValueString() override
						{
							std::string s;
							for(const T& v : mValue)
							{
								if (!s.empty()) s += mDelimiter;
								if constexpr (std::is_same_v<T, std::string>) s += v;
								else s += std::to_string(v);
							}
							return s;
						}
private:

	const std::string	mDelimiter = ",";
	std::array<T, S>	mValue;
	bool				mCallbackCharged;
	std::function<void(const std::array<T, S>&)> mCallback;
};
