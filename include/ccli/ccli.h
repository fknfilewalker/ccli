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
#include <optional>

class ccli
{
public:
	struct IterationDecision
	{
		enum DecisionType { Continue, Break };

		IterationDecision() = default;
		IterationDecision(const DecisionType d) : decision{ d } {}

		bool operator==(const DecisionType d) const { return decision == d; }
		const DecisionType decision{ Continue };
	};

	class VarBase;
	static void parseArgs(size_t argc, const char* const argv[]);
	static void loadConfig(const std::string& cfgFile);
	static void writeConfig(const std::string& cfgFile);
	static void executeCallbacks();
	static IterationDecision forEachVar(const std::function<IterationDecision(VarBase&, size_t)>&);

	enum Flag
	{
		NONE		= (0 << 0),
		READ_ONLY	= (1 << 0),	// display only, cannot be modified at all
		CLI_ONLY	= (1 << 1),	// can only be set through parseArgs
		LOCKED		= (1 << 2),	// this var is locked, cannot be modified until unlocked
		CONFIG_RD	= (1 << 3),	// load variable from config file
		CONFIG_RDWR = (3 << 3),	// load variable from config file and save changes back to config file
		MANUAL_EXEC = (1 << 5)	// execute callback only when executeCallback/executeCallbacks is called
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

		[[nodiscard]] const std::string& longName() const noexcept;
		[[nodiscard]] const std::string& shortName() const noexcept;
		[[nodiscard]] const std::string& description() const noexcept;

		virtual std::string valueString() = 0;
		virtual void valueString(std::string_view string) = 0;

		[[nodiscard]] bool hasCallback() const noexcept;
		virtual bool executeCallback() = 0;

		[[nodiscard]] virtual size_t size() const noexcept = 0;

		[[nodiscard]] bool isCliOnly() const noexcept;
		[[nodiscard]] bool isReadOnly() const noexcept;
		[[nodiscard]] bool isLocked() const noexcept;
		[[nodiscard]] bool isConfigRead() const noexcept;
		[[nodiscard]] bool isConfigReadWrite() const noexcept;
		[[nodiscard]] bool isCallbackAutoExecuted() const noexcept;

		[[nodiscard]] virtual bool isBool() const = 0;
		[[nodiscard]] virtual bool isIntegral() const = 0;
		[[nodiscard]] virtual bool isFloatingPoint() const = 0;
		[[nodiscard]] virtual bool isString() const = 0;

		[[nodiscard]] virtual std::optional<bool> asBool(size_t = 0) const = 0;
		[[nodiscard]] virtual std::optional<long long> asInt(size_t = 0) const = 0;
		[[nodiscard]] virtual std::optional<double> asFloat(size_t = 0) const = 0;
		[[nodiscard]] virtual std::optional<std::string_view> asString(size_t = 0) const = 0;

		void lock() noexcept;
		void unlock() noexcept;
		void locked(bool locked) noexcept;

	protected:
		virtual void setValueStringInternal(std::string_view string) = 0;

		static long long parseIntegral(const VarBase&, std::string_view);
		static double parseDouble(const VarBase&, std::string_view);
		static bool parseBool(std::string_view);

		friend class ccli;
		const std::string _shortName;
		const std::string _longName;
		const std::string _description;
		uint32_t _flags;
		const bool _hasCallback;
	};

	template <typename TData, size_t S = 1>
	struct Storage
	{
		static_assert(S >= 1, "Size must be larger 0");
		std::array<TData, S> data;

		static constexpr size_t size() noexcept { return S; }
		auto& at(size_t idx) { return data.at(idx); }
		const auto& at(size_t idx) const { return data.at(idx); }
		auto begin() { return data.begin(); }
		auto end() { return data.end(); }
		const std::array<TData, S>& asArray() const noexcept { return data; }
	};

	template <typename TData>
	struct Storage<TData, 1>
	{
		TData data;

		Storage() = default;
		template <typename U>
		requires(std::is_same_v<TData, std::string>)
		Storage(const U& d) noexcept : data{d} {}
		Storage(const TData& d) noexcept : data{ d } {}

		static constexpr size_t size() noexcept { return 1; }
		auto& at(size_t) { return data; }
		const auto& at(size_t) const { return data; }
		auto* begin() { return &data; }
		auto* end() { return &data + 1; }
		std::array<TData, 1> asArray() const noexcept { return { data }; }
	};

	template <auto Value>
	struct MaxLimit
	{
		template <typename T>
		static T apply(T x) noexcept { return x > static_cast<T>(Value) ? static_cast<T>(Value) : x; }
	};

	template <auto Value>
	struct MinLimit
	{
		template <typename T>
		static T apply(T x) noexcept { return x < static_cast<T>(Value) ? static_cast<T>(Value) : x; }
	};

	template <
		typename TData,
		size_t S = 1,
		typename... TLimits
	>
	class Var final : public VarBase
	{
		template <typename... TRest>
		struct LimitApplier
		{
			static auto apply(Storage<TData, S> x)
			{
				return x;
			}
		};

