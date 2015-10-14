#ifndef HX_UNORDERED_INCLUDED
#define HX_UNORDERED_INCLUDED

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) || ( defined(__GNUC__) && defined(HXCPP_CPP11) )
#include <unordered_set>
#include <unordered_map>

namespace hx
{

template<typename T>
struct UnorderedSet : public std::unordered_set<T> { };

template<typename KEY, typename VALUE>
struct UnorderedMap : public std::unordered_map<KEY,VALUE> { };

}


#else

#include <set>
#include <map>

namespace hx
{

template<typename T>
struct UnorderedSet : public std::set<T> { };

template<typename KEY, typename VALUE>
struct UnorderedMap : public std::map<KEY,VALUE> { };

}

#endif

#endif
