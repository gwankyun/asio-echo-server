#include "buffer.h"

char* buffer_t::data()
{
    return _data.data();
}

std::size_t& buffer_t::offset()
{
    return _offet;
}

std::size_t buffer_t::size()
{
    return _data.size();
}

void buffer_t::resize(std::size_t newsize)
{
    _data.resize(newsize);
}

void buffer_t::clear()
{
    ::clear(_data);
    _offet = 0;
}
