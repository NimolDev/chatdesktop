#ifndef PAGE_RESPONSE_HPP
#define PAGE_RESPONSE_HPP

#include <QString>

  template<typename T>
struct PageResponse
{

    T messages;
    int page;
    int limit;
    int total;
    int total_pages;
};

#endif // PAGE_RESPONSE_HPP
