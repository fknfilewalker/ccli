
#include "config.hpp"
#include "var_system.hpp"

var<float, 2, CONFIG_RDWR> var_test("test", "t", "Just a test", { 100,100 });
var<float, 2, CONFIG_RDWR> var_cbtest("cbtest", "cbt", "Just a test", { 100,100 }, [](std::array<float, 2> aValue) {
	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
});

int main(int argc, char* argv[]) {
	var_system::parseArgs(argc, argv);
	config c;
	c.load("test.cfg");
	c.write("test.cfg");
	return 0;
}
