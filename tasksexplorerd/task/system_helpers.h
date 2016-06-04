#pragma once

#include <sys/sysctl.h>
#include <vector>

namespace tasks
{
using proc_info_vec = std::vector<kinfo_proc>;

proc_info_vec GetKinfoProcs();
}
