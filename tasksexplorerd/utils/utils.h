#pragma once

#include <string>

namespace utils
{
std::string GetExecDir();
void HexDump( const char *desc, const void *addr, int len );
}
