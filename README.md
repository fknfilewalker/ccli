# ccli

A static command line argument parsing library written in c++20

## Examples

### Basic
```c++
#include <ccli/ccli.h>
#include <iostream>

ccli::Var boolV {"b", "bool", false, ccli::CliOnly};  // bool, set with -b or --bool
ccli::Var uintV {"", "uint", 11u};                    // uint32_t, set with --uint
ccli::Var fvec2V{"", "fvec2", { 0.5f, 1.0f }};        // float2, set with --fvec2

int main(int argc, char* argv[]) {
  try {
    // parse cl arguments and update all registered vars
    // e.g. -b --uint 100 --fvec2 1.4f 1.9f
    ccli::parseArgs(argc, argv);
  }
  catch (ccli::CCLIError& e) {
    std::cout << "Caught error: " << e.message() << std::endl;
  }

  return 0;
}
```

### Load vars from config file and write changes back to it
```c++
#include <ccli/ccli.h>

ccli::Var<bool> boolVar{"b"sv, "boolVar"sv, false, ccli::ConfigRead};
ccli::Var<bool> boolVar2{"b2"sv, "boolVar2"sv, false, ccli::ConfigRdwr};

int main() {
  try {
    ccli::ConfigCache configCache = ccli::loadConfig("filename.ini");
    ccli::writeConfig("filename.ini", configCache);
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
ccli::Var myVar1{ "v1"sv, "var_1"sv, false };   // -> ccli::Var<bool>
ccli::Var myVar2{ "v2"sv, "var_2"sv, 0.0f };    // -> ccli::Var<float>
ccli::Var myVar3{ "v3"sv, "var_3"sv, 0l };      // -> ccli::Var<long int>
ccli::Var myVar4{ "v4"sv, "var_4"sv, "hello" }; // -> ccli::Var<std::string>


ccli::Var myVar5{ "v5"sv, "var_5"sv, { false, false } };      // -> ccli::Var<bool, 2>
ccli::Var myVar6{ "v6"sv, "var_6"sv, { 0.0f, 1.0f, 2.0f } };  // -> ccli::Var<float, 3>
```

### Value Limiting
Variable values can be clamped/limited by using `ccli::MaxLimit` and `ccli::MinLimit`. String values cannot be limited.
```c++
ccli::Var<float, 1, ccli::MinLimit<1.0f>> myVar1{ "v1"sv, "var_1"sv, 1.1f };  // -> scalar, always >= 1.0f
ccli::Var<long, 1, ccli::MaxLimit<100>>   myVar2{ "v2"sv, "var_2"sv, 50 };    // -> scalar, always <= 100
```

```c++
ccli::Var<
  float, 4,
  ccli::MinLimit<1.0f>
  ccli::MaxLimit<20.0f>
  > myVar1{ "v1"sv, "var_1"sv, {1.0f, 2.0f, 3.0f, 4.0f} };  // -> vector, each item always >= 1.0f and <= 20.0f
```

## Read variable value
Scalar variables can be automatically converted to their stored type.
```c++
ccli::Var<float> myVar1{ "v1"sv, "var_1"sv };

float value1= myVar1.value();
float value2= myVar1;
```

Vector variables allow for array indexing.
```c++
ccli::Var<float, 4> myVar1{ "v1"sv, "var_1"sv };

float value1= myVar1.value().at(1);
float value2= myVar1[1];
```

## Store variable value
If a variable is not read-only, cli-only or locked, its value can be set.
```c++
ccli::Var<float> myVar1{ "v1"sv, "var_1"sv };
myVar1.value( 10.0f );

ccli::Var<float, 4> myVar2{ "v2"sv, "var_2"sv };
myVar2.value({ 10.0f, 20.0f, 30.0f, 40.0f });
```

Variable values can also be set by parsig string values the same way as if they were provided via the CLI.
```c++
ccli::Var<float> myVar1{ "v1"sv, "var_1"sv };
myVar1.VarBase::valueString( "10" );

ccli::Var<float, 4> myVar2{ "v2"sv, "var_2"sv };
myVar2.VarBase::valueString("10,20,30,40");
```

