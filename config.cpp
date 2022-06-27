#include "config.hpp"
#include "var_system.hpp"

#include <fstream>

namespace 
{
	bool write_file(std::string const& aFilename, std::string const& aContent)
	{
		std::ofstream file(aFilename, std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			ccli::log::warning("Could not open file '" + aFilename + "' for writing");
			return false;
		}
		file.write(aContent.c_str(), static_cast<std::streamsize>(aContent.size()));
		file.close();
		return true;
	}
}

config::config(): mDelimiter("=")
{}

void config::load(const std::string& aCfgFile)
{
	mConfigPairs.clear();
	// check if file exists
	std::ifstream f(aCfgFile);

	// load config line by line
	while (f.good()) {
		std::string line;
		std::getline(f, line);
		// remove quotation marks etc
		line.erase(std::remove(line.begin(), line.end(), '\"'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '\''), line.end());
		const size_t pos = line.find(mDelimiter);
		if (pos != std::string::npos) {
			std::string token = line.substr(0, pos);
			std::string value = line.substr(pos + 1, line.size());
			var_base* var = var_system::findVarByLongName(token);
			// also check rd
			if (var && var->isConfigRead()) {
				var->setValueString(value);
				if (var->isConfigReadWrite()) mConfigPairs.insert({ token, value });
			} else mConfigPairs.insert({ token, value });

		}
	}
	f.close();
}
void config::write(const std::string& aCfgFile)
{
	const auto map = var_system::getLongNameMap();
	bool write = false;
	// update vars
	for(auto pair : map)
	{
		if (pair.second->isConfigReadWrite()) {
			// also check if rdwr
			write |= update(pair.first, pair.second->getValueString());
		}
	}
	if (!write) return;
	// create output string
	std::string out;
	for (auto const& cfg : mConfigPairs) {
		out += cfg.first + mDelimiter + "\"" + cfg.second + "\"\n";
	}
	// write config
	if (!out.empty()) write_file(aCfgFile, out);
}

bool config::update(const std::string& aToken, const std::string& aValue)
{
	const auto it = mConfigPairs.find(aToken);
	if (it != mConfigPairs.end() && it->second != aValue) {
		it->second = aValue;
		return true;
	}
	if(it == mConfigPairs.end())
	{
		mConfigPairs.insert({ aToken, aValue });
		return true;
	}
	return false;
}
