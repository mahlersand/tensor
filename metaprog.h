#ifndef METAPROG_H
#define METAPROG_H

//This header includes modern, C++20 based metaprogramming helpers

#include <memory>
#include <tuple>
#include <functional>
#include <iostream>

//First returns the first value in a size_t based parameter pack

template<size_t ...>
struct First;

template<size_t DimsHead, std::size_t ...DimsTail>
struct First<DimsHead, DimsTail...>
{
    static constexpr std::size_t value = DimsHead;
};





//IsInSequence checks whether the first given value is in the remaining sequence of values

template<size_t, size_t ...>
struct IsInSequenceStruct;

template<size_t Num>
struct IsInSequenceStruct<Num>
{
    static constexpr bool value = false;
};

template<size_t Num, size_t ...Sequence>
struct IsInSequenceStruct
{
    static constexpr bool value = ((Num == Sequence) || ...);
};

template<size_t Num, size_t ...Sequence>
inline constexpr bool IsInSequence = IsInSequenceStruct<Num, Sequence ...>::value;





//IndexTuple is a tuple of indices of the given size

template<size_t, typename ...>
struct IndexTupleHelper;

template<typename ...Indices>
struct IndexTupleHelper<0, Indices ...>
{
    using type = std::tuple<Indices ...>;
};

template<size_t Size, typename ...Indices>
struct IndexTupleHelper
{
    using type = typename IndexTupleHelper<Size - 1, std::size_t, Indices ...>::type;
};

template<size_t Size>
using IndexTuple = typename IndexTupleHelper<Size>::type;







//MetaFilter is to be a means of filtering container types statically,
//for example to filter a tuple by indices or value types. It takes a
//list of parameters of type size_t, which need to iterate over the
//container

template<size_t ...>
struct MetaFilter;

template<>
struct MetaFilter<>
{
    template<typename ...TupleType>
    struct Tuple
    {
        template<size_t ...Indices>
        struct RemoveIndices
        {
            static auto filter(std::tuple<TupleType ...> const&)
            {
                return std::tuple<>();
            }
        };
    };
};

template<size_t MetaIteratorHead, size_t ...MetaIteratorTail>
struct MetaFilter<MetaIteratorHead, MetaIteratorTail ...>
{
    template<typename ...TupleType>
    struct Tuple
    {
        template<size_t ...Indices>
        struct RemoveIndices
        {
            static auto filter(std::tuple<TupleType ...> const& t)
            requires (IsInSequence<MetaIteratorHead, Indices ...>)
            {
                return MetaFilter<MetaIteratorTail ...>
                        ::template Tuple<TupleType ...>
                        ::template RemoveIndices<Indices ...>
                        ::filter(t);
            }

            static auto filter(std::tuple<TupleType ...> const& t)
            requires (!IsInSequence<MetaIteratorHead, Indices ...>)
            {
                return std::tuple_cat(
                            std::make_tuple(std::get<MetaIteratorHead>(t)),
                            MetaFilter<MetaIteratorTail ...>
                                ::template Tuple<TupleType ...>
                                ::template RemoveIndices<Indices ...>
                                ::filter(t));
            }
        };
    };
};

template<std::size_t ...Indices,
         typename ...TupleType,
         std::size_t ...TupleIterator>
auto tupleRemoveByIndexHelper(std::tuple<TupleType ...> const& t,
                              std::index_sequence<TupleIterator ...>)
{
    return MetaFilter<TupleIterator ...>::template Tuple<TupleType ...>::template RemoveIndices<Indices...>::filter(t);
}

template<std::size_t ...Indices,
         typename ...TupleType>
auto tupleRemoveByIndex(std::tuple<TupleType ...> const& t)
{
    return tupleRemoveByIndexHelper<Indices ...>(t, std::make_index_sequence<sizeof ...(TupleType)>{});
}




#endif // METAPROG_H
