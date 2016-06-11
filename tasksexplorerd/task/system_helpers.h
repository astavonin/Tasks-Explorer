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

proc_info_vec build_tasks_list();

struct proc_args
{
    std::string              path_name;
    std::string              app_name;
    std::vector<std::string> argv;
    std::unordered_map<std::string, std::string> env;
};

std::ostream& operator<<( std::ostream& os, const proc_args& p );

boost::optional<std::vector<char>> read_proc_args( pid_t pid, logger_ptr log );
proc_args parse_proc_args( const std::vector<char>& procargv, logger_ptr log );
}
