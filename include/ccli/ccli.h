/*
MIT License

Copyright(c) 2023 Lukas Lipp, Matthias Preymann

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
#include <span>
#include <map>
#include <cstdint>

namespace ccli
{
	// Parse
	void parseArgs(size_t argc, const char* const argv[]);
	// Config
	using ConfigCache = std::map<std::string, std::string>;
	ConfigCache loadConfig(const std::string& cfgFile);
	void writeConfig(const std::string& cfgFile, ConfigCache& cache);
	void writeConfig(const std::string& cfgFile);
	// Callback
	void executeCallbacks();
	// For all vars
	class VarBase;
	enum class IterationDecision { Continue, Break };
	IterationDecision forEachVar(const std::function<IterationDecision(VarBase& var, size_t idx)>&);

	enum Flag
	{
		None		= (0 << 0),
		ReadOnly	= (1 << 0),	// display only, cannot be modified at all
		CliOnly		= (1 << 1),	// can only be set through parseArgs
		Locked		= (1 << 2),	// this var is locked, cannot be modified until unlocked
		ConfigRead	= (1 << 3),	// load variable from config file
		ConfigRdwr	= (3 << 3),	// load variable from config file and save changes back to config file
		ManualExec	= (1 << 5)	// execute callback only when executeCallback/executeCallbacks is called
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
		void valueString(std::string_view string);

		virtual void chargeCallback() noexcept = 0;
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

		virtual bool tryStore(bool, size_t = 0) = 0;
		virtual bool tryStore(long long, size_t = 0) = 0;
		virtual bool tryStore(double, size_t = 0) = 0;
		virtual bool tryStore(std::string, size_t = 0) = 0;

		void lock() noexcept;
		void unlock() noexcept;
		void locked(bool locked) noexcept;

        size_t setValueStringInternal(std::string_view, size_t offset = 0);
	protected:
		static constexpr char _delimiter = ',';

		virtual void setValueStringInternalAtIndex(size_t, std::string_view) = 0;
		virtual void applyLimitsAndDoCallback() = 0;

		static long long parseIntegral(const VarBase&, std::string_view);
		static double parseDouble(const VarBase&, std::string_view);
		static bool parseBool(std::string_view);

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
		using TUnderlying = std::array<TData, S>;
		using TParameter = std::span<const TData>;
		TUnderlying data;

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
		using TUnderlying = TData;
		using TParameter = const TData&;
		TUnderlying data;

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
		using TCallback= std::function<void(typename TStorage::TParameter)>;
		static_assert(std::disjunction_v<std::is_integral<TData>, std::is_floating_point<TData>, std::is_same<TData, std::string>>
			, "Type must be integral, floating-point or string");
		static_assert((!std::is_same_v<TData, std::string> && !std::is_same_v<TData, bool>) || sizeof...(TLimits) == 0,
			"String and boolean values may not have limits");

		Var(const std::string_view shortName, const std::string_view longName, const TStorage& value = {},
		    const uint32_t flags = None, const std::string_view description = {},
		    const TCallback callback = {})
			: VarBase(shortName, longName, flags, description, static_cast<bool>(callback)),
				_callback{ callback }, _callbackCharged{ false }, _value{ LimitApplier<TLimits...>::apply(value) } {}

		~Var() override = default;
		Var(const Var&) = delete;
		Var(Var&&) = delete;
		Var& operator=(const Var&) = delete;
		Var& operator=(Var&&) = delete;

		void value(const TStorage& value)
		{
			if (isReadOnly() || isCliOnly() || isLocked()) return;
			setValueInternal(value);
		}

		const auto& value() const noexcept { return _value.data; }

		operator const TData&() const noexcept requires(S == 1) { return _value.data; }
		operator const char* () const noexcept requires(S == 1 && std::is_same_v<TData, std::string>) { return _value.data.c_str(); }
		const TData& operator[](size_t idx) const noexcept requires(S > 1) { return _value.data.at(idx); }

		std::string valueString() override
		{
			std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
			for (size_t i = 0; i != _value.size(); i++)
			{
				if (i) stream << _delimiter;
				if constexpr (std::is_same_v<TData, std::string>) stream << _value.at(i);
				else if constexpr (std::is_same_v<TData, bool>) stream << std::boolalpha << _value.at(i);
				else stream << std::to_string(_value.at(i));
			}
			return stream.str();
		}

		void chargeCallback() noexcept override { if(hasCallback()) _callbackCharged = true; }
		bool executeCallback() override
		{
			if (hasCallback() && _callbackCharged)
			{
				_callback(_value.data);
				_callbackCharged = false;
				return true;
			}
			return false;
		}

		[[nodiscard]] size_t size() const noexcept override { return _value.size(); }
		[[nodiscard]] bool isCallbackCharged() const noexcept { return _callbackCharged; }

		[[nodiscard]] bool isBool() const override { return std::is_same_v<TData, bool>; }
		[[nodiscard]] bool isIntegral() const override { return not isBool() && std::is_integral_v<TData>; }
		[[nodiscard]] bool isFloatingPoint() const override { return std::is_floating_point_v<TData>; }
		[[nodiscard]] bool isString() const override { return std::is_same_v<TData, std::string>; }

		template<typename T>
		[[nodiscard]] std::optional<T> asNumeric(size_t idx = 0) const
		{
			if constexpr (not std::is_same_v<TData, std::string>)
			{
				return { static_cast<T>(_value.at(idx)) };
			}
			return {};
		}

		[[nodiscard]] std::optional<bool> asBool(const size_t idx = 0) const override { return asNumeric<bool>( idx ); }
		[[nodiscard]] std::optional<long long> asInt(const size_t idx = 0) const override { return asNumeric<long long>(idx); }
		[[nodiscard]] std::optional<double> asFloat(const size_t idx = 0) const override { return asNumeric<double>(idx); }

		[[nodiscard]] std::optional<std::string_view> asString(size_t idx = 0) const override
		{
			if constexpr (std::is_same_v<TData, std::string>)
			{
				return { std::string_view{ _value.at(idx) } };
			}
			return {};
		}

		template<typename T>
		bool tryStoreNumeric(T val, size_t idx = 0) {
			if constexpr (not std::is_same_v<TData, std::string>)
			{
				_value.at(idx) = static_cast<TData>(val);
				return true;
			}
			return false;
		}

		bool tryStore(const bool val, const size_t idx = 0) override { return tryStoreNumeric<bool>(val, idx); }
		bool tryStore(const long long val, const size_t idx = 0) override { return tryStoreNumeric<long long>(val, idx); }
		bool tryStore(const double val, const size_t idx = 0) override { return tryStoreNumeric<double>(val, idx); }

		bool tryStore(std::string val, size_t idx = 0) override
		{
			if constexpr (std::is_same_v<TData, std::string>)
			{
				_value.at(idx) = std::move(val);
				return true;
			}
			return false;
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

		void setValueStringInternalAtIndex(size_t idx, std::string_view token) override {
			if (idx >= _value.size()) return;

			if constexpr (std::is_floating_point_v<TData>) _value.at(idx) = static_cast<TData>(parseDouble(*this, token));
			else if constexpr (std::is_same_v<TData, bool>) _value.at(idx) = parseBool(token);
			else if constexpr (std::is_integral_v<TData>) _value.at(idx) = static_cast<TData>(parseIntegral(*this, token));
			else if constexpr (std::is_same_v<TData, std::string>) _value.at(idx) = token;
		}

		void applyLimitsAndDoCallback() override {
			if constexpr (!std::is_same_v<TData, std::string>)
			{
				_value = LimitApplier<TLimits...>::apply(_value);
			}

			_callbackCharged = true;
			if (isCallbackAutoExecuted()) executeCallback();
		}

		const TCallback _callback;
		bool _callbackCharged;
		TStorage _value;
	};

	class CCLIError : public std::exception {
	public:
		explicit CCLIError(std::string m);
        const char* what() const noexcept override { return message().data(); }
		virtual std::string_view message() const { return _message; }
		virtual void throwSelf() const = 0;
	protected:
		struct ArgType {};
		CCLIError(ArgType, std::string a);
		mutable std::string _message;
		std::string _arg;
	};

	class DuplicatedVarNameError final : public CCLIError {
	public:
		explicit DuplicatedVarNameError(std::string name);
		std::string_view message() const override;
		void throwSelf() const override;
		std::string_view duplicatedName() const { return _arg; }
	};

	class FileError final : public CCLIError {
	public:
		explicit FileError(std::string path);
		std::string_view message() const override;
		void throwSelf() const override;
		std::string_view filePath() const { return _arg; }
	};

	class UnknownArgError final : public CCLIError {
	public:
		explicit UnknownArgError(std::string name);
		std::string_view message() const override;
		void throwSelf() const override;
		std::string_view unknownName() const { return _arg; }
	};

	class MissingValueError final : public CCLIError {
	public:
		explicit MissingValueError(std::string name);
		std::string_view message() const override;
		void throwSelf() const override;
		std::string_view variable() const { return _arg; }
	};

	class ConversionError final : public CCLIError {
	public:
		ConversionError(const VarBase&, std::string name);
		std::string_view message() const override;
		void throwSelf() const override;
		std::string_view unconvertibleValueString() const { return _arg; }
		const VarBase& variable() const { return _variable; }
	private:
		const VarBase& _variable;
	};

	// Deduction guides
    template <class T, class... U>
	explicit Storage(T, U...)->Storage<T, 1 + sizeof...(U)>;

	// Size N
	template<typename T, size_t S>
	Var(std::string_view, std::string_view, const T(&)[S]) -> Var<T, S>;
	template<typename T, size_t S>
	Var(std::string_view, std::string_view, const T(&)[S], uint32_t) -> Var<T, S>;
	template<typename T, size_t S>
	Var(std::string_view, std::string_view, const T(&)[S], uint32_t, std::string_view) -> Var<T, S>;
	// Size N with callback
	template<typename T, size_t S, typename F>
	Var(std::string_view, std::string_view, const T(&)[S], uint32_t, std::string_view, F) -> Var<T, S>;

	// Size 1
	template<typename T>
	Var(std::string_view, std::string_view, T) -> Var<T, 1>;
	template<typename T>
	Var(std::string_view, std::string_view, T, uint32_t) -> Var<T, 1>;
	template<typename T>
	Var(std::string_view, std::string_view, T, uint32_t, std::string_view) -> Var<T, 1>;
	// Size 1 with callback
	template<typename T, typename F>
	Var(std::string_view, std::string_view, T, uint32_t, std::string_view, F) -> Var<T, 1>;

	// Char
	Var(std::string_view, std::string_view, const char*)->Var<std::string, 1>;
	Var(std::string_view, std::string_view, const char*, uint32_t)->Var<std::string, 1>;
	Var(std::string_view, std::string_view, const char*, uint32_t, std::string_view)->Var<std::string, 1>;
	// Char with callback
	template<typename F>
	Var(std::string_view, std::string_view, const char*, uint32_t, std::string_view, F) -> Var<std::string, 1>;
};
