#include "ccli.hpp"
#include <iostream>


ccli::var<float, 2, ccli::CONFIG_RDWR> var_test("test", "t", "Just a test", { 100,100 });
ccli::var<float, 2, ccli::CONFIG_RDWR> var_test2("test", "", "Hallo", { 100,100 });

void print(const std::array<float, 2> aValue)
{
	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
}
ccli::var<float, 2> var_cbtest("cbtest", "cbt", "Just a test", { 100,100 }, print);

ccli::var<float, 2> var_cbtestlambda("cbtestlambda", "cbtl", "Just a test", { 100,100 }, [](const std::array<float, 2> aValue) {
	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
	});

ccli::var<bool> var_nix("", "nix", "Just a test", { });
ccli::var<bool> var_nix2("", "nix2", "Just a test", { });


int main(int argc, char* argv[]) {
	ccli::parseArgs(argc, argv);
	ccli::loadConfig("test.cfg");
	ccli::writeConfig("test.cfg");
	std::cout << var_nix.getValue()[0] << " " << var_nix2.getValue()[0] << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << s << std::endl;
	for (const std::string& s : ccli::getHelp()) std::cout << s << std::endl;
	return 0;
}
