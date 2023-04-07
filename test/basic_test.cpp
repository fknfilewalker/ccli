#include <ccli/ccli.h>
#include <iostream>
#include <cassert>

//ccli::Var<float, 2, ccli::CONFIG_RDWR> var_test("test", "t", "Just a test", { 100,100 });
//ccli::Var<float, 2, ccli::CONFIG_RDWR> var_test2("test", "", "Hallo", { 100,100 });
//
//void print(const std::array<float, 2> aValue)
//{
//	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
//}
//ccli::Var<float, 2> var_cbtest("cbtest", "cbt", "Just a test", { 100,100 }, print);
//
//ccli::Var<float, 2> var_cbtestlambda("cbtestlambda", "cbtl", "Just a test", { 100,100 }, [](const std::array<float, 2> aValue) {
//	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
//	});
//
//ccli::Var<bool> var_nix("", "nix", "Just a test", { });
//ccli::Var<bool> var_nix2("", "nix2", "Just a test", { });

void test1()
{
	ccli::Var<bool> bool_var1("b1", "bool1", 0, ccli::NONE, "First bool Var");
	ccli::Var<bool> bool_var2("b2", "bool2", false, ccli::NONE, "Second bool Var");
	ccli::Var<bool> bool_var3("b3", "bool3", 1, ccli::NONE, "Third bool Var");
	ccli::Var<bool> bool_var4("b4", "bool4", true, ccli::NONE, "Fourth bool Var");

	assert(bool_var1.getValue() == false);
	assert(bool_var2.getValue() == false);
	assert(bool_var3.getValue() == true);
	assert(bool_var4.getValue() == true);
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl << std::endl;

	const char* argv[] = { "-b1", "1", "-b2", "true", "-b3", "0", "-b4", "false" };
	ccli::parseArgs(std::size(argv), argv);
	assert(bool_var1.getValue() == true);
	assert(bool_var2.getValue() == true);
	assert(bool_var3.getValue() == false);
	assert(bool_var4.getValue() == false);
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl << std::endl;

	const char* argv2[] = { "-b1", "0", "-b2", "-b3", "--bool4" };
	ccli::parseArgs(std::size(argv2), argv2);
	assert(bool_var1.getValue() == false);
	assert(bool_var2.getValue() == true);
	assert(bool_var3.getValue() == true);
	assert(bool_var4.getValue() == true);
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;
}

void test2()
{
	ccli::Var<uint32_t, 3> uvec3_var("uvec3", "", {1, 2, 3});
	ccli::Var<std::string, 2> string_var("string", "", { "This is a test", "really"});

	assert(uvec3_var.getValue().at(0) == 1 && uvec3_var.getValue().at(1) == 2 && uvec3_var.getValue().at(2) == 3);
	assert(string_var.getValue().at(0) == "This is a test" && string_var.getValue().at(1) == "really");
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;

	const char* argv[] = { "-uvec3", "5,6,7", "-string", "This is not a test,or is it"};
	ccli::parseArgs(std::size(argv), argv);
	assert(uvec3_var.getValue().at(0) == 5 && uvec3_var.getValue().at(1) == 6 && uvec3_var.getValue().at(2) == 7);
	assert(string_var.getValue().at(0) == "This is not a test" && string_var.getValue().at(1) == "or is it");
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;

}

int main(int argc, char* argv[]) {
	test1();
	test2();

	ccli::Var<float, 4, ccli::MaxLimit<1.0>, ccli::MinLimit<-1>> float_var1("f1", "float1", {0}, ccli::NONE, "First bool Var");
	ccli::Var<float, 4> float_var2("f2", "float2", {0}, ccli::NONE, "First bool Var");
	ccli::Var<float, 2> var_test("t", "test", { 100, 200 }, ccli::CONFIG_RDWR);
	ccli::Var<short, 1, ccli::MaxLimit<500>> short_var("s", "short", 0);
	ccli::Var<bool> bool_var("b", "bool1", false);
	ccli::parseArgs(argc, argv);
	ccli::loadConfig("test.cfg");
	var_test.setValue({ 200, 200 });
	ccli::writeConfig("test.cfg");
	std::cout << float_var1.getValue()[0] << std::endl;
	std::cout << float_var1.getValue()[1] << std::endl;
	std::cout << var_test.getValue()[0] << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << s << std::endl;

	std::cout << "Sizeof float_var1: " << sizeof(float_var1) << std::endl;
	std::cout << "Sizeof float_var2: " << sizeof(float_var2) << std::endl;

	std::cout << "repr: " << float_var1.getValueString() << std::endl;

	ccli::Var<std::string> string_var("str1", "string1", "A cool value");
	ccli::Var<std::string> string_var2(std::string{ "str2" }, "string2", "Another cool value");

	std::cout << "Currently registerd variables..." << std::endl;
	ccli::forEachVar([](ccli::VarBase& var, size_t idx) -> ccli::IterationDecision {
		std::cout << "... " << idx << " - " << var.getLongName() << " " << var.getValueString() << std::endl;
		return {};
	});

	std::cout << *var_test.asBool() << std::endl;
	std::cout << *var_test.asInt() << std::endl;
	std::cout << *var_test.asFloat() << std::endl;
	std::cout << var_test.asString().has_value() << std::endl;
	std::cout << *string_var.asString() << std::endl;

	return 0;
}
