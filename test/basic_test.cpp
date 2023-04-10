#include <ccli/ccli.h>
#include <iostream>
#include <cassert>
#include <string_view>

using namespace std::literals;

void basicBoolTest()
{
	ccli::Var<bool> boolVar1("b1"sv, "bool1"sv, 0, ccli::NONE, "First bool Var"sv);
	ccli::Var<bool> boolVar2("b2"sv, "bool2"sv, false, ccli::NONE, "Second bool Var"sv);
	ccli::Var<bool> boolVar3("b3"sv, "bool3"sv, 1, ccli::NONE, "Third bool Var"sv);
	ccli::Var<bool> boolVar4("b4"sv, "bool4"sv, true, ccli::NONE, "Fourth bool Var"sv);

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
	ccli::Var<uint32_t> readOnlyVar(""sv, "readOnly"sv, 111, ccli::READ_ONLY);
	ccli::Var<uint32_t> cliOnlyVar(""sv, "cliOnly"sv, 222, ccli::CLI_ONLY);
	ccli::Var<uint32_t> lockedVar(""sv, "locked"sv, 333, ccli::LOCKED);
	assert(readOnlyVar.value() == 111);
	assert(cliOnlyVar.value() == 222);
	assert(lockedVar.value() == 333);

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
	assert(stringVar.value().at(0) == "This is a test" && stringVar.value().at(1) == "really");
	assert(limitVar.value().at(0) == 2 && limitVar.value().at(1) == 2);

	try {
		const char* argv[] = { "-uvec3", "5,6,7", "-string", "This is not a test,or is it", "-limit", "100,200" };
		ccli::parseArgs(std::size(argv), argv);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	assert(uvec3Var.value().at(0) == 5 && uvec3Var.value().at(1) == 6 && uvec3Var.value().at(2) == 7);
	assert(stringVar.value().at(0) == "This is not a test" && stringVar.value().at(1) == "or is it");
	assert(limitVar.value().at(0) == 2 && limitVar.value().at(1) == 2);
}

void lambdaCallbackTest() {
	float value = 0.0f;
	ccli::Var<float> lambdaVar("lambda"sv, ""sv, 100.0f, 0, ""sv,
		[&](const ccli::Storage<float>& v) {
			value = v.at(0);
		}
	);
	assert(value == 0.0f);
	float value2 = 0.0f;
	ccli::Var<float> lambdaVar2("lambdaLazy"sv, ""sv, 100.0f, ccli::Flag::MANUAL_EXEC, ""sv,
		[&](const ccli::Storage<float>& v) {
			value2 = v.at(0);
		}
	);

	try {
		const char* argv[] = { "-lambda", "222", "-lambdaLazy", "222" };
		ccli::parseArgs(std::size(argv), argv);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
	assert(std::abs(222.0f - value) < std::numeric_limits<float>::epsilon());

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

int main(int argc, char* argv[]) {
	basicBoolTest();
	immutableTest();
	arrayTest();
	lambdaCallbackTest();
	exceptionTest();

	ccli::Var<float, 4, ccli::MaxLimit<1>, ccli::MinLimit<-1>> float_var1("f1", "float1", {0}, ccli::NONE, "First bool Var");
	ccli::Var<float, 4> float_var2("f2", "float2", {0}, ccli::NONE, "First bool Var");
	ccli::Var<float, 2> var_test("t", "test", { 100, 200 }, ccli::CONFIG_RDWR);
	ccli::Var<short, 1, ccli::MaxLimit<500>> short_var("s", "short", 0);
	ccli::Var<bool> bool_var("b", "bool1", false);

	try {
		ccli::parseArgs(argc, argv);
		ccli::loadConfig("test.cfg");
		var_test.value({ 200, 200 });
		ccli::writeConfig("test.cfg");
		std::cout << float_var1.value()[0] << std::endl;
		std::cout << float_var1.value()[1] << std::endl;
		std::cout << var_test.value()[0] << std::endl;
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}

	std::cout << "Sizeof float_var1: " << sizeof(float_var1) << std::endl;
	std::cout << "Sizeof float_var2: " << sizeof(float_var2) << std::endl;

	std::cout << "repr: " << float_var1.valueString() << std::endl;

	ccli::Var<std::string> string_var("str1", "string1", "A cool value");
	ccli::Var<std::string> string_var2(std::string{ "str2" }, "string2", "Another cool value");

	std::cout << "Currently registerd variables..." << std::endl;
	ccli::forEachVar([](ccli::VarBase& var, size_t idx) -> ccli::IterationDecision {
		std::cout << "... " << idx << " - " << var.longName() << " " << var.valueString() << std::endl;
		return {};
	});

	std::cout << *var_test.asBool() << std::endl;
	std::cout << *var_test.asInt() << std::endl;
	std::cout << *var_test.asFloat() << std::endl;
	std::cout << var_test.asString().has_value() << std::endl;
	std::cout << *string_var.asString() << std::endl;

	return 0;
}
