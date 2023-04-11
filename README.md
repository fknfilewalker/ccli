# ccli

A static command line argument parsing library written in c++20
# Examples

### Basic
```c++
#include <ccli/ccli.h>
// optional
#include <cassert>
#include <string_view>
using namespace std::literals;

// declares a bool var that can be set with either -b or --bool
ccli::Var<bool> boolVar("b"sv, "bool"sv, false, ccli::None, "First bool Var"sv);
// declares a uint32_t var (type decution) that can be set with --uint
ccli::Var uintVar(""sv, "uint"sv, 11u);
// declares a float2 var that can be set with --fvec2
ccli::Var<float, 2> fvec2Var(""sv, "fvec2"sv, { 0.5f, 1.0f });

// argv: -b --uint 100 --fvec2 1.4f 1.9f
int main(int argc, char* argv[]) {
	try {
		// parse cl arguments and update all registered vars
		ccli::parseArgs(argc, argv);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}

	assert(boolVar.value());
	assert(uintVar.value() == 100);
	constexpr float epsilon = std::numeric_limits<float>::epsilon();
	assert(std::abs(fvec2Var.value().at(0) - 1.4f) < epsilon);
	assert(std::abs(fvec2Var.value().at(1) - 1.9f) < epsilon);

	return 0;
}
```
