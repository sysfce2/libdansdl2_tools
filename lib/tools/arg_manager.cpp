#include <tools/arg_manager.h>
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace tools;

arg_manager::arg_manager(int argc, char ** argv) {
	init(argc, argv);
}

void arg_manager::init(int argc, char ** argv) {

	int i=0;
	while(i< argc) {
		data.push_back(t_arg(argv[i]));
		++i;
	}
}

const arg_manager::t_arg arg_manager::get_argument(unsigned int p_arg) const {

	try {
		return data[p_arg];
	}
	catch (...) {
		throw arg_manager_exception("Invalid argument index");
	}
}

int arg_manager::find_index(const t_arg& val) const {
	int i=0;

	for(const auto& arg: data) {
		if(val==arg) return i;
		else ++i;
	}

	return -1;
}

int arg_manager::find_index_value(const t_arg& val) const {

	int i=0;
	for(const auto& arg: data) {
		if(arg.substr(0, val.size())==val) return i;
		else ++i;
	}

	return -1;
}

std::string arg_manager::get_value(
	const t_arg& argumento, 
	const char delimiter
) const {

	std::stringstream ss;
	ss<<argumento<<delimiter;
	const std::string f_index=ss.str();

	auto it=std::find_if(std::begin(data), std::end(data), [&f_index](const std::string& arg)
		{return arg.find(f_index)!=std::string::npos;});

	if(it==data.end()) {
		throw arg_manager_exception("Unable to locate argument "+argumento);
	}
	else {
		auto ex=explode(*it, delimiter);
		if(ex.size()!=2) {
			throw arg_manager_exception("Invalid delimiter for argument "+argumento);
		}
		else {
			return ex[1];
		}
	}
}

const arg_manager::t_arg arg_manager::get_following(const t_arg& _v) const {

	auto index=find_index(_v);
	if(-1==index) {
		throw arg_manager_exception("Argument not found: "+_v);
	}

	try {
		return data.at(index+1);
	}
	catch(...) {
		throw arg_manager_exception("No argument follows "+_v);
	}
}

bool arg_manager::arg_follows(const t_arg& _v) const {

	auto index=find_index(_v);
	if(-1==index) {
		return false;
	}

	return (unsigned)index+1 < data.size();
}
