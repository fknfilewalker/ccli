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

#include <ccli/ccli.h>

#include <cassert>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <charconv>
#include <variant>

namespace
{
	using namespace std::literals;

	constexpr char configDelimiter = '=';
	using MapType = std::map<std::string, ccli::VarBase*, std::less<>>;
	using VecType = std::vector<ccli::VarBase*>;

	/*
	** vars
	*/
	// contains static lists keeping track of all vars
	MapType& getLongNameVarMap()
	{
		static MapType map;
		return map;
	}

	MapType& getShortNameVarMap()
	{
		static MapType map;
		return map;
	}

	VecType& getVarList() {
		static VecType map;
		return map;
	}

	ccli::VarBase* findVarByLongName(const std::string_view longName)
	{
		MapType& map = getLongNameVarMap();
		const auto it = map.find(longName);
		if (it != map.end()) return it->second;
		return nullptr;
	}

	ccli::VarBase* findVarByShortName(const std::string_view shortName)
	{
		auto& map = getShortNameVarMap();
		const auto it = map.find(shortName);
		if (it != map.end()) return it->second;
		return nullptr;
	}

	void addToVarList(const std::string& longName, const std::string& shortName, ccli::VarBase* const aVar)
	{
		auto& mapLong = getLongNameVarMap();
		auto& mapShort = getShortNameVarMap();

		std::optional<std::pair<MapType::iterator, bool>> pairShort;
		std::optional<std::pair<MapType::iterator, bool>> pairLong;
		if (!shortName.empty())
		{
			pairShort= mapShort.insert(std::pair<std::string, ccli::VarBase*>(shortName, aVar));
		}

		if ((!pairShort.has_value() || pairShort->second) && !longName.empty())
		{
			pairLong = mapLong.insert(std::pair<std::string, ccli::VarBase*>(longName, aVar));
		}

		if ((pairShort.has_value() && !pairShort->second) || (pairLong.has_value() && !pairLong->second)) {
			if (pairShort.has_value() && pairShort->second) {
				mapShort.erase(pairShort->first);
			}

			assert(!(pairLong.has_value() && pairLong->second));
			throw ccli::DuplicatedVarNameError{ (pairShort.has_value() && !pairShort->second) ? shortName : longName };
		}

		getVarList().push_back(aVar);
	}

	void removeFromVarList(const std::string_view longName, const std::string_view shortName,
		const ccli::VarBase* const aVar)
	{
		auto& mapLong = getLongNameVarMap();
		auto& mapShort = getShortNameVarMap();
		auto& varList = getVarList();

		if (!longName.empty())
		{
			const auto it = mapLong.find(longName);
			if (it != mapLong.end() && it->second == aVar) mapLong.erase(it);
		}
		if (!shortName.empty())
		{
			const auto it = mapShort.find(shortName);
			if (it != mapShort.end() && it->second == aVar) mapShort.erase(it);
		}

		const auto it = std::find(varList.begin(), varList.end(), aVar);
		if (it != varList.end()) {
			varList.erase(it);
		}
	}

	/*
	** callbacks
	*/
	// contains static list keeping track of all callbacks
	std::set<ccli::VarBase*>& getCallbackSet()
	{
		static std::set<ccli::VarBase*> set;
		return set;
	}

	void addToCallbackSet(ccli::VarBase* var)
	{
		auto& set = getCallbackSet();
		set.insert(var);
	}

	void removeFromCallbackSet(ccli::VarBase* var)
	{
		auto& set = getCallbackSet();
		set.erase(var);
	}

	/*
	** config
	*/
	bool doesConfigVarNeedUpdate(ccli::ConfigCache& cache, const std::string& token, const std::string& value)
	{
		const auto it = cache.find(token);
		if (it != cache.end() && it->second != value)
		{
			it->second = value;
			return true;
		}
		if (it == cache.end())
		{
			cache.insert({ token, value });
			return true;
		}
		return false;
	}

	void writeConfigFile(std::string const& filename, const std::string_view content)
	{
		std::ofstream file(filename, std::ios::out | std::ios::binary);
		if (!file.is_open())
		{
			throw ccli::FileError{ filename };
		}
		file.write(content.data(), static_cast<std::streamsize>(content.size()));
		file.close();
	}
}

