
#include "config.hpp"
#include "var_system.hpp"

var<float, 2, CONFIG_RDWR> var_vec2("test", "t", "Just a test", { 100,100 });

int main(int argc, char* argv[]) {
	var_system::parseArgs(argc, argv);
	config c;
	c.load("test.cfg");
	c.write("test.cfg");
	return 0;
}
