#include "var_system.hpp"

#include <sstream>
#include <deque>

std::map<std::string, var_base*>& var_system::getLongNameMap()
{
	static std::map<std::string, var_base*> map;
	return map;
}

std::map<std::string, var_base*>& var_system::getShortNameMap()
{
	static std::map<std::string, var_base*> map;
	return map;
}

std::set<var_base*>& var_system::getCallbackSet()
{
	static std::set<var_base*> set;
	return set;
}

void var_system::addToCallbackSet(var_base* aVar)
{
	auto& set = getCallbackSet();
	set.insert(aVar);
}

void var_system::removeFromCallbackSet(var_base* aVar)
{
	auto& set = getCallbackSet();
	set.erase(aVar);
}

var_base* var_system::findVarByLongName(const std::string& aLongName)
{
	std::map<std::string, var_base*>& map = getLongNameMap();
	const auto it = map.find(aLongName);
	if (it != map.end()) return it->second;
	return nullptr;
}

var_base* var_system::findVarByShortName(const std::string& aShortName)
{
	std::map<std::string, var_base*>& map = getShortNameMap();
	const auto it = map.find(aShortName);
	if (it != map.end()) return it->second;
	return nullptr;
}

void var_system::executeCallbacks()
{
	for(const auto var : getCallbackSet())
	{
		var->executeCallback();
	}
}

void var_system::parseArgs(const int aArgc, char* const aArgv[])
{
	std::deque<std::string> args;
	for (int i = 0; i < aArgc; i++) {
		args.emplace_back(aArgv[i]);
	}

	var_base* var = nullptr;
	while (!args.empty())
	{
		const auto& arg = args.front();
		const bool shortName = !arg.empty() ? arg[0] == '-' : false;
		const bool longName = arg.size() >= 2 ? shortName && arg[1] == '-' : false;
		if (shortName || longName) {
			if (var || arg.size() == 1) var->setValueString(""); // if no value follows just send empty string to var
			if (longName) var = findVarByLongName(arg.substr(2));
			else if (shortName) var = findVarByShortName(arg.substr(1));
		}
		else if (var)
		{
			var->setValueString(arg);
			var = nullptr;
		}
		args.pop_front();
	}
}