template<typename ...TErrors>
class DeferredError {
public:
	[[nodiscard]] bool hasError() const { return !std::holds_alternative<std::monostate>(_storage); }

	template<typename T, typename ...Args>
	void defer(Args&&... args) {
		if (!hasError()) {
			_storage = T{ std::forward<Args>(args)... };
		}
	}

	void throwError() {
		throwErrorImpl<TErrors...>();
	}

private:
	template<typename T, typename ...TRest>
	void throwErrorImpl() {
		if (std::holds_alternative<T>(_storage)) {
			throw std::get<T>(_storage);
		}

		if constexpr (sizeof...(TRest) > 0) {
			throwErrorImpl<TRest...>();
		}
	}

	std::variant<TErrors..., std::monostate> _storage{ std::monostate{} };
};

void ccli::parseArgs(const size_t argc, const char* const argv[])
{
	size_t i = 0;
	if (argc > 0)
	{
		// check if first arg is exe
		const std::string_view exePath{ argv[0] };
		if (std::filesystem::exists(std::filesystem::status(exePath)))
		{
			i++;
		}
	}

	DeferredError<ccli::UnknownVarNameError, ccli::MissingValueError> deferredError;

	std::string_view arg;
	VarBase* var = nullptr;
	size_t idxOffset = 0;
	auto valuelessVar = [&]() {
		// Arg without value is only allowed for bools
		if (var && idxOffset == 0) {
			if (var->isBool() && var->size() == 1) {
				var->setValueStringInternal("");
				return;
			}

			deferredError.defer<ccli::MissingValueError>(std::string{ arg });
		}
	};

	for (; i < argc; i++)
	{
		arg= argv[i];
		const bool shortName = arg.size() >= 2 ? arg[0] == '-' && isalpha(arg[1]) : false;
		const bool longName = arg.size() >= 2 ? arg[0] == '-' && arg[1] == '-' : false;
		if (shortName || longName)
		{
			valuelessVar();

			// find new arg
			var = nullptr;
			idxOffset = 0;
			if (longName) var = findVarByLongName(arg.substr(2));
			else if (shortName) var = findVarByShortName(arg.substr(1));
			
			// error if not found -> throw the error after parsing the rest of the arguments
			if (var == nullptr) {
				deferredError.defer<ccli::UnknownVarNameError>(std::string{ arg });
			}
		}
		// Var found
		else if (var)
		{
			idxOffset= var->setValueStringInternal(arg, idxOffset);
		}
	}
	
	// Var is last argument
	valuelessVar();

	// Finally throw any potential error
	deferredError.throwError();
}

ccli::ConfigCache ccli::loadConfig(const std::string& cfgFile)
{
	std::map<std::string, std::string> configMap;
	// check if file exists
	std::ifstream f(cfgFile);

	// load config line by line
	while (f.good())
	{
		std::string line;
		std::getline(f, line);
		// remove quotation marks etc
		std::erase(line, '\"');
		std::erase(line, '\'');
		const size_t pos = line.find(configDelimiter);
		if (pos != std::string::npos)
		{
			auto token = line.substr(0, pos);
			auto value = line.substr(pos + 1, line.size());
			VarBase* var = findVarByLongName(token);
			// also check rd
			if (var && var->isConfigRead())
			{
				var->valueString(value);
				configMap.insert({ std::move(token), std::move(value) });
			}
			else configMap.insert({ std::move(token), std::move(value) });
		}
	}
	f.close();
	return configMap;
}

void ccli::writeConfig(const std::string& cfgFile, ConfigCache& cache)
{
	const auto map = getLongNameVarMap();
	bool write = false;
	// update vars
	for (const auto& [fst, snd] : map)
	{
		if (snd->isConfigReadWrite())
		{
			// also check if rdwr
			write |= doesConfigVarNeedUpdate(cache, fst, snd->valueString());
		}
	}
	if (!write) return;
	// create output string
	std::stringstream outStream;
	for (const auto& [fst, snd] : cache)
	{
		outStream << fst << configDelimiter;
		outStream << '\"' << snd << "\"\n";
	}
	// write config
	if (outStream.tellp()) writeConfigFile(cfgFile, outStream.view());
}

