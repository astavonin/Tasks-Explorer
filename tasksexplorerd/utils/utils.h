#pragma once

#include <string>

namespace utils
{
std::string exec_dir();
void hex_dump( const char *desc, const void *addr, int len );
}
