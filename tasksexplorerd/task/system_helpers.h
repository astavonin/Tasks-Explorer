#pragma once

#include <sys/sysctl.h>
#include <boost/optional.hpp>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "logger.h"

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

boost::optional<std::vector<char>> ReadProcArgs( pid_t pid, logger_ptr log );
ProcArgs ParseProcArgs( const std::vector<char>& procargv, logger_ptr log );
}
