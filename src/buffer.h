#pragma once
#include <vector>

template<typename T>
inline void clear(std::vector<T>& vec)
{
    std::vector<T>().swap(vec);
}

class buffer_t
{
public:
    buffer_t() = default;

    ~buffer_t() = default;

    char* data();

    std::size_t& offset();

    std::size_t size();

    void resize(std::size_t newsize);

    void clear();

private:
    std::vector<char> _data;
    std::size_t _offet = 0;
};
