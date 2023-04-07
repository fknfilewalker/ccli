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

#include <ccli/ccli.h>

#include <cassert>
#include <map>
#include <set>
#include <fstream>
#include <filesystem>
#include <charconv>
#include <deque>

namespace
{
	using namespace std::literals;

	constexpr char configDelimiter = '=';
	using MapType = std::map<std::string, ccli::VarBase*, std::less<>>;

	/*
	** vars
	*/
	// contains static lists keeping track of all vars
	std::map<std::string, ccli::VarBase*, std::less<>>& getLongNameVarMap()
	{
		static MapType map;
		return map;
	}

	std::map<std::string, ccli::VarBase*, std::less<>>& getShortNameVarMap()
	{
		static MapType map;
		return map;
	}

	ccli::VarBase* findVarByLongName(const std::string_view longName)
	{
		std::map<std::string, ccli::VarBase*, std::less<>>& map = getLongNameVarMap();
		const auto it = map.find(longName);
		if (it != map.end()) return it->second;
		return nullptr;
	}

	ccli::VarBase* findVarByShortName(const std::string_view shortName)
	{
		std::map<std::string, ccli::VarBase*, std::less<>>& map = getShortNameVarMap();
		const auto it = map.find(shortName);
		if (it != map.end()) return it->second;
		return nullptr;
	}

	void addToVarList(const std::string& longName, const std::string& shortName, ccli::VarBase* const aVar)
	{
		std::map<std::string, ccli::VarBase*, std::less<>>& mapLong = getLongNameVarMap();
		std::map<std::string, ccli::VarBase*, std::less<>>& mapShort = getShortNameVarMap();

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
	}

	void removeFromVarList(const std::string_view longName, const std::string_view shortName,
		const ccli::VarBase* const aVar)
	{
		std::map<std::string, ccli::VarBase*, std::less<>>& mapLong = getLongNameVarMap();
		std::map<std::string, ccli::VarBase*, std::less<>>& mapShort = getShortNameVarMap();

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

	void addToCallbackSet(ccli::VarBase* aVar)
	{
		auto& set = getCallbackSet();
		set.insert(aVar);
	}

	void removeFromCallbackSet(ccli::VarBase* aVar)
	{
		auto& set = getCallbackSet();
		set.erase(aVar);
	}

	/*
	** config
	*/
	// keep track of all data from loaded config
	std::map<std::string, std::string>& getConfigMap()
	{
		static std::map<std::string, std::string> map;
		return map;
	}

	bool doesConfigVarNeedUpdate(std::map<std::string, std::string>& aMap, const std::string& aToken,
		const std::string& aValue)
	{
		const auto it = aMap.find(aToken);
		if (it != aMap.end() && it->second != aValue)
		{
			it->second = aValue;
			return true;
		}
		if (it == aMap.end())
		{
			aMap.insert({ aToken, aValue });
			return true;
		}
		return false;
	}

	void writeConfigFile(std::string const& aFilename, const std::string_view aContent)
	{
		std::ofstream file(aFilename, std::ios::out | std::ios::binary);
		if (!file.is_open())
		{
			throw ccli::FileError{ aFilename };
		}
		file.write(aContent.data(), static_cast<std::streamsize>(aContent.size()));
		file.close();
	}
}

void ccli::parseArgs(const size_t argc, const char* const argv[])
{
	std::deque<std::string> args;
	for (size_t i = 0; i < argc; i++)
	{
		args.emplace_back(argv[i]);
	}
	if (!args.empty())
	{
		// check if first arg is exe
		if (std::filesystem::exists(std::filesystem::status(args.front())))
		{
			args.pop_front();
		}
	}

	VarBase* var = nullptr;
	while (!args.empty())
	{
		auto arg = std::move(args.front());
		const bool shortName = arg.size() >= 2 ? arg[0] == '-' && isalpha(arg[1]) : false;
		const bool longName = arg.size() >= 2 ? arg[0] == '-' && arg[1] == '-' : false;
		if (shortName || longName)
		{
			// arg without value (cleared otherwise)
			if (var && var->isBool() && var->size() == 1) var->setValueStringInternal("");
			// find new arg
			if (longName) var = findVarByLongName(arg.substr(2));
			else if (shortName) var = findVarByShortName(arg.substr(1));
			// error if not found
			if (var == nullptr) {
				throw ccli::UnknownVarNameError{ std::move(arg) };
			}
		}
		// Var found
		else if (var)
		{
			var->setValueStringInternal(arg);
			var = nullptr;
		}
		args.pop_front();
	}
	// Var is last argument
	if (var && var->isBool() && var->size() == 1) var->setValueStringInternal("");
}

void ccli::loadConfig(const std::string& cfgFile)
{
	std::map<std::string, std::string>& configMap = getConfigMap();
	configMap.clear();
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
			std::string token = line.substr(0, pos);
			std::string value = line.substr(pos + 1, line.size());
			VarBase* var = findVarByLongName(token);
			// also check rd
			if (var && var->isConfigRead())
			{
				var->valueString(value);
				if (var->isConfigReadWrite()) configMap.insert({ token, value });
			}
			else configMap.insert({ token, value });
		}
	}
	f.close();
}

