#include <iostream>
#include <memory>

#include "tensor.h"
#include "metaprog.h"

using namespace std;

int main()
{
    Tensor<double, 3> v1 {{1., 2., 3.}};
    Tensor<double, 3> v2 {{1., 2., 3.}};

    auto r1 = tensorMul(v1, v2);
    auto r2 = tensorMul(v1, r1);
    [[maybe_unused]]
    auto r3 = tensorMul(r2, r2);
    auto r4 = r1.tensorContract<0, 1>();
    auto r5 = r2.tensorContract<0, 2>();

    std::cout << "r1 =" << std::endl << r1 << std::endl << std::endl
              << "r2 =" << std::endl << r2 << std::endl << std::endl
              << "r4 =" << std::endl << r4 << std::endl << std::endl
              << "r5 =" << std::endl << r5 << std::endl << std::endl;

}
