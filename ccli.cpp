/*
MIT License

Copyright(c) 2022 Lukas Lipp
Copyright(c) 2023 Matthias Preymann

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

#include "ccli.hpp"

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <deque>
#include <fstream>
#include <filesystem>
#include <charconv>

constexpr char configDelimiter = '=';
constexpr uint32_t helpColumnWidthShort = 20;
constexpr uint32_t helpColumnWidthLong = 20;

namespace {
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
	std::map<std::string, ccli::var_base*>& getLongNameVarMap()
	{
		static std::map<std::string, ccli::var_base*> map;
		return map;
	}
	std::map<std::string, ccli::var_base*>& getShortNameVarMap()
	{
		static std::map<std::string, ccli::var_base*> map;
		return map;
	}
	ccli::var_base* findVarByLongName(const std::string& aLongName)
	{
		std::map<std::string, ccli::var_base*>& map = getLongNameVarMap();
		const auto it = map.find(aLongName);
		if (it != map.end()) return it->second;
		return nullptr;
	}
	ccli::var_base* findVarByShortName(const std::string& aShortName)
	{
		std::map<std::string, ccli::var_base*>& map = getShortNameVarMap();
		const auto it = map.find(aShortName);
		if (it != map.end()) return it->second;
		return nullptr;
	}
	void addToVarList(const std::string& aLongName, const std::string& aShortName, ccli::var_base* const aVar)
	{
		std::map<std::string, ccli::var_base*>& mapLong = getLongNameVarMap();
		std::map<std::string, ccli::var_base*>& mapShort = getShortNameVarMap();
		if (!aLongName.empty()) {
			const bool inserted = mapLong.insert(std::pair<std::string, ccli::var_base*>(aLongName, aVar)).second;
			if (!inserted) getErrorDeque().emplace_back("Long identifier '--" + aLongName + "' already exists");
		}
		if (!aShortName.empty()) {
			const bool inserted = mapShort.insert(std::pair<std::string, ccli::var_base*>(aShortName, aVar)).second;
			if (!inserted) getErrorDeque().emplace_back("Short identifier '-" + aShortName + "' already exists");
		}
	}
	void removeFromVarList(const std::string& aLongName, const std::string& aShortName, const ccli::var_base* const aVar)
	{
		std::map<std::string, ccli::var_base*>& mapLong = getLongNameVarMap();
		std::map<std::string, ccli::var_base*>& mapShort = getShortNameVarMap();

		if (!aLongName.empty()) {
			const auto it = mapLong.find(aLongName);
			if (it != mapLong.end() && it->second == aVar) mapLong.erase(it);
		}
		if (!aShortName.empty()) {
			const auto it = mapShort.find(aShortName);
			if (it != mapShort.end() && it->second == aVar) mapShort.erase(it);
		}
	}

	/*
	** callbacks
	*/
	// contains static list keeping track of all callbacks
	std::set<ccli::var_base*>& getCallbackSet()
	{
		static std::set<ccli::var_base*> set;
		return set;
	}
	void addToCallbackSet(ccli::var_base* aVar)
	{
		auto& set = getCallbackSet();
		set.insert(aVar);
	}
	void removeFromCallbackSet(ccli::var_base* aVar)
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
	bool doesConfigVarNeedUpdate(std::map<std::string, std::string>& aMap, const std::string& aToken, const std::string& aValue)
	{
		const auto it = aMap.find(aToken);
		if (it != aMap.end() && it->second != aValue) {
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
	bool writeConfigFile(std::string const& aFilename, std::string const& aContent)
	{
		std::ofstream file(aFilename, std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			getErrorDeque().emplace_back("Could not open file '" + aFilename + "' for writing");
			return false;
		}
		file.write(aContent.c_str(), static_cast<std::streamsize>(aContent.size()));
		file.close();
		return true;
	}
}

void ccli::parseArgs(const int aArgc, const char* const aArgv[])
{
	std::deque<std::string> args;
	for (int i = 0; i < aArgc; i++) {
		args.emplace_back(aArgv[i]);
	}
	if(!args.empty()) {
		// check if first arg is exe
		if(std::filesystem::exists(std::filesystem::status(args.front()))) {
			args.pop_front();
		}
	}
	
	var_base* var = nullptr;
	while (!args.empty()) {
		const auto& arg = args.front();
		const bool shortName = arg.size() >= 2 ? arg[0] == '-' && isalpha(arg[1]) : false;
		const bool longName = arg.size() >= 2 ? arg[0] == '-' && arg[1] == '-' : false;
		if (shortName || longName) {
			// arg without value (cleared otherwise)
			if (var && var->isBool() && var->size() == 1) var->setValueStringInternal("");
			// find new arg
			if (longName) var = findVarByLongName(arg.substr(2));
			else if (shortName) var = findVarByShortName(arg.substr(1));
			// error if not found
			if (var == nullptr) getErrorDeque().emplace_back("'" + arg + "' not found");
		}
		// var found
		else if (var) {
			var->setValueStringInternal(arg);
			var = nullptr;
		}
		args.pop_front();
	}
	// var is last argument
	if (var && var->isBool() && var->size() == 1) var->setValueStringInternal("");
}

void ccli::loadConfig(const std::string& aCfgFile)
{
	std::map<std::string, std::string>& configMap = getConfigMap();
	configMap.clear();
	// check if file exists
	std::ifstream f(aCfgFile);

	// load config line by line
	while (f.good()) {
		std::string line;
		std::getline(f, line);
		// remove quotation marks etc
		line.erase(std::remove(line.begin(), line.end(), '\"'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '\''), line.end());
		const size_t pos = line.find(configDelimiter);
		if (pos != std::string::npos) {
			std::string token = line.substr(0, pos);
			std::string value = line.substr(pos + 1, line.size());
			var_base* var = findVarByLongName(token);
			// also check rd
			if (var && var->isConfigRead()) {
				var->setValueString(value);
				if (var->isConfigReadWrite()) configMap.insert({ token, value });
			}
			else configMap.insert({ token, value });
		}
	}
	f.close();
}

void ccli::writeConfig(const std::string& aCfgFile)
{
	std::map<std::string, std::string>& configMap = getConfigMap();
	const auto map = getLongNameVarMap();
	bool write = false;
	// update vars
	for (const auto& [fst, snd] : map)
	{
		if (snd->isConfigReadWrite()) {
			// also check if rdwr
			write |= doesConfigVarNeedUpdate(configMap, fst, snd->getValueString());
		}
	}
	if (!write) return;
	// create output string
	std::string out;
	for (const auto& [fst, snd] : configMap) {
		out += fst + configDelimiter;
		out += "\"" + snd + "\"\n";
	}
	// write config
	if (!out.empty()) writeConfigFile(aCfgFile, out);
}

void ccli::executeCallbacks()
{
	for (const auto var : getCallbackSet())
	{
		var->executeCallback();
	}
}

std::deque<std::string> ccli::getHelp()
{
	std::deque<std::string> help;
	const std::map<std::string, ccli::var_base*>& mapLong = getLongNameVarMap();
	const std::map<std::string, ccli::var_base*>& mapShort = getShortNameVarMap();

	for(const auto& var : mapShort)
	{
		std::string s = "  -" + var.second->getShortName();
		//if(!var.second->getLongName().empty()) s += " | --" + var.second->getLongName();
		if (s.size() > helpColumnWidthShort)
		{
			help.push_back(s);
			s.clear();
		}
		s += std::string(helpColumnWidthShort - s.size(), ' ');
		s += var.second->getDescription();
		help.push_back(s);
	}
	for (const auto& var : mapLong)
	{
		//if(var.second->getShortName().empty())
		{
			std::string s = "  --" + var.second->getLongName();
			if (s.size() > helpColumnWidthLong)
			{
				help.push_back(s);
				s.clear();
			}
			s += std::string(helpColumnWidthLong - s.size(), ' ');
			s += var.second->getDescription();
			help.push_back(s);
		}
	}
	return help;
}

std::deque<std::string> ccli::checkErrors()
{
	std::deque<std::string>& deque = getErrorDeque();
	std::deque<std::string> out = deque;
	deque.clear();
	return out;
}

/*
** var_base
*/
ccli::var_base::var_base(std::string aShortName, std::string aLongName, uint32_t aFlags,
	std::string aDescription, const bool aHasCallback) :
	mShortName(std::move(aShortName)), mLongName(std::move(aLongName)),
	mDescription(std::move(aDescription)), mFlags(aFlags), mHasCallback(aHasCallback), mLocked(false)
{
	assert(!mLongName.empty() || !mShortName.empty());
	addToVarList(mLongName, mShortName, this);
	if(mHasCallback) addToCallbackSet(this);
	/*if (mLongName.empty() && (isConfigRead() || isConfigReadWrite())) {
		getErrorDeque().emplace_back("Config requieres long name \"\'-" + mShortName + "\'");
	}*/
}

ccli::var_base::~var_base()
{
	removeFromVarList(mLongName, mShortName, this);
	if (mHasCallback) removeFromCallbackSet(this);
}

const std::string& ccli::var_base::getLongName() const
{ return mLongName; }

const std::string& ccli::var_base::getShortName() const
{ return mShortName; }

const std::string& ccli::var_base::getDescription() const
{ return mDescription; }

bool ccli::var_base::hasCallback() const
{ return mHasCallback; }

bool ccli::var_base::isCliOnly() const
{ return mFlags & CLI_ONLY; }

bool ccli::var_base::isReadOnly() const
{ return mFlags & READ_ONLY; }

bool ccli::var_base::isConfigRead() const
{ return !mLongName.empty() && mFlags & CONFIG_RD; }

bool ccli::var_base::isConfigReadWrite() const
{ return !mLongName.empty() && mFlags & CONFIG_RDWR; }

bool ccli::var_base::isCallbackAutoExecuted() const
{ return !(mFlags & MANUAL_EXEC); }

bool ccli::var_base::locked() const
{ return mLocked; }

void ccli::var_base::locked(const bool aLocked)
{ mLocked = aLocked; }

int ccli::var_base::parseInt(std::string_view token) {
	int value;
	auto result = std::from_chars(token.data(), token.data() + token.size(), value);
	if (result.ec == std::errc::invalid_argument) {
		getErrorDeque().emplace_back("'" + std::string{ token } + "' not convertible");
	}

	return value;
}

float ccli::var_base::parseFloat(std::string_view token) {
	float value;
	auto result = std::from_chars(token.data(), token.data() + token.size(), value);
	if (result.ec == std::errc::invalid_argument) {
		getErrorDeque().emplace_back("'" + std::string{ token } + "' not convertible");
	}

	return value;
}

bool ccli::var_base::parseBool(std::string_view token) {
	if (token.empty()) {
		return true;
	}

	bool value;
	std::string copiedToken{ token };

	std::istringstream{ copiedToken } >> std::boolalpha >> value;
	if (value) {
		return true;
	}

	std::istringstream{ copiedToken } >> value;
	return value;
}
