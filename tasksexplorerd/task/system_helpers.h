#pragma once

#include <sys/sysctl.h>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace tasks
{
using proc_info_vec = std::vector<kinfo_proc>;

proc_info_vec GetKinfoProcs();

struct ProcArgs
{
    std::string              fullPathName;
    std::string              appName;
    std::vector<std::string> argv;
    std::unordered_map<std::string, std::string> env;
};

std::ostream& operator<<( std::ostream& os, const ProcArgs& p );

std::unique_ptr<ProcArgs> ParseProcArgs( const std::vector<char>& procargv );
}
