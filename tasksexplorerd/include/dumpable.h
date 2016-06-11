#pragma once

namespace common
{
class dumpable
{
public:
    virtual ~dumpable()
    {
    }
    virtual void dump( std::ostream& os ) const = 0;

    friend std::ostream& operator<<( std::ostream& os, const dumpable& d );
};

std::ostream&
operator<<( std::ostream& os, const dumpable& d );
}
