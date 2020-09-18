#ifndef TENSOR_H
#define TENSOR_H

#include <array>
#include <functional>
#include <tuple>
#include <memory>
#include <iostream>

#include "metaprog.h"

//Using declarations
//SFINAE
using std::conditional_t;

//Metaprogramming
using std::index_sequence;
using std::get;
using std::make_index_sequence;
using std::tuple;
using std::make_tuple;
using std::tuple_cat;
using std::pair;
using std::make_pair;

//Functional
using std::function;
using std::apply;

//Memory
using std::swap;

//Other Types
using std::size_t;
using std::array;
using std::ostream;








//Forward declarations of root types

template<typename T, size_t ...Dims>
class Tensor;






//Tensor-specific metaprogramming helpers

template<typename T, int M, int N, size_t ...Dimensions>
struct ContractionResultHelper;

template<typename T, int M, int N>
struct ContractionResultHelper<T, M, N>
{
    using type = Tensor<T>;
};

template<typename T, int M, int N, size_t DimensionsHead, size_t ...DimensionsTail>
struct ContractionResultHelper<T, M, N, DimensionsHead, DimensionsTail ...>
{
    using type = conditional_t<(M == 0), typename ContractionResultHelper<T, M-1, N-1, DimensionsTail ...>::type,
                 conditional_t<(N == 0), typename ContractionResultHelper<T, M-1, N-1, DimensionsTail ...>::type,
                                         typename ContractionResultHelper<T, M-1, N-1, DimensionsTail ...>::type::template IncreaseOrder_t<DimensionsHead>>>;
};






//
//Tensor
//

//0 D - Tensor
template<typename T>
class Tensor<T>
{
    T value;

public:
    Tensor<T>() : value(0.0l) {  }
    Tensor<T>(T const& other) : value(other) {  }
    Tensor<T>(T&& other) { swap(value, other); }
    Tensor<T>(Tensor<T> const& other) : value(other.value) {  }
    Tensor<T>(Tensor<T>&& other) { swap(value, other.value); }

    template<size_t N>
    using IncreaseOrder_t = Tensor<T, N>;

    Tensor<T> operator=(T const x) const
    {
        value = x;
        return *this;
    }

    Tensor<T> operator=(Tensor<T> const x) const
    {
        value = x.value;
        return *this;
    }

    T& tensorAt()
    {
        return value;
    }

    T& tensorAt(tuple<>)
    {
        return value;
    }

    operator T() const
    {
        return value;
    }
};

//n D - Tensor
template<typename T, size_t DimsHead, std::size_t ...DimsTail>
class Tensor<T, DimsHead, DimsTail...> : public std::array<Tensor<T, DimsTail...>, DimsHead>
{
public:
    static constexpr IndexTuple<sizeof ...(DimsTail) + 1> dimensions = make_tuple(DimsHead, DimsTail...);
    static constexpr size_t dimensionCount = sizeof ...(DimsTail) + 1;

    template<size_t N>
    using IncreaseOrder_t = Tensor<T, N, DimsHead, DimsTail ...>;

    enum IterationMode
    {
        Iterators,
        Values
    };

private:
    template<int IterationMode>
    class TensorIterator;
    template<int IterationMode>
    class TensorRange;

    template<int IterationMode>
    class TensorIterator {
    public:
        IndexTuple<dimensionCount> multiIndex;

        TensorIterator& operator=(TensorIterator const& other)
        {
            multiIndex = other.multiIndex;
            return *this;
        }

        decltype (auto) operator<=>(TensorIterator const& other) const
        {
            return multiIndex <=> other.multiIndex;
        }

        bool operator==(TensorIterator const& other) const
        {
            return multiIndex == other.multiIndex;
        }

        bool operator!=(TensorIterator const& other) const
        {
            return multiIndex != other.multiIndex;
        }

        TensorIterator<IterationMode> const& operator*() const
        requires (IterationMode == Iterators)
        {
            return *this;
        }