		template <typename TLimit, typename... TRest>
		struct LimitApplier<TLimit, TRest...>
		{
			static auto apply(Storage<TData, S> x)
			{
				for(uint32_t i = 0; i < x.size(); i++) x.at(i) = TLimit::apply(x.at(i));
				return LimitApplier<TRest...>::apply(x);
			}
		};

	public:
		using TStorage = Storage<TData, S>;
		static_assert(std::disjunction_v<std::is_integral<TData>, std::is_floating_point<TData>, std::is_same<TData, std::string>>
			, "Type must be integral, floating-point or string");
		static_assert((!std::is_same_v<TData, std::string> && !std::is_same_v<TData, bool>) || sizeof...(TLimits) == 0,
			"String and boolean values may not have limits");

		Var(const std::string_view shortName, const std::string_view longName, const TStorage& value = {},
		    const uint32_t flags = NONE, const std::string_view description = {},
		    const std::function<void(const Storage<TData, S>&)> callback = {})
			: VarBase(shortName, longName, flags, description, callback != nullptr),
				_callback{ callback }, _callbackCharged{ false }, _value{ LimitApplier<TLimits...>::apply(value) } {}

		~Var() override = default;
		Var(const Var&) = delete;
		Var(Var&&) = delete;
		Var& operator=(const Var&) = delete;
		Var& operator=(Var&&) = delete;

		void value(const TStorage& aValue)
		{
			if (isReadOnly() || isCliOnly() || isLocked()) return;
			setValueInternal(aValue);
		}
		const auto& value() const noexcept { return _value.data; }

		void valueString(const std::string_view string) override
		{
			if (isReadOnly() || isCliOnly() || isLocked()) return;
			setValueStringInternal(string);
		}
		std::string valueString() override
		{
			std::stringstream stream;
			for (size_t i = 0; i != _value.size(); i++)
			{
				if (i) stream << _delimiter;
				if constexpr (std::is_same_v<TData, std::string>) stream << _value.at(i);
				else if constexpr (std::is_same_v<TData, bool>) stream << std::boolalpha << _value.at(i);
				else stream << std::to_string(_value.at(i));
			}
			return stream.str();
		}

		void chargeCallback() noexcept { _callbackCharged = true; }
		bool executeCallback() override
		{
			if (hasCallback() && _callbackCharged)
			{
				_callback(_value);
				_callbackCharged = false;
				return true;
			}
			return false;
		}

		[[nodiscard]] size_t size() const noexcept override { return _value.size(); }
		[[nodiscard]] bool isCallbackCharged() const { return _callbackCharged; }

		[[nodiscard]] bool isBool() const override { return std::is_same_v<TData, bool>; }
		[[nodiscard]] bool isIntegral() const override { return std::is_integral_v<TData>; }
		[[nodiscard]] bool isFloatingPoint() const override { return std::is_floating_point_v<TData>; }
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

	private:
		void setValueInternal(const TStorage& value)
		{
			if (isReadOnly() || isLocked()) return;
			_value = value;
			if (hasCallback())
			{
				_callbackCharged = true;
				if (isCallbackAutoExecuted()) executeCallback();
			}
		}

		void setValueStringInternal(std::string_view string) override
		{
			if (isReadOnly() || isLocked()) return;
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

				if constexpr (std::is_floating_point_v<TData>) _value.at(count) = static_cast<TData>(parseDouble(*this, token));
				else if constexpr (std::is_same_v<TData, bool>) _value.at(count) = parseBool(token);
				else if constexpr (std::is_integral_v<TData>) _value.at(count) = static_cast<TData>(parseIntegral(*this, token));
				else if constexpr (std::is_same_v<TData, std::string>) _value.at(count) = token;
				count++;
			}
			while (pos != std::string::npos);

			if constexpr (!std::is_same_v<TData, std::string>)
			{
				_value = LimitApplier<TLimits...>::apply(_value);
			}

			_callbackCharged = true;
			if (isCallbackAutoExecuted()) executeCallback();
		}

		static constexpr const char* _delimiter = ",";
		const std::function<void(const Storage<TData, S>&)> _callback;
		bool _callbackCharged;
		TStorage _value;
	};

	class CCLIError : public std::exception {
	public:
		CCLIError(std::string m);
		const char* what() const final { return message().data(); }
		virtual std::string_view message() const { return _message; }
	protected:
		struct ArgType {};
		CCLIError(ArgType, std::string a);
		mutable std::string _message;
		std::string _arg;
	};

	class DuplicatedVarNameError final : public CCLIError {
	public:
		DuplicatedVarNameError(std::string name);
		std::string_view message() const override;
		std::string_view duplicatedName() const { return _arg; }
	};

	class FileError final : public CCLIError {
	public:
		FileError(std::string path);
		std::string_view message() const override;
		std::string_view filePath() const { return _arg; }
	};

	class UnknownVarNameError final : public CCLIError {
	public:
		UnknownVarNameError(std::string name);
		std::string_view message() const override;
		std::string_view unknownName() const { return _arg; }
	};

	class ConversionError final : public CCLIError {
	public:
		ConversionError(const VarBase&, std::string name);
		std::string_view message() const override;
		std::string_view unconvertibleValueString() const { return _arg; }
		const VarBase& variable() const { return _variable; }
	private:
		const VarBase& _variable;
	};
};
