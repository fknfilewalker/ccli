#include <ccli/ccli.h>
#include <iostream>


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
	std::cout << "<Test 1 type=\"bool test\">" << std::endl;
	// args
	ccli::Var<bool> bool_var1("b1", "bool1", 0, ccli::NONE, "First bool Var");
	ccli::Var<bool> bool_var2("b2", "bool2", false, ccli::NONE, "Second bool Var");
	ccli::Var<bool> bool_var3("b3", "bool3", 1, ccli::NONE, "Third bool Var");
	ccli::Var<bool> bool_var4("b4", "bool4", true, ccli::NONE, "Fourth bool Var");

	std::cout << "\t<args> no args (defaults) </args>" << std::endl;
	std::cout << "\tbool_var1 is " << bool_var1.getValue() << " should be " << false << std::endl;
	std::cout << "\tbool_var2 is " << bool_var2.getValue() << " should be " << false << std::endl;
	std::cout << "\tbool_var3 is " << bool_var3.getValue() << " should be " << true << std::endl;
	std::cout << "\tbool_var4 is " << bool_var4.getValue() << " should be " << true << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;
	std::cout << std::endl;

	const char* argv[] = { "-b1", "1", "-b2", "true", "-b3", "0", "-b4", "false" };
	ccli::parseArgs(8, argv);
	std::cout << "\t<args> -b1 1 -b2 true -b3 0 -b4 false </args>" << std::endl;
	std::cout << "\tbool_var1 is " << bool_var1.getValue() << " should be " << true << std::endl;
	std::cout << "\tbool_var2 is " << bool_var2.getValue() << " should be " << true << std::endl;
	std::cout << "\tbool_var3 is " << bool_var3.getValue() << " should be " << false << std::endl;
	std::cout << "\tbool_var4 is " << bool_var4.getValue() << " should be " << false << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;
	std::cout << std::endl;

	const char* argv2[] = { "-b1", "0", "-b2", "-b3", "--bool4" };
	ccli::parseArgs(5, argv2);
	std::cout << "\t<args> -b1 0 -b2 -b3 --bool4 </args>" << std::endl;
	std::cout << "\tbool_var1 is " << bool_var1.getValue() << " should be " << false << std::endl;
	std::cout << "\tbool_var2 is " << bool_var2.getValue() << " should be " << true << std::endl;
	std::cout << "\tbool_var3 is " << bool_var3.getValue() << " should be " << true << std::endl;
	std::cout << "\tbool_var4 is " << bool_var4.getValue() << " should be " << true << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;

	std::cout << "</Test 1>" << std::endl;
}

void test2()
{
	std::cout << "<Test 2 type=\"\">" << std::endl;

	/*const char* argv[] = { "--bool1", "-b2", "-f2", "200,200" };
	ccli::parseArgs(4, argv);
	std::cout << "\t--- --bool1 -b2 -f2 200,200 ---" << std::endl;
	std::cout << "\tbool_var1 is " << bool_var1.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tbool_var2 is " << bool_var2.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tfvec3_var1 is " << fvec2_var1.getValue()[0] << " " << fvec2_var1.getValue()[1] << " should be " << 200 << " " << 200 << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;
	std::cout << std::endl;

	const char* argv2[] = { "--bool1", "-b2", "f" };
	ccli::parseArgs(4, argv);
	std::cout << "\t--- --bool1 -b2 -f2 200,200 ---" << std::endl;
	std::cout << "\tbool_var1 is " << bool_var1.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tbool_var2 is " << bool_var2.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tfvec3_var1 is " << fvec2_var1.getValue()[0] << " " << fvec2_var1.getValue()[1] << " should be " << 200 << " " << 200 << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;
	std::cout << std::endl;*/

	std::cout << "</Test 2>" << std::endl;
}

int main(int argc, char* argv[]) {
	test1();


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