    private:
        template<size_t ...Indices>
        void crementHelper(index_sequence<Indices ...>)
        {
            //If we have more than one dimension, we need to carry over
            if(dimensionCount > 1) {
                const size_t max = dimensionCount - 2;
                //idx = max - Indices

                //This madness is necessary to have the decrement behave correctly
                //The first operand of / has to be positive to be meaningful
                //Might fix later with some actual modular arithmetics
                ((get<max - Indices>(multiIndex) +=
                        (get<max - Indices + 1>(multiIndex) + get<max - Indices + 1>(dimensions))
                        / get<max - Indices + 1>(dimensions) - 1)
                , ...);

                //This madness is necessary to have the decrement behave correctly
                //The first operand of % has to be positive to be meaningful
                //Might fix later with some actual modular arithmetics
                ((get<max - Indices + 1>(multiIndex) =
                        (get<max - Indices + 1>(multiIndex) + get<max - Indices + 1>(dimensions))
                        % get<max - Indices + 1>(dimensions))
                , ...);
            }
        }

    public:
        TensorIterator operator++()
        {
            //Increment the last dimension
            ++get<dimensionCount - 1>(multiIndex);

            //We need to carry over dimensionCount - 1 times
            crementHelper(make_index_sequence<dimensionCount - 1>());

            return *this;
        }

        TensorIterator operator--()
        {
            //Decrement the last dimension
            --get<dimensionCount - 1>(multiIndex);

            //We need to carry over dimensionCount - 1 times
            crementHelper(make_index_sequence<dimensionCount - 1>());

            return *this;
        }
    };

public:
    template<int IterationMode>
    using iterator_t = TensorIterator<IterationMode>;

private:
    template<int IterationMode>
    class TensorRange {
    public:
        iterator_t<IterationMode> begin()
        {
            return (iterator_t<IterationMode>{ IndexTuple<dimensionCount>{} });
        }

        iterator_t<IterationMode> end()
        {
            return ++iterator_t<IterationMode>{ make_tuple(DimsHead - 1, (DimsTail - 1)...) };
        }
    };

public:
    auto& tensorAt()
    {
        return *this;
    }

    template<typename ...Idx_t>
    auto& tensorAt(size_t firstIndex, Idx_t ...Multiindex)
    {
        return (*this).operator[](firstIndex).tensorAt(Multiindex ...);
    }

    template<typename ...Indices, size_t ...IndexIterator>
    auto& tensorAtHelper(tuple<Indices ...> const& indices, index_sequence<IndexIterator ...>)
    {
        return tensorAt(get<IndexIterator>(indices)...);
    }

    template<typename ...Indices>
    auto& tensorAt(tuple<Indices ...> const& indices)
    {
        return tensorAtHelper(indices, make_index_sequence<sizeof ...(Indices)>{});
    }

    template<size_t M, size_t N>
    auto tensorContract()
    requires ((0 < sizeof ...(DimsTail))
              && (M != N)
              && (get<M>(dimensions) == get<N>(dimensions)))
    {
        using ResultType = typename ContractionResultHelper<T, M, N, DimsHead, DimsTail ...>::type;

        ResultType result;

        for(auto i : this->iterators()) {
            if(get<M>(i.multiIndex) == get<N>(i.multiIndex))
                result.tensorAt(tupleRemoveByIndex<M, N>(i.multiIndex))
                    += this->tensorAt(i.multiIndex);
        }

        return result;
    }

    TensorRange<Iterators> iterators()
    {
        return TensorRange<Iterators>();
    }
};

template<typename T, size_t ...Dimensions>
ostream& operator<<(ostream& stream, Tensor<T, Dimensions ...> const& t)
requires (sizeof ...(Dimensions) > 0)
{
    stream << "[";

    auto it = t.begin();
    auto __end = t.end();

    for(; it != __end; ++it) {
        stream << *it;
        auto it2 = it;
        ++it2;
        if(it2 != __end) {
            stream << ",";
            if(sizeof ...(Dimensions) > 1)
                stream << std::endl;
        }
    }

    stream << "]";
    return stream;
}




//Tensor product of two tensors
template<typename T1, typename T2, size_t ...D1, size_t ...D2>
decltype (auto) tensorMul(Tensor<T1, D1 ...> t1, Tensor<T2, D2 ...> t2)
{
    using TR = decltype (T1() * T2());

    Tensor<TR, D1 ..., D2 ...> result;

    for(auto it1 : t1.iterators()) {
        for(auto it2 : t2.iterators()) {
            result.tensorAt(it1.multiIndex).tensorAt(it2.multiIndex) =
                    t1.tensorAt(it1.multiIndex) * t2.tensorAt(it2.multiIndex);
        }
    }

    return result;
}


#endif // TENSOR_H
