#pragma once

#include <mach/mach_error.h>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/info_tuple.hpp>
#include <exception>
#include <string>

namespace err
{
using api_function = boost::errinfo_api_function;

using mach_kern_ret   = boost::error_info<struct mach_error_code_, int>;
using mach_err_desc   = boost::error_info<struct mach_err_desc_, char*>;
using mach_error_info = boost::tuple<mach_kern_ret, mach_err_desc>;

inline mach_error_info mach_error( int kr )
{
    return mach_error_info( kr, mach_error_string( kr ) );
}

// Is used for informing about system API failure only
struct sys_api_error : virtual std::exception, virtual boost::exception
{
};
};
