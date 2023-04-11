# ccli

A static command line argument parsing library written in c++20

## Examples

### Basic
```c++
#include <ccli/ccli.h>
// optional
#include <cassert>
#include <string_view>
using namespace std::literals;

// declares a bool var that can be set with either -b or --bool
ccli::Var<bool> boolVar{"b"sv, "bool"sv, false, ccli::None, "First bool Var"sv};
// declares a uint32_t var (type decution) that can be set with --uint
ccli::Var uintVar{""sv, "uint"sv, 11u};
// declares a float2 var that can be set with --fvec2
ccli::Var<float, 2> fvec2Var{""sv, "fvec2"sv, { 0.5f, 1.0f }};

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

### Load vars from config file
```c++
#include <ccli/ccli.h>

ccli::Var<bool> boolVar{"b"sv, "boolVar"sv, false, ccli::ConfigRead, };

int main() {
	try {
		ccli::loadConfig("filename.ini");
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
}

```

## Variable Declaration

### Scalar and vector variables
```c++
ccli::Var<float>    myVar1{ "v1"sv, "var_1"sv };  // -> scalar
ccli::Var<float, 1> myVar2{ "v2"sv, "var_2"sv };  // -> scalar
ccli::Var<float, 4> myVar3{ "v3"sv, "var_3"sv };  // -> vector
```

Add init values:
```c++
ccli::Var<float>    myVar1{ "v1"sv, "var_1"sv, 0.0f };
ccli::Var<float, 1> myVar2{ "v2"sv, "var_2"sv, 0.0f };
ccli::Var<float, 4> myVar3{ "v3"sv, "var_3"sv, {0.0f, 1.0f} };
```

### Type deduction
In most simple cases the variable type can be deduced:
```c++
ccli::Var myVar1{ "v1"sv, "var_1"sv, false };		// -> ccli::Var<bool>
ccli::Var myVar2{ "v2"sv, "var_2"sv, 0.0f };  	// -> ccli::Var<float>
ccli::Var myVar3{ "v3"sv, "var_3"sv, 0l };  		// -> ccli::Var<long int>
ccli::Var myVar4{ "v4"sv, "var_4"sv, "hello" }; // -> ccli::Var<std::string>


ccli::Var myVar5{ "v5"sv, "var_5"sv, { false, false } };			// -> ccli::Var<bool, 2>
ccli::Var myVar6{ "v6"sv, "var_6"sv, { 0.0f, 1.0f, 2.0f } };	// -> ccli::Var<float, 3>
```

### Value Limiting
Variable values can be clamped/limited by using `ccli::MaxLimit` and `ccli::MinLimit`. String values cannot be limited.
```c++
ccli::Var<float, 1, ccli::MinLimit<1.0f>> myVar1{ "v1"sv, "var_1"sv, 1.1f };  // -> scalar, always >= 1.0f
ccli::Var<long, 1, ccli::MaxLimit<100>>   myVar2{ "v2"sv, "var_2"sv, 50 };  	// -> scalar, always <= 100
```

```c++
ccli::Var<
	float, 4,
	ccli::MinLimit<1.0f>
	ccli::MaxLimit<20.0f>
	> myVar1{ "v1"sv, "var_1"sv, {1.0f, 2.0f, 3.0f, 4.0f} };  // -> vector, each item always >= 1.0f and <= 20.0f
```