void ccli::writeConfig(const std::string& cfgFile)
{
	ConfigCache cache;
	writeConfig(cfgFile, cache);
}

void ccli::executeCallbacks()
{
	for (const auto var : getCallbackSet())
	{
		var->executeCallback();
	}
}

ccli::IterationDecision ccli::forEachVar(const std::function<IterationDecision(VarBase&, size_t)>& callback)
{
	size_t idx = 0;
	const auto& varList = getVarList();
	for (auto* varPtr : varList)
	{
		auto result = callback(*varPtr, idx++);
		if (result == IterationDecision::Break)
		{
			return result;
		}
	}

	return IterationDecision::Continue;
}

/*
** VarBase
*/
ccli::VarBase::VarBase(const std::string_view shortName, const std::string_view longName, const uint32_t flags,
	const std::string_view description, const bool hasCallback) :
	_shortName{ shortName }, _longName{ longName },
	_description{ description }, _flags{ flags }, _hasCallback{ hasCallback }
{
	assert(!_longName.empty() || !_shortName.empty());
	addToVarList(_longName, _shortName, this);
	if (_hasCallback) addToCallbackSet(this);
	/*if (_longName.empty() && (isConfigRead() || isConfigReadWrite())) {
		getErrorDeque().emplace_back("Config requieres long name \"\'-" + _shortName + "\'");
	}*/
}

ccli::VarBase::~VarBase()
{
	removeFromVarList(_longName, _shortName, this);
	if (_hasCallback) removeFromCallbackSet(this);
}

const std::string& ccli::VarBase::longName() const noexcept
{
	return _longName;
}

const std::string& ccli::VarBase::shortName() const noexcept
{
	return _shortName;
}

const std::string& ccli::VarBase::description() const noexcept
{
	return _description;
}

void ccli::VarBase::valueString(std::string_view string)
{
	if (isReadOnly() || isCliOnly() || isLocked()) return;
	setValueStringInternal(string);
}

bool ccli::VarBase::hasCallback() const noexcept
{
	return _hasCallback;
}

bool ccli::VarBase::isCliOnly() const noexcept
{
	return _flags & CliOnly;
}

bool ccli::VarBase::isReadOnly() const noexcept
{
	return _flags & ReadOnly;
}

bool ccli::VarBase::isLocked() const noexcept
{
	return _flags & Locked;
}

bool ccli::VarBase::isConfigRead() const noexcept
{
	return !_longName.empty() && ((_flags & ConfigRead) == ConfigRead);
}

bool ccli::VarBase::isConfigReadWrite() const noexcept
{
	return !_longName.empty() && ((_flags & ConfigRdwr) == ConfigRdwr);
}

bool ccli::VarBase::isCallbackAutoExecuted() const noexcept
{
	return !(_flags & ManualExec);
}

void ccli::VarBase::lock() noexcept
{
	_flags = _flags | Locked;
}

void ccli::VarBase::unlock() noexcept
{
	_flags = _flags ^ Locked;
}

void ccli::VarBase::locked(const bool locked) noexcept
{
	if (locked) lock();
	else unlock();
}

size_t ccli::VarBase::setValueStringInternal(const std::string_view string, const size_t offset)
{
	if (isReadOnly() || isLocked()) return offset+ 1;
	// empty string only allowed for bool and string
	if (string.empty()) {
		if (!isBool() && !isString()) return offset+ 1;
	}

	const auto maxSize = size();
	CSVParser csv{ string, _delimiter };
	do
	{
		if (csv.count()+ offset >= maxSize) break;

		const auto token = csv.next();
		setValueStringInternalAtIndex(csv.count() + offset - 1, token);
	} while (csv.hasNext());

	applyLimitsAndDoCallback();

	return csv.count() + offset;
}

