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

namespace
{
	constexpr char configDelimiter = '=';
	/*
	** errors
	*/
	std::deque<std::string>& getErrorDeque()
	{
		static std::deque<std::string> deque;
		return deque;
	}

	/*
	** vars
	*/
	// contains static lists keeping track of all vars
	std::map<std::string, ccli::VarBase*, std::less<>>& getLongNameVarMap()
	{
		static std::map<std::string, ccli::VarBase*, std::less<>> map;
		return map;
	}

	std::map<std::string, ccli::VarBase*, std::less<>>& getShortNameVarMap()
	{
		static std::map<std::string, ccli::VarBase*, std::less<>> map;
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
		if (!longName.empty())
		{
			const bool inserted = mapLong.insert(std::pair<std::string, ccli::VarBase*>(longName, aVar)).second;
			if (!inserted) getErrorDeque().emplace_back("Long identifier '--" + longName + "' already exists");
		}
		if (!shortName.empty())
		{
			const bool inserted = mapShort.insert(std::pair<std::string, ccli::VarBase*>(shortName, aVar)).second;
			if (!inserted) getErrorDeque().emplace_back("Short identifier '-" + shortName + "' already exists");
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
			aMap.insert({aToken, aValue});
			return true;
		}
		return false;
	}

	bool writeConfigFile(std::string const& aFilename, const std::string_view aContent)
	{
		std::ofstream file(aFilename, std::ios::out | std::ios::binary);
		if (!file.is_open())
		{
			getErrorDeque().emplace_back("Could not open file '" + aFilename + "' for writing");
			return false;
		}
		file.write(aContent.data(), static_cast<std::streamsize>(aContent.size()));
		file.close();
		return true;
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
		const auto& arg = args.front();
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
			if (var == nullptr) getErrorDeque().emplace_back("'" + arg + "' not found");
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
				var->setValueString(value);
				if (var->isConfigReadWrite()) configMap.insert({token, value});
			}
			else configMap.insert({token, value});
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
			write |= doesConfigVarNeedUpdate(configMap, fst, snd->getValueString());
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

std::deque<std::string> ccli::checkErrors()
{
	std::deque<std::string>& deque = getErrorDeque();
	std::deque<std::string> out = deque;
	deque.clear();
	return out;
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
	_description{ description }, _flags{ flags }, _hasCallback{ hasCallback }, _locked{ false }
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

const std::string& ccli::VarBase::getLongName() const
{
	return _longName;
}

const std::string& ccli::VarBase::getShortName() const
{
	return _shortName;
}

const std::string& ccli::VarBase::getDescription() const
{
	return _description;
}

bool ccli::VarBase::hasCallback() const
{
	return _hasCallback;
}

bool ccli::VarBase::isCliOnly() const
{
	return _flags & CLI_ONLY;
}

bool ccli::VarBase::isReadOnly() const
{
	return _flags & READ_ONLY;
}

bool ccli::VarBase::isConfigRead() const
{
	return !_longName.empty() && _flags & CONFIG_RD;
}

bool ccli::VarBase::isConfigReadWrite() const
{
	return !_longName.empty() && _flags & CONFIG_RDWR;
}

bool ccli::VarBase::isCallbackAutoExecuted() const
{
	return !(_flags & MANUAL_EXEC);
}

bool ccli::VarBase::locked() const
{
	return _locked;
}

void ccli::VarBase::locked(const bool aLocked)
{
	_locked = aLocked;
}

template <typename T>
T parseUsingFromChars(std::string_view token)
{
	T value;
	auto result = std::from_chars(token.data(), token.data() + token.size(), value);
	if (result.ec == std::errc::invalid_argument)
	{
		getErrorDeque().emplace_back("'" + std::string{ token } + "' not convertible");
	}

	return value;
}

long long ccli::VarBase::parseIntegral(const std::string_view token)
{
	return parseUsingFromChars<long long>(token);
}

double ccli::VarBase::parseDouble(const std::string_view token)
{
	return parseUsingFromChars<double>(token);
}

bool ccli::VarBase::parseBool(const std::string_view token)
{
	if (token.empty())
	{
		return true;
	}

	bool value;
	const std::string copiedToken{token};

	std::istringstream{copiedToken} >> std::boolalpha >> value;
	if (value)
	{
		return true;
	}

	std::istringstream{copiedToken} >> value;
	return value;
}
