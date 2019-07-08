#ifndef BOOST_REAL_IRRATIONAL_HELPERS_HPP
#define BOOST_REAL_IRRATIONAL_HELPERS_HPP

#include <vector>
#include <algorithm>
#include <real/real.hpp>

namespace boost {
    namespace real {
        namespace irrational {

            /**
             * @brief The function returns the n-th digit of the champernowne number in the
             * binary integer version.
             *
             * @param n - The number digit index.
             * @return The value of the champernowne number n-th digit (either 0 or 1)
             */
            int champernowne_binary_get_nth_digit(unsigned int n) {
                std::vector<int> binary = {1};
                unsigned int index = 0;

                while (index < n) {

                    if (std::all_of(binary.begin(), binary.end(), [](int d) -> bool { return d == 1; })) {

                        for (int i = (int)binary.size() - 1; i >= 0; i--) {
                            binary[i] = 0;
                        }
                        binary.insert(binary.begin(), 1);

                    } else {

                        for (int i = (int)binary.size() - 1; i >= 0; i--) {
                            if (binary[i] == 0) {
                                for (int j = (int)binary.size() - 1; j > i; j--) {
                                    binary[j] = 0;
                                }
                                binary[i] = 1;
                                break;
                            }
                        }
                    }

                    index += binary.size();
                }

                return binary[binary.size() - 1 - (index - n)];
            }

            // e^x = sum_{k=0}^\inf = x^0/0! + x^1/1! + x^2/2! + x^3/3! + x^3/6! + ...

            class exponential {
                private:
                // would be nice to interoperate between long, int, and boost::real::real,
                // and have ctors from the integral types
                boost::real::real k_prev = boost::real::real_explicit("0");
                boost::real::real const * const x_ptr;
                boost::real::real last_term; // x^kn / kn!
                boost::real::real current_value; // summation from k0 to k_n, with precision digits

                public:
                exponential(boost::real::real &x) : x_ptr(&x) {
                    last_term = boost::real::real ("1");
                    current_value = boost::real::real ("1");
                };

                int get_nth_digit(unsigned int n) {
                    boost::real::real one = boost::real::real_explicit("1");
                    // if n < k_prev, reset

                    boost::real::real min_bound;
                    std::get<boost::real::real_explicit>(min_bound.get_real_number()).digits = {1};
                    std::get<boost::real::real_explicit>(min_bound.get_real_number()).exponent = 1-n;

                    // keep getting terms from the taylor series until the terms go below our precision bound
                    while((last_term > min_bound) || (last_term == min_bound)) {
                        last_term = last_term * (*x_ptr) / (k_prev + one);
                        current_value = current_value + last_term;
                    }

                    return 0;
                }

            };
        }
    }
}

#endif //BOOST_REAL_IRRATIONAL_HELPERS_HPP
