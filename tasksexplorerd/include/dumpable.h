#pragma once
#include <type_traits>

namespace common
{
class dumpable
{
public:
    virtual ~dumpable()
    {
    }
    virtual void dump( std::ostream& os ) const = 0;
};

template <typename T,
          typename = std::enable_if_t<std::is_base_of<dumpable, T>::value>>
std::ostream& operator<<( std::ostream& os, const T& d )
{
    d.dump( os );
    return os;
}
}