template <typename T>
T parseUsingFromChars(const ccli::VarBase& var, std::string_view token)
{
	T value;
	auto result = std::from_chars(token.data(), token.data() + token.size(), value);
	if (result.ec == std::errc::invalid_argument)
	{
		throw ccli::ConversionError{ var, std::string{ token } };
	}

	return value;
}

long long ccli::VarBase::parseIntegral(const ccli::VarBase& var, const std::string_view token)
{
	return parseUsingFromChars<long long>(var, token);
}

double ccli::VarBase::parseDouble(const ccli::VarBase& var, const std::string_view token)
{
	return parseUsingFromChars<double>(var, token);
}

bool ccli::VarBase::parseBool(const std::string_view token)
{
	if (token.empty())
	{
		return true;
	}

	bool value;
	const std::string copiedToken{ token };

	std::istringstream{ copiedToken } >> std::boolalpha >> value;
	if (value)
	{
		return true;
	}

	std::istringstream{ copiedToken } >> value;
	return value;
}

template<typename T, typename ... Rest>
void builderAppend(std::string& buffer, const T& val, const Rest& ... rest) {
	buffer+= val;

	if constexpr (sizeof...(Rest) > 0) {
		builderAppend(buffer, rest...);
	}
}

template<typename T, typename ... Rest>
size_t countAppendedLength(const T& val, const Rest& ... rest)
{
	if constexpr (sizeof...(Rest) > 0) {
		return val.size() + countAppendedLength(rest...);
	}

	return val.size();
}

template<typename ... T>
std::string buildString(const T& ... vals) {
	std::string buffer;
	buffer.reserve(countAppendedLength(vals...));
	builderAppend(buffer, vals...);
	return buffer;
}

ccli::CCLIError::CCLIError(std::string m)
	: _message{ std::move(m) } {}

ccli::CCLIError::CCLIError(ccli::CCLIError::ArgType, std::string a)
	: _arg{ std::move(a) } {}

ccli::DuplicatedVarNameError::DuplicatedVarNameError(std::string name)
	: CCLIError{ {}, std::move(name) } {}

std::string_view ccli::DuplicatedVarNameError::message() const
{
	if (_message.empty()) {
		_message = buildString("Variable with the identifier '"sv, _arg, "' already exists. Cannot create another one."sv);
	}

	return _message;
}

ccli::FileError::FileError(std::string path)
	: CCLIError{ {}, std::move(path) } {}

std::string_view ccli::FileError::message() const
{
	if (_message.empty()) {
		_message = buildString("Could not open file '"sv, _arg, "' for writing. Could not save varibles to disk."sv);
	}

	return _message;
}

ccli::UnknownVarNameError::UnknownVarNameError(std::string name)
	: CCLIError{ {}, std::move(name) } {}

std::string_view ccli::UnknownVarNameError::message() const
{
	if (_message.empty()) {
		_message = buildString("Unknown variable name '"sv, _arg, "' while parsing arguments."sv);
	}

	return _message;
}

ccli::MissingValueError::MissingValueError(std::string name)
	: CCLIError{ {}, std::move(name) } {}

std::string_view ccli::MissingValueError::message() const
{
	if (_message.empty()) {
		_message = buildString("Variable '"sv, _arg, "' requires value when parsing arguments. (Only boolean variables are allowed as valueless switches.)"sv);
	}

	return _message;
}

ccli::ConversionError::ConversionError(const VarBase& var, std::string name)
	: CCLIError{ {}, std::move(name) }, _variable{ var } {}

std::string_view ccli::ConversionError::message() const
{
	if (_message.empty()) {
		_message = buildString("Could not convert '"sv, _arg, "' to variable."sv);
	}

	return _message;
}

bool ccli::CSVParser::hasNext() const
{
	return _pos != std::string::npos;
}

std::string_view ccli::CSVParser::next()
{
	_pos = _data.find(_delimiter, _current);
	_token = _data.substr(_current, _pos - _current);
	_current = _pos + 1;
	_count++;

	return _token;
}

size_t ccli::CSVParser::count() const
{
	return _count;
}

std::string_view ccli::CSVParser::token() const
{
	return _token;
}
