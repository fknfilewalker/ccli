/*
MIT License

Copyright(c) 2022 Lukas Lipp, Matthias Preymann

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
	struct IterationDecision
	{
		enum DecisionType { Continue, Break };

		IterationDecision() = default;

		IterationDecision(const DecisionType d) : decision{ d }
		{
		}

		bool operator==(const DecisionType d) const { return decision == d; }
		const DecisionType decision{ Continue };
	};

	class VarBase;
	static void parseArgs(int argc, const char* const argv[]);
	static void loadConfig(const std::string& cfgFile);
	static void writeConfig(const std::string& cfgFile);
	static void executeCallbacks();
	static std::deque<std::string> checkErrors();
	static IterationDecision forEachVar(const std::function<IterationDecision(VarBase&, size_t)>&);

	enum Flag
	{
		NONE		= (0 << 0),
		CLI_ONLY	= (1 << 0),	// can only be set through parseArgs
		READ_ONLY	= (1 << 1),	// display only, cannot be changed at all
		CONFIG_RD	= (1 << 2),	// load variable from config file
		CONFIG_RDWR = (3 << 2),	// load variable from config file and save changes back to config file
		MANUAL_EXEC = (1 << 4)	// execute callback only when executeCallback/executeCallbacks is called
	};

	class VarBase
	{
	public:
		VarBase(std::string_view shortName, std::string_view longName, uint32_t flags,
		         std::string_view description, bool hasCallback);
		virtual ~VarBase();

		VarBase(const VarBase&) = delete;
		VarBase(VarBase&&) = delete;
		VarBase& operator=(const VarBase&) = delete;
		VarBase& operator=(VarBase&&) = delete;

		[[nodiscard]] const std::string& getLongName() const;
		[[nodiscard]] const std::string& getShortName() const;
		[[nodiscard]] const std::string& getDescription() const;

		virtual std::string getValueString() = 0;
		virtual void setValueString(std::string_view aString) = 0;

		[[nodiscard]] bool hasCallback() const;
		virtual bool executeCallback() = 0;

		[[nodiscard]] virtual size_t size() const = 0;

		[[nodiscard]] bool isCliOnly() const;
		[[nodiscard]] bool isReadOnly() const;
		[[nodiscard]] bool isConfigRead() const;
		[[nodiscard]] bool isConfigReadWrite() const;
		[[nodiscard]] bool isCallbackAutoExecuted() const;

		[[nodiscard]] virtual bool isBool() const = 0;
		[[nodiscard]] virtual bool isInt() const = 0;
		[[nodiscard]] virtual bool isFloat() const = 0;
		[[nodiscard]] virtual bool isString() const = 0;

		[[nodiscard]] virtual std::optional<bool> asBool(size_t = 0) const = 0;
		[[nodiscard]] virtual std::optional<long long> asInt(size_t = 0) const = 0;
		[[nodiscard]] virtual std::optional<double> asFloat(size_t = 0) const = 0;
		[[nodiscard]] virtual std::optional<std::string_view> asString(size_t = 0) const = 0;

		[[nodiscard]] bool locked() const;
		void locked(bool aLocked);

	protected:
		virtual void setValueStringInternal(std::string_view aString) = 0;

		static long long parseIntegral(std::string_view);
		static double parseDouble(std::string_view);
		static bool parseBool(std::string_view);

		friend class ccli;
		const std::string _shortName;
		const std::string _longName;
		const std::string _description;
		const uint32_t _flags;
		const bool _hasCallback;
		bool _locked;
	};

	template <auto Value>
	struct MaxLimit
	{
		template <typename T>
		static T apply(T x) { return x > static_cast<T>(Value) ? static_cast<T>(Value) : x; }
	};

	template <auto Value>
	struct MinLimit
	{
		template <typename T>
		static T apply(T x) { return x < static_cast<T>(Value) ? static_cast<T>(Value) : x; }
	};


	template <typename TData, size_t S>
	struct VariableSizedData
	{
		static_assert(S >= 1, "Size must be larger 0");
		std::array<TData, S> mData;

		static constexpr auto size() { return S; }
		auto& at(size_t idx) { return mData.at(idx); }
		const auto& at(size_t idx) const { return mData.at(idx); }
		auto begin() { return mData.begin(); }
		auto end() { return mData.end(); }
		const std::array<TData, S>& asArray() const { return mData; }
	};

	template <typename TData>
	struct VariableSizedData<TData, 1>
	{
		VariableSizedData() = default;

		template <typename U, typename = std::enable_if_t<std::is_same_v<TData, std::string>>>
		VariableSizedData(const U& d) : mData{d} {}
		VariableSizedData(const TData& d) : mData{d} {}

		TData mData;

		static constexpr auto size() { return 1; }
		auto& at(size_t idx) { return mData; }
		const auto& at(size_t idx) const { return mData; }
		auto* begin() { return &mData; }
		auto* end() { return &mData + 1; }
		std::array<TData, 1> asArray() const { return { mData }; }
	};

	template <
		typename TData,
		size_t S = 1,
		typename... TLimits
	>
	class Var final : public VarBase
	{
	public:
		using TStorage = VariableSizedData<TData, S>;
		static_assert(std::disjunction_v<std::is_integral<TData>, std::is_floating_point<TData>, std::is_same<TData, std::string>>
			, "Type must be integral, floating-point or string");
		static_assert((!std::is_same_v<TData, std::string> && !std::is_same_v<TData, bool>) || sizeof...(TLimits) == 0,
			"String and boolean values may not have limits");

		Var(const std::string_view shortName, const std::string_view longName, const TStorage& value = {},
		    const uint32_t flags = NONE, const std::string_view description = {},
		    const std::function<void(const std::array<TData, S>&)> callback = {})
			: VarBase(shortName, longName, flags, description, callback != nullptr),
				_callback{ callback }, _callbackCharged{ false }, _value{ value } {}

		~Var() override = default;
		Var(const Var&) = delete;
		Var(Var&&) = delete;
		Var& operator=(const Var&) = delete;
		Var& operator=(Var&&) = delete;

		auto& getValue() { return _value.mData; }

		void setValue(const TStorage& aValue)
		{
			if (isCliOnly()) return;
			setValueInternal(aValue);
		}

		void chargeCallback() { _callbackCharged = true; }
		bool executeCallback() override
		{
			if (hasCallback() && _callbackCharged)
			{
				_callback(_value.asArray());
				_callbackCharged = false;
				return true;
			}
			return false;
		}

		[[nodiscard]] size_t size() const override { return _value.size(); }
		[[nodiscard]] bool isCallbackCharged() const { return _callbackCharged; }

		[[nodiscard]] bool isBool() const override { return std::is_same_v<TData, bool>; }
		[[nodiscard]] bool isInt() const override { return std::is_integral_v<TData>; }
		[[nodiscard]] bool isFloat() const override { return std::is_floating_point_v<TData>; }
		[[nodiscard]] bool isString() const override { return std::is_same_v<TData, std::string>; }

		[[nodiscard]] std::optional<bool> asBool(size_t idx= 0) const override
		{
			if constexpr (std::is_same_v<TData, std::string>)
			{
				return {};
			}
			else
			{
				return { static_cast<bool>(_value.at(idx)) };
			}
		}

		[[nodiscard]] std::optional<long long> asInt(size_t idx= 0) const override
		{
			if constexpr (std::is_same_v<TData, std::string>)
			{
				return {};
			}
			else
			{
				return { static_cast<long long>(_value.at(idx)) };
			}
		}

		[[nodiscard]] std::optional<double> asFloat(size_t idx= 0) const override
		{
			if constexpr (std::is_same_v<TData, std::string>)
			{
				return {};
			}
			else
			{
				return { static_cast<double>(_value.at(idx)) };
			}
		}

		[[nodiscard]] std::optional<std::string_view> asString(size_t idx= 0) const override
		{
			if constexpr (std::is_same_v<TData, std::string>)
			{
				return { std::string_view{ _value.at(idx) } };
			}

			return {};
		}

		void setValueString(const std::string_view string) override
		{
			if (isCliOnly()) return;
			setValueStringInternal(string);
		}

		std::string getValueString() override
		{
			std::stringstream stream;
			for (size_t i = 0; i != _value.size(); i++)
			{
				if (i)
				{
					stream << _delimiter;
				}
				stream << _value.at(i);
			}
			return stream.str();
		}

	private:
		template <typename... TRest>
		struct LimitApplier
		{
			static auto apply(TData x)
			{
				return x;
			}
		};

		template <typename TLimit, typename... TRest>
		struct LimitApplier<TLimit, TRest...>
		{
			static auto apply(TData x)
			{
				return LimitApplier<TRest...>::apply(TLimit::apply(x));
			}
		};

		void setValueInternal(const TStorage& value)
		{
			if (_locked || isReadOnly()) return;
			_value = value;
			if (hasCallback())
			{
				_callbackCharged = true;
				if (isCallbackAutoExecuted()) executeCallback();
			}
		}

		void setValueStringInternal(std::string_view string) override
		{
			if (_locked || isReadOnly()) return;
			// empty string only allowed for bool and string
			if constexpr (!std::is_same_v<TData, bool> && !std::is_same_v<TData, std::string>) if (string.empty())
				return;

			size_t count = 0;
			size_t current = 0;
			size_t pos;
			do
			{
				pos = string.find(_delimiter, current);
				std::string_view token = string.substr(current, pos - current);
				current = pos + 1;

				if (count > _value.size() - 1) break;

				if constexpr (std::is_floating_point_v<TData>) _value.at(count) = static_cast<TData>(parseDouble(token));
				else if constexpr (std::is_same_v<TData, bool>) _value.at(count) = parseBool(token);
				else if constexpr (std::is_integral_v<TData>) _value.at(count) = static_cast<TData>(parseIntegral(token));
				else if constexpr (std::is_same_v<TData, std::string>) _value.at(count) = token;
				count++;
			}
			while (pos != std::string::npos);

			if constexpr (!std::is_same_v<TData, std::string>)
			{
				for (uint32_t i = 0; i < size(); i++)
				{
					_value.at(i) = LimitApplier<TLimits...>::apply(_value.at(i));
				}
			}

			_callbackCharged = true;
			if (isCallbackAutoExecuted()) executeCallback();
		}

		static constexpr const char* _delimiter = ",";
		const std::function<void(const std::array<TData, S>&)> _callback;
		bool _callbackCharged;
		TStorage _value;
	};
};