## Type erasure
Handling the complicated templated types of variables is simplified by using the `VarBase` base class.

### Type checking
```c++
ccli::Var<float> myVar1{ "v1"sv, "var_1"sv };

ccli::VarBase& v= myVar1;
v.isIntegral();       // -> false
v.isFloatingPoint();  // -> true
v.isBool();           // -> false
v.isString();         // -> false
```

### Type casting
Type casting happens as fallible conversions to a desired type using virtual methods. Bools, floating point and integer numbers can be converted into each other. String values cannot be converted into another type. In case of a bad conversion, an empty `std::optional` is returned.
```c++
ccli::Var<float> myVar1{ "v1"sv, "var_1"sv, 2.0 };

ccli::VarBase& v= myVar1;
v.asInt();            // -> 2ll
v.asFloat();          // -> 2.0
v.asBool();           // -> true
v.asString();         // -> {empty optional}
```

```c++
ccli::Var<std::string> myVar1{ "v1"sv, "var_1"sv, "hello" };

ccli::VarBase& v= myVar1;
v.asInt();            // -> {empty optional}
v.asFloat();          // -> {empty optional}
v.asBool();           // -> {empty optional}
v.asString();         // -> "hello"
```

```c++
ccli::Var<float, 4> myVar2{ "v2"sv, "var_2"sv, { 10.0f, 20.0f, 30.0f, 40.0f } };

ccli::VarBase& v= myVar1;
v.asInt(2);            // -> 30ll
v.asFloat(2);          // -> 30.0
v.asBool(2);           // -> true
v.asString(2);         // -> {empty optional}
```

### To string
```c++
ccli::Var<float, 4> myVar2{ "v2"sv, "var_2"sv, { 10.0f, 20.0f, 30.0f, 40.0f } };

ccli::VarBase& v= myVar1;
v.valueString();    // -> "10,20,30,40"
```

## Variable registry
Variables are automatically added to a global registery upon creation. If a variable with the same name already exists a `DuplicatedVarNameError` is thrown. If a variable goes out of scope, it is removed from the registry.

### Argument parsing
When parsing CLI arguments from a `argc`-`argv` array all currently registered variables are considered to be CLI options. Bool variables do not require a value for the option: Any mentioned bool option is set to `true`. Numeric and string variables require a value, else a `ccli::MissingValueError` is thrown.
```c++
try {
  ccli::parseArgs(argc, argv);
}
catch (ccli::CCLIError& e) {
  std::cout << "Caught error: " << e.message() << std::endl;
}
```

### Iterating
All currently registerd variables can be iterated over using `forEachVar`.

```c++
// Print all registered variables
ccli::forEachVar([](ccli::VarBase& var, const size_t idx) -> ccli::IterationDecision {
  std::cout << "Var #" << idx << " - " << var.longName() << " " << var.valueString() << std::endl;
  return {};
});

```

```c++
// Is there a variable with a 'false' value?
auto result= ccli::forEachVar([](ccli::VarBase& var, const size_t idx) -> ccli::IterationDecision {
  auto val= var.asBool();
  if( val.has_value() && *val == false) {
    return ccli::IterationDecision::Break;    
  }
  return {};
});

bool hasFalseVar= result == ccli::IterationDecision::Break;
```
## Errors

- `ccli::CCLIError` Base class for all errors thrown by CCLI.

- `ccli::DuplicatedVarNameError` Thrown if two variables with either the same short- or long name are registered.

- `ccli::FileError` Thrown if a config file could not be opened for writing.

- `ccli::UnknownVarNameError` Thrown if a CLI option references an unknown variable name during parsing.

- `ccli::MissingValueError` Thrown if a non-bool CLI option is missing a value during parsing.

- `ccli::ConversionError` Thrown if a value string cannot be converted to the variables type.
