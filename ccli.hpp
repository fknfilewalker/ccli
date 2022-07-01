#pragma once

#include <string>
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
	void parseArgs(int aArgc, char* const aArgv[]);
	void loadConfig(const std::string& aCfgFile);
	void writeConfig(const std::string& aCfgFile);
	void executeCallbacks();


	class var_base
	{
	public:
									var_base(std::string aLongName, std::string aShortName, std::string aDescription, const bool aHasCallback);

		virtual						~var_base();

		const std::string&			getLongName() const;
		const std::string&			getShortName() const;
		const std::string&			getDescription() const;

		virtual void				setValueString(const std::string& aString) = 0;
		virtual std::string			getValueString() = 0;
		
		bool						hasCallback() const;
		virtual bool				executeCallback() = 0;

		virtual bool				isConfigRead() const = 0;
		virtual bool				isConfigReadWrite() const = 0;

		bool						locked() const;
		void						locked(const bool aLocked);

	protected:
		const std::string			mLongName;
		const std::string			mShortName;
		const std::string			mDescription;
		const bool					mHasCallback;
		bool						mLocked;
	};


	enum Flag {
		NONE		= (0 << 0),
		INIT		= (1 << 0),		// can only be set from the command-line
		ROM			= (1 << 1),		// display only, cannot be set at all
		CONFIG_RD	= (1 << 2),		// allow loading variable from config file
		CONFIG_RDWR = (3 << 2),		// allow loading variable from config file and save changes back to config file when application is closed
		MANUAL_EXEC = (1 << 4)		// execute callback not immediately after value is set but when execute executeCallback/executeCallbacks is called
	};

	template <class T, size_t S = 1, uint32_t F = NONE>
	class var final : public var_base {
		static_assert(std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, bool> || std::is_same_v<T, std::string>, "Type must be bool, int, float or string");
	public:
							var(const std::string& aLongName, const std::string& aShortName, const std::string& aDescription, const std::array<T, S>& aValue = {}, const std::function<void(const std::array<T, S>&)> aCallback = {})
								: var_base(aLongName, aShortName, aDescription, aCallback != nullptr), mCallback(aCallback), mCallbackCharged(false), mValue(aValue) {}

							~var() override = default;

		std::array<T, S>&	getValue() { return mValue; }
		void				setValue(const std::array<T, S>& aValue)
							{
								if (mValue.size() == 0 || mLocked) return;
								mValue = aValue; mCallbackCharged = true;
								if (isCallbackAutoExecuted()) executeCallback();
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
		bool				isCallbackAutoExecuted() const { return !(F & MANUAL_EXEC); }
		bool				isCallbackCharged() const { return mCallbackCharged; }

		void				setValueString(const std::string& aString) override
							{
								if (mValue.size() == 0 || mLocked) return;

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
								if (isCallbackAutoExecuted()) executeCallback();
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
		const std::function<void(const std::array<T, S>&)> mCallback;
		bool				mCallbackCharged;
		std::array<T, S>	mValue;
	};
}
