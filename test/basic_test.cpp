#include <ccli/ccli.h>
#include <iostream>
#include <cassert>
#include <string_view>
#include <array>
#include <iostream>
#include <fstream>

using namespace std::literals;

void basicBoolTest()
{
	ccli::Var<bool> boolVar1("b1"sv, "bool1"sv, 0, ccli::None, "First bool Var"sv);
	ccli::Var<bool> boolVar2("b2"sv, "bool2"sv, false, ccli::None, "Second bool Var"sv);
	ccli::Var<bool> boolVar3("b3"sv, "bool3"sv, 1, ccli::None, "Third bool Var"sv);
	ccli::Var<bool> boolVar4("b4"sv, "bool4"sv, true, ccli::None, "Fourth bool Var"sv);

	assert(boolVar1.value() == false);
	assert(boolVar2.value() == false);
	assert(boolVar3.value() == true);
	assert(boolVar4.value() == true);

	try {
		const char* argv[] = { "-b1", "1", "-b2", "true", "-b3", "0", "-b4", "false" };
		ccli::parseArgs(std::size(argv), argv);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	assert(boolVar1.value() == true);
	assert(boolVar2.value() == true);
	assert(boolVar3.value() == false);
	assert(boolVar4.value() == false);

	assert(boolVar1 == true);
	assert(boolVar2 == true);
	assert(boolVar3 == false);
	assert(boolVar4 == false);

	try {
		const char* argv[] = { "-b1", "0", "-b2", "-b3", "--bool4" };
		ccli::parseArgs(std::size(argv), argv);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	assert(boolVar1.value() == false);
	assert(boolVar2.value() == true);
	assert(boolVar3.value() == true);
	assert(boolVar4.value() == true);
}

void immutableTest()
{
	ccli::Var<uint32_t> readOnlyVar(""sv, "readOnly"sv, 111, ccli::ReadOnly);
	ccli::Var<uint32_t> cliOnlyVar(""sv, "cliOnly"sv, 222, ccli::CliOnly);
	ccli::Var<uint32_t> lockedVar(""sv, "locked"sv, 333, ccli::Locked);
	assert(readOnlyVar.value() == 111);
	assert(cliOnlyVar.value() == 222);
	assert(lockedVar.value() == 333);

	assert(readOnlyVar == 111);
	assert(cliOnlyVar == 222);
	assert(lockedVar == 333);

	try {
		const char* argv[] = { "--readOnly", "1", "--cliOnly", "2", "--locked", "3"};
		ccli::parseArgs(std::size(argv), argv);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	assert(readOnlyVar.value() == 111);
	assert(cliOnlyVar.value() == 2);
	assert(lockedVar.value() == 333);

	readOnlyVar.value(1111);
	cliOnlyVar.value(2222);
	lockedVar.value(3333);
	assert(readOnlyVar.value() == 111);
	assert(cliOnlyVar.value() == 2);
	assert(lockedVar.value() == 333);

	lockedVar.unlock();
	lockedVar.value(3333);
	assert(lockedVar.value() == 3333);
}

void arrayTest()
{
	ccli::Var<uint32_t, 3> uvec3Var("uvec3"sv, ""sv, { 1, 2, 3 });
	ccli::Var<std::string, 2> stringVar("string"sv, ""sv, { "This is a test", "really" });
	ccli::Var<uint8_t, 2, ccli::MaxLimit<2>> limitVar("limit"sv, ""sv, { 3, 4 });

	assert(uvec3Var.value().at(0) == 1 && uvec3Var.value().at(1) == 2 && uvec3Var.value().at(2) == 3);
	assert(uvec3Var[0] == 1 && uvec3Var[1] == 2 && uvec3Var[2] == 3);
	assert(stringVar.value().at(0) == "This is a test" && stringVar.value().at(1) == "really");
	assert(stringVar[0] == "This is a test" && stringVar[1] == "really");
	assert(limitVar.value().at(0) == 2 && limitVar.value().at(1) == 2);
	assert(limitVar[0] == 2 && limitVar[1] == 2);

	try {
		const char* argv[] = { "-uvec3", "5,6,7", "-string", "This is not a test,or is it", "-limit", "100,200" };
		ccli::parseArgs(std::size(argv), argv);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	assert(uvec3Var.value().at(0) == 5 && uvec3Var.value().at(1) == 6 && uvec3Var.value().at(2) == 7);
	assert(uvec3Var[0] == 5 && uvec3Var[1] == 6 && uvec3Var[2] == 7);
	assert(stringVar.value().at(0) == "This is not a test" && stringVar.value().at(1) == "or is it");
	assert(stringVar[0] == "This is not a test" && stringVar[1] == "or is it");
	assert(limitVar.value().at(0) == 2 && limitVar.value().at(1) == 2);
	assert(limitVar[0] == 2 && limitVar[1] == 2);
}

void lambdaCallbackTest() {
	float value = 0.0f;
	ccli::Var<float> lambdaVar("lambda"sv, ""sv, 100.0f, 0, ""sv,
		[&](const float v) {
			value = v;
		}
	);
	assert(value == 0.0f);
	float value2 = 0.0f;
	ccli::Var<float> lambdaVar2("lambdaLazy"sv, ""sv, 100.0f, ccli::Flag::ManualExec, ""sv,
		[&](const float v) {
			value2 = v;
		}
	);

	std::array<float, 3> values3 = { 0.0f, 0.0f,0.0f };
	ccli::Var<float, 3> lambdaVar3("arrayLambda"sv, ""sv, { 100.0f , 200.0f, 300.0f }, 0, ""sv,
		[&](std::span<const float> v) {
			assert(v.size() == values3.size());
			std::ranges::copy(v, values3.begin());
		}
	);

	try {
		const char* argv[] = { "-lambda", "222", "-lambdaLazy", "222", "-arrayLambda", "1,2,3"};
		ccli::parseArgs(std::size(argv), argv);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	assert(std::abs(222.0f - value) < std::numeric_limits<float>::epsilon());
	assert(std::abs(1.0f - values3[0]) < std::numeric_limits<float>::epsilon());
	assert(std::abs(2.0f - values3[1]) < std::numeric_limits<float>::epsilon());
	assert(std::abs(3.0f - values3[2]) < std::numeric_limits<float>::epsilon());

	lambdaVar.value(300.0f);
	assert(std::abs(300.0f - value) < std::numeric_limits<float>::epsilon());

	assert(value2 == 0.0f);
	ccli::executeCallbacks();
	assert(std::abs(222.0f - value2) < std::numeric_limits<float>::epsilon());

	value2 = 0.0f;
	ccli::executeCallbacks();
	assert(value2 == 0.0f);
}

void exceptionTest() {
	try {
		throw ccli::FileError{ "a/file/name" };
	}
	catch (const std::exception& e) {
		assert(e.what() && strlen(e.what()));
	}

	{
		bool didCatch = false;
		ccli::Var<float, 1> floatVar("float", "", 0.0);
		try {
			const char* argv[] = { "-float", "badValue" };
			ccli::parseArgs(std::size(argv), argv);
		}
		catch (const ccli::ConversionError& e) {
			didCatch = true;
			assert(e.unconvertibleValueString() == "badValue"sv);
			assert(&e.variable() == &floatVar);
			assert(e.what() && strlen(e.what()));
			assert(not e.message().empty());
		}
		catch (...) {
			assert(false);
		}

		assert(didCatch);
	}

	{
		bool didCatch = false;
		try {
			const char* argv[] = { "-aBadVariableNameWhichDoesNotExist", "someValue" };
			ccli::parseArgs(std::size(argv), argv);
		}
		catch (const ccli::UnknownVarNameError& e) {
			didCatch = true;
			assert(e.unknownName() == "-aBadVariableNameWhichDoesNotExist"sv);
			assert(e.what() && strlen(e.what()));
			assert(not e.message().empty());
		}
		catch (...) {
			assert(false);
		}

		assert(didCatch);
	}

	{
		// Numerics require a value
		bool didCatch = false;
		try {
			ccli::Var<float, 1> floatVar("float", "", 0.0);
			const char* argv[] = { "-float" };
			ccli::parseArgs(std::size(argv), argv);
		}
		catch (const ccli::MissingValueError& e) {
			didCatch = true;
			assert(e.variable() == "-float"sv);
			assert(e.what() && strlen(e.what()));
			assert(not e.message().empty());
		}
		catch (...) {
			assert(false);
		}

		assert(didCatch);
	}

	{
		// Bools do not require a value
		ccli::Var<bool> boolVar("bool", "", false);
		try {
			const char* argv[] = { "-bool" };
			ccli::parseArgs(std::size(argv), argv);
		}
		catch (...) {
			assert(false);
		}

		assert(boolVar == true);
	}

	{
		// Check that parsing errors are deferred
		const char* argv[] = { "-aBadVariableNameWhichDoesNotExist", "someValue", "-anotherBadVariableName", "-goodVarName", "123", "234"};
		ccli::Var<float, 2> floatVar("goodVarName", "", { 0.0f, 0.0f } );
		try {
			ccli::parseArgs(std::size(argv), argv);
		}
		catch (const ccli::UnknownVarNameError& e) {
			assert(e.unknownName() == "-aBadVariableNameWhichDoesNotExist"sv);
		}
		catch (...) {
			assert(false);
		}

		assert(std::abs(floatVar.value()[0] - 123.0f) < std::numeric_limits<float>::epsilon());
		assert(std::abs(floatVar.value()[1] - 234.0f) < std::numeric_limits<float>::epsilon());
	}

	{
		bool didCatch = false;
		try {
			ccli::Var<float, 1> floatVar1("f1", "float", 0.0);
			ccli::Var<float, 1> floatVar2("f2", "float", 0.0);
		}
		catch (const ccli::DuplicatedVarNameError& e) {
			didCatch = true;
			assert(e.duplicatedName() == "float"sv);
			assert(e.what() && strlen(e.what()));
			assert(not e.message().empty());
		}
		catch (...) {
			assert(false);
		}

		assert(didCatch);
	}
}

void configTest()
{
	ccli::Var<uint32_t, 2> uint2Var(""sv, "uint2"sv, { 100, 200 }, ccli::ConfigRdwr);
	ccli::Var<std::string> stringVar(""sv, "string"sv, { "This is a string" }, ccli::ConfigRdwr);

	static constexpr auto filename = "test/configTest.cfg";
	try {
		ccli::writeConfig(filename);
		uint2Var.value({ 1, 1 });
		stringVar.value("empty");
		ccli::loadConfig(filename);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	if (remove(filename) != 0)
	{
		std::cout << "Error deleting file" << std::endl;
	}

	constexpr std::array<uint32_t, 2> result = { 100, 200 };
	assert(uint2Var.value() == result);
	assert(stringVar.value() == "This is a string");
}

void configTest2()
{
	ccli::Var<uint32_t> uintVar(""sv, "uint"sv, 100, ccli::ConfigRead);
	ccli::Var<std::string> stringVar(""sv, "string"sv, { "This is a string" }, ccli::ConfigRdwr);

	assert(uintVar.value() == 100);
	assert(stringVar.value() == "This is a string");

	static constexpr auto filename = "test/configTest2.cfg";
	{
		std::ofstream cfgfile;
		cfgfile.open(filename);
		cfgfile << "string=\"This is a test\"\nuint=\"150\"\n";
		cfgfile.close();
	}
	try {
		ccli::ConfigCache cache = ccli::loadConfig(filename);

		assert(uintVar.value() == 150);
		assert(stringVar.value() == "This is a test");

		uintVar.value(200);
		stringVar.value("This is a joke");
		assert(uintVar.value() == 200);
		assert(stringVar.value() == "This is a joke");

		ccli::writeConfig(filename, cache);
		{
			std::stringstream cfgfile;
			cfgfile << std::ifstream(filename).rdbuf();
			assert(cfgfile.view() == "string=\"This is a joke\"\nuint=\"150\"\n");
		}
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	if (remove(filename) != 0)
	{
		std::cout << "Error deleting file" << std::endl;
	}
}

void registeredVarTest()
{
	ccli::Var<float, 4, ccli::MaxLimit<1>, ccli::MinLimit<-1>> float4Var("f1", "float1", { 0 }, ccli::None, "First bool Var");
	ccli::Var<float, 4> float4Var2("f2", "float2", { 0.0f }, ccli::None, "First bool Var");
	ccli::Var<float, 2> float2Var{"t"sv, "test"sv, { 100.0f, 200.0f }, ccli::ConfigRdwr };
	ccli::Var<short, 1, ccli::MaxLimit<500>> shortVar("s", "short", 0);
	ccli::Var<bool> boolVar("b", "bool1", false);

	ccli::Var<std::string> stringVar("str1", "string1", "A cool value");
	ccli::Var<std::string> stringVar2(std::string{ "str2" }, "string2", "Another cool value");

	std::cout << "Currently registerd variables..." << std::endl;
	ccli::forEachVar([](ccli::VarBase& var, const size_t idx) -> ccli::IterationDecision {
		std::cout << "... " << idx << " - " << var.longName() << " " << var.valueString() << std::endl;
		return {};
		});

	std::cout << *float2Var.asBool() << std::endl;
	std::cout << *float2Var.asInt() << std::endl;
	std::cout << *float2Var.asFloat() << std::endl;
	std::cout << float2Var.asString().has_value() << std::endl;
	std::cout << *stringVar.asString() << std::endl;
}

void deductionTest() {
	ccli::Var myVar{ "myVar"sv, ""sv, 1.0f };
	static_assert( std::is_same_v<decltype(myVar), ccli::Var<float>>, "Could not deduce ccli::Var<float>");

	auto floatLambda = [](float data) {};

	ccli::Var myVar2{ "myVar2"sv, ""sv, 1.0f, 0, ""sv, floatLambda };
	static_assert(std::is_same_v<decltype(myVar2), ccli::Var<float>>, "Could not deduce ccli::Var<float>");


	ccli::Var myVarArray{ "myArray"sv, ""sv, { 1.0f, 2.0f, 3.0f, 4.0f } };
	static_assert(std::is_same_v<decltype(myVarArray), ccli::Var<float, 4>>, "Could not deduce ccli::Var<float, 4>");

	auto longArrayLambda = [](std::span<const long> data) {};

	ccli::Var myVarArray2{ "myArray2"sv, ""sv, { 1l, 2l, 3l }, 0, ""sv, longArrayLambda };
	static_assert(std::is_same_v<decltype(myVarArray2), ccli::Var<long, 3>>, "Could not deduce ccli::Var<long, 3>");
}

void multiValueParsing() {
	try {
		ccli::Var<float, 9> myVar1{ "float"sv, ""sv };
		ccli::Var<bool> myVar2{ "b"sv, ""sv };
		ccli::Var<int, 4> myVar3{ "int"sv, ""sv };
		const char* argv[] = { "-float", "1,2,3", "4,5,6", "7,8,9", "-b", "-int", "1,2", "3,4"};
		ccli::parseArgs(std::size(argv), argv);

		uint32_t x = 1;
		for (auto f : myVar1.value()) {
			assert(static_cast<uint32_t>(f) == x++);
		}

		assert(myVar2.value() == true);

		int z = 1;
		for (auto i : myVar3.value()) {
			assert(i == z++);
		}
	}
	catch (...) {
		assert(false);
	}
}

void settingVariableTest() {
	ccli::Var<float> myVar1{ "float1"sv, ""sv, 1.0f };

	myVar1.value(22.0f);
	assert(std::abs(myVar1 - 22.0f) < std::numeric_limits<float>::epsilon());

	ccli::Var<float, 3> myVar2{ "float2"sv, ""sv, {1.0f, 2.0f, 3.0f} };

	myVar1.VarBase::valueString("123");
	assert(std::abs(myVar1 - 123.0f) < std::numeric_limits<float>::epsilon());

	myVar2.value({ 10.0f, 20.0f, 30.0f});
	assert(std::abs(myVar2[0] - 10.0f) < std::numeric_limits<float>::epsilon());
	assert(std::abs(myVar2[1] - 20.0f) < std::numeric_limits<float>::epsilon());
	assert(std::abs(myVar2[2] - 30.0f) < std::numeric_limits<float>::epsilon());

	myVar2.VarBase::valueString("1,2,3"sv);
	assert(std::abs(myVar2[0] - 1.0f) < std::numeric_limits<float>::epsilon());
	assert(std::abs(myVar2[1] - 2.0f) < std::numeric_limits<float>::epsilon());
	assert(std::abs(myVar2[2] - 3.0f) < std::numeric_limits<float>::epsilon());
}

void tryStoreTest() {
	ccli::Var<float> myVar1{ "float1"sv, ""sv, 1.0f };
	ccli::Var<std::string> myVar2{ "string1"sv, ""sv, "someValue"};

	{
		assert(myVar1.tryStoreNumeric<float>(2.0f));
		assert(2.0f - myVar1 < std::numeric_limits<float>::epsilon());

		ccli::VarBase& var = myVar1;
		assert(var.tryStore(3.0));
		assert(3.0f - myVar1 < std::numeric_limits<float>::epsilon());

		assert(!var.tryStore(std::string{ "hello world" }));
	}

	{
		assert(!myVar2.tryStoreNumeric<float>(2.0f));

		ccli::VarBase& var = myVar2;
		assert(!var.tryStore(3.0));

		assert(var.tryStore(std::string{ "hello world" }));
		assert(myVar2.value() == "hello world");
	}
}

int main(int argc, char* argv[]) {
	basicBoolTest();
	immutableTest();
	arrayTest();
	lambdaCallbackTest();
	exceptionTest();
	configTest();
	configTest2();
	registeredVarTest();
	deductionTest();
	multiValueParsing();
	settingVariableTest();
	tryStoreTest();

	return 0;
}
