#include "ccli.hpp"
#include <iostream>


//ccli::var<float, 2, ccli::CONFIG_RDWR> var_test("test", "t", "Just a test", { 100,100 });
//ccli::var<float, 2, ccli::CONFIG_RDWR> var_test2("test", "", "Hallo", { 100,100 });
//
//void print(const std::array<float, 2> aValue)
//{
//	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
//}
//ccli::var<float, 2> var_cbtest("cbtest", "cbt", "Just a test", { 100,100 }, print);
//
//ccli::var<float, 2> var_cbtestlambda("cbtestlambda", "cbtl", "Just a test", { 100,100 }, [](const std::array<float, 2> aValue) {
//	std::cout << "Callback test: " << aValue[0] << " " << aValue[1] << std::endl;
//	});
//
//ccli::var<bool> var_nix("", "nix", "Just a test", { });
//ccli::var<bool> var_nix2("", "nix2", "Just a test", { });

void test1()
{
	std::cout << "<Test 1 type=\"bool test\">" << std::endl;
	// args
	ccli::var<bool> bool_var1("bool1", "b1", "First bool var");
	ccli::var<bool> bool_var2("bool2", "b2", "Second bool var");
	ccli::var<bool> bool_var3("bool3", "b3", "Third bool var", { true });
	ccli::var<bool> bool_var4("bool4", "b4", "Fourth bool var", { 1 });

	std::cout << "\t<args> no args (defaults) </args>" << std::endl;
	std::cout << "\tbool_var1 is " << bool_var1.getValue()[0] << " should be " << false << std::endl;
	std::cout << "\tbool_var2 is " << bool_var2.getValue()[0] << " should be " << false << std::endl;
	std::cout << "\tbool_var3 is " << bool_var3.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tbool_var4 is " << bool_var4.getValue()[0] << " should be " << true << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;
	std::cout << std::endl;

	const char* argv[] = { "-b1", "1", "-b2", "true", "-b3", "0", "-b4", "false" };
	ccli::parseArgs(8, argv);
	std::cout << "\t<args> -b1 1 -b2 true -b3 0 -b4 false </args>" << std::endl;
	std::cout << "\tbool_var1 is " << bool_var1.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tbool_var2 is " << bool_var2.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tbool_var3 is " << bool_var3.getValue()[0] << " should be " << false << std::endl;
	std::cout << "\tbool_var4 is " << bool_var4.getValue()[0] << " should be " << false << std::endl;
	for (const std::string& s : ccli::checkErrors()) std::cout << "\t\t" << s << std::endl;
	std::cout << std::endl;

	const char* argv2[] = { "-b1", "0", "-b2", "-b3", "--bool4" };
	ccli::parseArgs(5, argv2);
	std::cout << "\t<args> -b1 0 -b2 -b3 --bool4 </args>" << std::endl;
	std::cout << "\tbool_var1 is " << bool_var1.getValue()[0] << " should be " << false << std::endl;
	std::cout << "\tbool_var2 is " << bool_var2.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tbool_var3 is " << bool_var3.getValue()[0] << " should be " << true << std::endl;
	std::cout << "\tbool_var4 is " << bool_var4.getValue()[0] << " should be " << true << std::endl;
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

	ccli::var<float> float_var1("float1", "f1", "First bool var", { {-1},{1} });
	ccli::parseArgs(argc, argv);
	std::cout << float_var1.getValue()[0] << std::endl;
	/*ccli::parseArgs(argc, argv);
	ccli::loadConfig("test.cfg");
	ccli::writeConfig("test.cfg");
	std::cout << var_nix.getValue()[0] << " " << var_nix2.getValue()[0] << 
	for (const std::string& s : ccli::checkErrors()) std::cout << s << std::endl;
	for (const std::string& s : ccli::getHelp()) std::cout << s << std::endl;*/
	return 0;
}
