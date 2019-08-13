![Boost.Real](doc/other/logo/logo.png)

Boost.Real is a data type for representing real numbers using interval arithmetic.

[![Build Status](https://travis-ci.org/BoostGSoC19/Real.svg?branch=master)](https://travis-ci.org/BoostGSoC19/Real)
[![codecov](https://codecov.io/gh/BoostGSoC19/Real/branch/master/graph/badge.svg)](https://codecov.io/gh/BoostGSoC19/Real)


## Documentation
   * [Project documentation main page](https://boostgsoc19.github.io/Real/)
   * [Doxygen documentation](https://boostgsoc19.github.io/Real/doc/html/index.html)
   * [Medium post 2018](https://medium.com/@laobelloli/boost-real-9e2dfbfbed5b)

## Introduction

### Problems addressed by boost::real

#### Compounding numerical errors with operations on floating point types
When performing several operations on a standard floating point type (like ```double``` or ```float```), numerical errors compound with the number of operations. These standard types do not track the errors generated (by the limited precision and truncation), and thus, lead to an untrustworthy result.

#### Choosing precision for desired accuracy
Another major problem occurs when representing irrational numbers like π or e<sup>π</sup>, or numbers with many/infinite digits. Generally, a truncation is used. If a certain result accuracy is desired, one must choose the correct precision for the truncation. This may be difficult to calculate, as it depends on the type and quantity of operations performed on the truncation.

### The boost::real solution
Boost::real is a data type that represents a real number. It addresses the aforementioned issues using ***range arithmetic*** [1] and ***dynamic precision*** (determined at run time). The main goal of this data type is to represent a real number x as an iterable interval, such that a more precise interval is obtained on each iteration.

A boost::real number has a ```const_precision_iterator``` that iterates the interval representing the number, by increasing precision. When comparing boost::real::reals, the interval is iterated until the result can be returned. For example, doing 2.111 > 2.2999 may be equivalent to [2.1, 2.2] > [2.2,2.3] -- the less significant digits are unnecessary.

For representation of irrational numbers, boost::real provides a constructor that takes a function pointer f of an integer n, so that f(n) returns the n'th digit of the represented number. For example, the number 1/3 can be represented by a function that for any input n > 0, returns 3.

## The boost::real numbers representation
In boost::real, a number has one of the next three representations:

1. **Explicit**: Vector of digits with the most significant digits first. An exponent is used to determine the decimal place, so that the number is equal to {digits}\*10^(-1 + exp). Essentially, the decimal starts to the left of the given digits, and the exponent shifts it. A boolean is used to set the number as positive (True) or negative (False)
2. **Algorithm**: This representation is equivalent to the Explicit number but instead of using a vector of digits, a function pointer must be provided. The function takes an unsigned integer "n" as parameter and returns the n-th digit of the number.
3. **Operation**: Composition of two numbers related by an operator (+, -, \*, /). The real_operation has pointers to the operands.

Because of the third representation type, a number resulting from a composition of operations is a binary tree where each internal node is an operation and its children are its operands. The tree leaves are of types (1) or (2) while the parent nodes are of type (3). More information about this representation can be found in [3]

## The boost::real precision iterator
The boost::real::const_precision_iterator <!-- is a forward iterator [4] that --> iterates the interval precision. The iterator returns an interval that represents the lower and upper bounds of the result's actual value for a given precision. Each time the iterator is incremented, the precision increases and an interval with a better precision is obtained. Normally, there is no need to interact with the precision iterator. It is used by the boost::real comparison operators.

## Interface
See doxygen-generated documentation.

## Examples

```cpp
#include <real/real.hpp>
#include <iostream>

int main() {
    boost::real::real a = "0.999999"_r;
    auto b = a;
    auto c = a + b;

    std::cout << "a: " << a << std::endl;
    std::cout << "b: " << b << std::endl;

    for(auto it = c.get_real_itr().cbegin(); it != c.get_real_itr().cend(); ++it) {
        std::cout << "c iteration " << it.get_interval() << std::endl;
    }

    boost::real::real g = "0.999998"_r;

    if (g < a) {
        std::cout << "g < a --> true" << std::endl;
    } else {
        std::cout << "g < a --> false" << std::endl;
    }

    auto h = a - g;
    std::cout << "a: " << a << std::endl;
    std::cout << "g: " << g << std::endl;
    std::cout << "h: " << h << std::endl;

    return 0;
}
```

### Output
```cpp
a: [0.99999899999999999999999999999999999999999999999999999999999999999999999999999999999999999933600778729, 0.99999899999999999999999999999999999999999999999999999999999999999999999999999999999999999982691714296]
b: [0.99999899999999999999999999999999999999999999999999999999999999999999999999999999999999999933600778729, 0.99999899999999999999999999999999999999999999999999999999999999999999999999999999999999999982691714296]
c iteration [1.99999799765646084706, 1.99999800138175115246]
c iteration [1.99999799999999999805656152704, 1.99999800000000000152600849191]
c iteration [1.99999799999999999999999999829434064267, 1.99999800000000000000000000152551492851]
c iteration [1.99999799999999999999999999999999999840487258578, 1.99999800000000000000000000000000000141413814631]
c iteration [1.99999799999999999999999999999999999999999999839920146096, 1.99999800000000000000000000000000000000000000120179841572]
c iteration [1.99999799999999999999999999999999999999999999999999999776247829313, 1.9999980000000000000000000000000000000000000000000000003726001095]
c iteration [1.99999799999999999999999999999999999999999999999999999999999999774936870769, 1.9999980000000000000000000000000000000000000000000000000000000001802340823]
c iteration [1.99999799999999999999999999999999999999999999999999999999999999999999999807650128953, 1.99999800000000000000000000000000000000000000000000000000000000000000000034042109297]
c iteration [1.99999799999999999999999999999999999999999999999999999999999999999999999999999999850405365298, 1.99999800000000000000000000000000000000000000000000000000000000000000000000000000061249327697]
g < a --> true
a: [0.99999899999999999999999999999999999999999999999999999999999999999999999999999999999999999933600778729, 0.99999899999999999999999999999999999999999999999999999999999999999999999999999999999999999982691714296]
g: [0.9999979999999999999999999999999999999999999999999999999999999999999999999999999999999999991629249303, 0.99999799999999999999999999999999999999999999999999999999999999999999999999999999999999999965383428597]
h: [0.00000099999999999999999999999999999999999999999999999999999999999999999999999999999999999968217350127, 0.00000100000000000000000000000000000000000000000000000000000000000000000000000000000000000066399221262]
```

## Current limitations
* Runtime efficiency is poor for certain operations
* Precision iterator behavior is undefined when operating between operands with different precisions


## References
    1. Computable calculus / Oliver Aberth, San Diego : Academic Press, c2001
    2. Lambov, B. (2007). RealLib: An efficient implementation of exact real arithmetic. Mathematical Structures in Computer Science, 17(1), 81-98.
    3. Aberth, O., & Schaefer, M. J. (1992). Precise computation using approximation_interval arithmetic, via C++. ACM Transactions on Mathematical Software (TOMS), 18(4), 481-491.
    4. https://en.cppreference.com/w/cpp/concept/ForwardIterator
