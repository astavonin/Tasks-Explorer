#include <hayai.hpp>
#include "../task/system_helpers.h"

BENCHMARK( ProcsGoup, GetKinfoProcs, 10, 10 )
{
    auto procs = tasks::GetKinfoProcs();
}

