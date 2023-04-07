#include <ccli/ccli.h>
#include <iostream>
#include <cassert>

void test1()
{
	ccli::Var<bool> boolVar1("b1", "bool1", 0, ccli::NONE, "First bool Var");
	ccli::Var<bool> boolVar2("b2", "bool2", false, ccli::NONE, "Second bool Var");
	ccli::Var<bool> boolVar3("b3", "bool3", 1, ccli::NONE, "Third bool Var");
	ccli::Var<bool> boolVar4("b4", "bool4", true, ccli::NONE, "Fourth bool Var");

	try {
		assert(boolVar1.value() == false);
		assert(boolVar2.value() == false);
		assert(boolVar3.value() == true);
		assert(boolVar4.value() == true);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}

	try{
		const char* argv[] = { "-b1", "1", "-b2", "true", "-b3", "0", "-b4", "false" };
		ccli::parseArgs(std::size(argv), argv);
		assert(boolVar1.value() == true);
		assert(boolVar2.value() == true);
		assert(boolVar3.value() == false);
		assert(boolVar4.value() == false);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}

	try {
		const char* argv2[] = { "-b1", "0", "-b2", "-b3", "--bool4" };
		ccli::parseArgs(std::size(argv2), argv2);
		assert(boolVar1.value() == false);
		assert(boolVar2.value() == true);
		assert(boolVar3.value() == true);
		assert(boolVar4.value() == true);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
}

void test2()
{
	ccli::Var<uint32_t, 3> uvec3Var("uvec3", "", { 1, 2, 3 });
	ccli::Var<std::string, 2> stringVar("string", "", { "This is a test", "really" });
	ccli::Var<uint8_t, 2, ccli::MaxLimit<2>> limitVar("limit", "", { 3, 4 });

	try {
		assert(uvec3Var.value().at(0) == 1 && uvec3Var.value().at(1) == 2 && uvec3Var.value().at(2) == 3);
		assert(stringVar.value().at(0) == "This is a test" && stringVar.value().at(1) == "really");
		assert(limitVar.value().at(0) == 2 && limitVar.value().at(1) == 2);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}

	try {
		const char* argv[] = { "-uvec3", "5,6,7", "-string", "This is not a test,or is it", "-limit", "100,200" };
		ccli::parseArgs(std::size(argv), argv);
		assert(uvec3Var.value().at(0) == 5 && uvec3Var.value().at(1) == 6 && uvec3Var.value().at(2) == 7);
		assert(stringVar.value().at(0) == "This is not a test" && stringVar.value().at(1) == "or is it");
		assert(limitVar.value().at(0) == 2 && limitVar.value().at(1) == 2);
	}
	catch (ccli::CCLIError& e) {
		std::cout << "Caught error: " << e.message() << std::endl;
	}
}

void test3() {
	float value = 0.0f;
	ccli::Var<float> lambdaVar("lambda", "", { 100.0f }, 0, "",
		[&](const ccli::Storage<float>& v) {
			value = v.at(0);
		}
	);
	assert(value == 0.0f);

	const char* argv[] = { "-lambda", "222"};
	ccli::parseArgs(std::size(argv), argv);
	assert(std::abs(222.0f - value) < std::numeric_limits<float>::epsilon());
}

void test4() {
	try {
		throw ccli::FileError{ "a/file/name" };
	}
	catch (const std::exception& e) {
		std::cout << "Caught error: " << e.what() << std::endl;
	}
}

int main(int argc, char* argv[]) {
	test1();
	test2();
	test3();
	test4();

	ccli::Var<float, 4, ccli::MaxLimit<1.0>, ccli::MinLimit<-1>> float_var1("f1", "float1", {0}, ccli::NONE, "First bool Var");
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
