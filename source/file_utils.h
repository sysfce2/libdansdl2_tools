#ifndef TOOLS_FILE_UTILS_H
#define TOOLS_FILE_UTILS_H

#include <string>
#include <fstream>

namespace tools
{

//!Returns true if the given file exists.
bool		file_exists(const std::string&);

//!Dumps the contents of the file to a string.
std::string	dump_file(const std::string&);

}

#endif