void ccli::writeConfig(const std::string& cfgFile)
{
	std::map<std::string, std::string>& configMap = getConfigMap();
	const auto map = getLongNameVarMap();
	bool write = false;
	// update vars
	for (const auto& [fst, snd] : map)
	{
		if (snd->isConfigReadWrite())
		{
			// also check if rdwr
			write |= doesConfigVarNeedUpdate(configMap, fst, snd->valueString());
		}
	}
	if (!write) return;
	// create output string
	std::string out;
	for (const auto& [fst, snd] : configMap)
	{
		out += fst + configDelimiter;
		out += "\"" + snd + "\"\n";
	}
	// write config
	if (!out.empty()) writeConfigFile(cfgFile, out);
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
	const auto& map = getShortNameVarMap();
	for (auto& pair : map)
	{
		auto result = callback(*pair.second, idx++);
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

bool ccli::VarBase::hasCallback() const noexcept
{
	return _hasCallback;
}

bool ccli::VarBase::isCliOnly() const noexcept
{
	return _flags & CLI_ONLY;
}

bool ccli::VarBase::isReadOnly() const noexcept
{
	return _flags & READ_ONLY;
}

bool ccli::VarBase::isLocked() const noexcept
{
	return _flags & LOCKED;
}

bool ccli::VarBase::isConfigRead() const noexcept
{
	return !_longName.empty() && _flags & CONFIG_RD;
}

bool ccli::VarBase::isConfigReadWrite() const noexcept
{
	return !_longName.empty() && _flags & CONFIG_RDWR;
}

bool ccli::VarBase::isCallbackAutoExecuted() const noexcept
{
	return !(_flags & MANUAL_EXEC);
}

void ccli::VarBase::locked(const bool locked)
{
	if(locked) _flags = _flags | LOCKED;
	else _flags = _flags ^ LOCKED;
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
void streamAppend(std::stringstream& stream, const T& val, const Rest& ... rest) {
	stream << val;

	if constexpr (sizeof...(Rest) > 0) {
		streamAppend(stream, rest...);
	}
}

template<typename ... T>
std::string buildString(const T& ... vals) {
	std::stringstream stream;
	streamAppend(stream, vals...);
	return stream.str();
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

ccli::ConversionError::ConversionError(const VarBase& var, std::string name)
	: CCLIError{ {}, std::move(name) }, _variable{ var } {}

std::string_view ccli::ConversionError::message() const
{
	if (_message.empty()) {
		_message = buildString("Could not convert '"sv, _arg, "' to variable."sv);
	}

	return _message;
}
