/*
MIT License

Copyright(c) 2022 Lukas Lipp

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <string>
#include <array>
#include <functional>
#include <sstream>
#include <deque>
#include <optional>

class ccli
{
public:
	static void						parseArgs(int aArgc, const char* const aArgv[]);
	static void						loadConfig(const std::string& aCfgFile);
	static void						writeConfig(const std::string& aCfgFile);
	static void						executeCallbacks();
	static std::deque<std::string>	getHelp();
	static std::deque<std::string>  checkErrors();

	enum Flag {
		NONE						= (0 << 0),
		CLI_ONLY					= (1 << 0),	// can only be set through parseArgs
		READ_ONLY					= (1 << 1),	// display only, cannot be changed at all
		CONFIG_RD					= (1 << 2),	// load variable from config file
		CONFIG_RDWR					= (3 << 2),	// load variable from config file and save changes back to config file
		MANUAL_EXEC					= (1 << 4)	// execute callback only when executeCallback/executeCallbacks is called
	};

	class var_base
	{
	public:
									var_base(std::string aShortName, std::string aLongName, uint32_t aFlags, 
												std::string aDescription, bool aHasCallback);
		virtual						~var_base();
									
									var_base(const var_base&) = delete;
									var_base(var_base&&) = delete;
		var_base&					operator=(const var_base&) = delete;
		var_base&					operator=(var_base&&) = delete;

		const std::string&			getLongName() const;
		const std::string&			getShortName() const;
		const std::string&			getDescription() const;

		virtual std::string			getValueString() = 0;
		virtual void				setValueString(const std::string& aString) = 0;

		bool						hasCallback() const;
		virtual bool				executeCallback() = 0;

		virtual size_t				size() const = 0;

		bool						isCliOnly() const;
		bool						isReadOnly() const;
		bool						isConfigRead() const;
		bool						isConfigReadWrite() const;
		bool						isCallbackAutoExecuted() const;

		virtual bool				isBool() const = 0;
		virtual bool				isInt() const = 0;
		virtual bool				isFloat() const = 0;
		virtual bool				isString() const = 0;

		bool						locked() const;
		void						locked(bool aLocked);

	protected:
		virtual void				setValueStringInternal(const std::string& aString) = 0;

		friend class ccli;
		const std::string			mShortName;
		const std::string			mLongName;
		const std::string			mDescription;
		const uint32_t				mFlags;
		const bool					mHasCallback;
		bool						mLocked;
	};

	template<typename T, size_t>
	struct NoLimits {
		void order() {}
		T limit(size_t, T x) const { return x; }

		static constexpr bool hasLimits = false;
	};

	template<typename TData, size_t S>
	struct Limits {
		Limits(const std::array<TData, S >& left, const std::array<TData, S >& right) : mValues{ left, right } {}

		void order() {
			for (uint32_t i = 0; i < S; i++) {
				if (mValues.first[i] > mValues.second[i]) {
					std::swap(mValues.first[i], mValues.second[i]);
				}
			}
		}

		TData limit(size_t idx, TData x) const {
			if (x < mValues.first[idx]) {
				return mValues.first[idx];
			}
			if (x > mValues.second[idx]) {
				return mValues.second[idx];
			}
			return x;
		}

		static constexpr bool hasLimits = true;
	private:
		std::pair<std::array<TData, S >, std::array<TData, S >> mValues;
	};

	template <
		typename TData,
		size_t S = 1,
		template<typename, size_t> class TLimits= NoLimits
	>
	class var final : public var_base {
	public:
		using TLimitsData = TLimits<TData, S>;
		static_assert(std::is_same_v<TData, int> || std::is_same_v<TData, float> || std::is_same_v<TData, bool> || std::is_same_v<TData, std::string>, "Type must be bool, int, float or string");
		static_assert(S >= 1, "Size must be larger 0");
		static_assert(!std::is_same_v<TData, std::string> || !TLimitsData::hasLimits, "String value may not have limits");

							var(const std::string& aShortName, const std::string& aLongName, const std::array<TData, S>& aValue = {}, 
								uint32_t aFlags = NONE, const std::string& aDescription = {},
								const std::function<void(const std::array<TData, S>&)> aCallback = {})
									: var_base(aShortName, aLongName, aFlags, aDescription, aCallback != nullptr),
									mCallback(aCallback), mCallbackCharged(false), mValue(aValue) {}

							template <typename = std::enable_if_t<TLimitsData::hasLimits>>
							var(const std::string& aShortName, const std::string& aLongName, const TLimitsData& aLimits, 
								const std::array<TData, S>& aValue = {}, uint32_t aFlags = NONE,
								const std::string& aDescription = {}, const std::function<void(const std::array<TData, S>&)> aCallback = {})
								: var_base(aShortName, aLongName, aFlags, aDescription, aCallback != nullptr),
								mCallback(aCallback), mCallbackCharged(false), mValue(aValue), mLimits(aLimits)
							{
								mLimits.order();
							}
							~var() override = default;

							var(const var&) = delete;
							var(var&&) = delete;
		var&				operator=(const var&) = delete;
		var&				operator=(var&&) = delete;

		auto&				getValue() { return mValue; }
		void				setValue(const std::array<TData, S>& aValue)
							{
								if (isCliOnly()) return;
								setValueInternal(aValue);
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

		size_t				size()const override { return mValue.size(); }

		bool				isCallbackCharged() const { return mCallbackCharged; }

		bool				isBool() const override { return std::is_same_v<TData, bool>; }
		bool				isInt() const override { return std::is_same_v<TData, int>; }
		bool				isFloat() const override { return std::is_same_v<TData, float>; }
		bool				isString() const override { return std::is_same_v<TData, std::string>; }

		void				setValueString(const std::string& aString) override
							{
								if (isCliOnly()) return;
								setValueStringInternal(aString);
							}
		std::string			getValueString() override
							{
								std::string s;
								for(const TData& v : mValue)
								{
									if (!s.empty()) s += mDelimiter;
									if constexpr (std::is_same_v<TData, std::string>) s += v;
									else s += std::to_string(v);
								}
								return s;
							}
	private:
		void				setValueInternal(const std::array<TData, S>& aValue)
							{
								if (mLocked || isReadOnly()) return;
								mValue = aValue;
								if (hasCallback()) {
									mCallbackCharged = true;
									if (isCallbackAutoExecuted()) executeCallback();
								}
							}
		void				setValueStringInternal(const std::string& aString) override
							{
								if (mLocked || isReadOnly()) return;
								// empty string only allowed for bool and string
								if constexpr (!std::is_same_v<TData, bool> && !std::is_same_v<TData, std::string>) if (aString.empty()) return;

								size_t count = 0;
								size_t current = 0;
								size_t pos;
								do {
									pos = aString.find(mDelimiter, current);
									std::string token = aString.substr(current, pos - current);
									current = pos + 1;

									if (count > mValue.size() - 1) break;

									if constexpr (std::is_same_v<TData, float>) mValue.at(count) = std::stof(token);
									else if constexpr (std::is_same_v<TData, int>) mValue.at(count) = std::stoi(token);
									else if constexpr (std::is_same_v<TData, std::string>) mValue.at(count) = token;
									else if constexpr (std::is_same_v<TData, bool>) {
										if (token.empty()) mValue.at(count) = true;
										else {
											bool bn, bs;
											std::istringstream(token) >> bn;
											std::istringstream(token) >> std::boolalpha >> bs;
											mValue[count] = bn || bs;
										}
									}
									count++;
								} while (pos != std::string::npos);

								// check limits
								if constexpr (!std::is_same_v<TData, std::string>) {
									for (uint32_t i = 0; i < size(); i++)
									{
										mValue[i] = mLimits.limit(i, mValue[i]);
									}
								}

								mCallbackCharged = true;
								if (isCallbackAutoExecuted()) executeCallback();
							}

		static constexpr char*	mDelimiter = ",";
		const std::function<void(const std::array<TData, S>&)> mCallback;
		bool				mCallbackCharged;
		std::array<TData, S>	mValue;
		TLimitsData	mLimits;
	};
};
