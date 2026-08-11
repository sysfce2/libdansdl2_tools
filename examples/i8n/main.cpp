#include <tools/i8n.h>
#include <tools/string_utils.h>
#include <tools/file_utils.h>

#include <iostream>

int main(int, char **) {

	using namespace tools;

	//i8n localization={"../examples/i8n/data", "en", {"test01.dat", "test02.dat"}};
	i8n localization={"../examples/i8n/data", "en"};
	localization.add_files({"test01.dat", "test02.dat"}, false); //false=Do not build the database yet, we are not done adding files!
	localization.add_user_file("user.dat", "../examples/i8n/somewhere-else", false); //false=Do not build the database yet, we are not done adding files!
	localization.build();

	localization.add_file({"test03.dat"});

	localization.set({"var", "supervar"});
	std::cout<<localization.get("label-1")<<std::endl;
	std::cout<<localization.get("label-2")<<std::endl;
	std::cout<<localization.get("label-3")<<std::endl;
	std::cout<<localization.get("label-4")<<std::endl;
	std::cout<<localization.get("label-5")<<std::endl;
	std::cout<<localization.get("complex", {{"varhere","varhere1"}, {"varthere","varthere1"}})<<std::endl;
	std::cout<<localization.get("user_text")<<std::endl;
	std::cout<<localization.get("label-doesnotexist")<<std::endl;

	localization.set_fail_entry("{{Will not be able to find ((__key__))}}");
	std::cout<<localization.get("label-doesnotexist")<<std::endl;

	return 0;
}
