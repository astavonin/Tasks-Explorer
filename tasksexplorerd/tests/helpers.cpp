#include "helpers.h"
#include <utils.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include "../task/system_helpers.h"

namespace fs = boost::filesystem;

namespace tests
{
namespace helpers
{
std::vector<char> read_file( const std::string &name )
{
    fs::path exec( utils::GetExecDir() );
    fs::path f( name );

    std::fstream is( fs::canonical( exec.parent_path() / f ).string(),
                     std::ifstream::binary | std::ifstream::in );
    if( !is ) throw std::runtime_error( "Unable to read file" );

    is.seekg( 0, is.end );
    auto length = is.tellg();
    is.seekg( 0, is.beg );

    std::vector<char> buff( length );
    is.read( &buff[0], length );

    return buff;
}
}
}
