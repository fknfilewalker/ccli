#pragma once

#include <string>
#include <map>

class config {
public:
										config();

	void								load(const std::string& aCfgFile);
	void								write(const std::string& aCfgFile);
private:
	bool								update(const std::string& aToken, const std::string& aValue);

	const std::string					mDelimiter;
	std::map<std::string, std::string>	mConfigPairs;
};
