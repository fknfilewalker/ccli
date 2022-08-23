
#include "ccli.hpp"
#include <iostream>

using namespace ccli;

ccli::var<float, 2, CONFIG_RDWR> var_test("test", "t", "Just a test", { 100,100 });
ccli::var<float, 2, CONFIG_RDWR> var_test2("hallo", "", "Hallo", { 100,100 });

void print(const std::array<float, 2> aValue)
{
	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
}
ccli::var<float, 2> var_cbtest("cbtest", "cbt", "Just a test", { 100,100 }, print);

ccli::var<float, 2> var_cbtestlambda("cbtestlambda", "cbtl", "Just a test", { 100,100 }, [](const std::array<float, 2> aValue) {
	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
	});

ccli::var<bool, 1> var_nix("", "nix", "Just a test", { });


int main(int argc, char* argv[]) {
	ccli::parseArgs(argc, argv);
	ccli::loadConfig("test.cfg");
	ccli::writeConfig("test.cfg");
	ccli::printHelp();
	return 0;
}
