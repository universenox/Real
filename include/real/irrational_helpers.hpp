#ifndef BOOST_REAL_IRRATIONAL_HELPERS_HPP
#define BOOST_REAL_IRRATIONAL_HELPERS_HPP

#include <vector>
#include <algorithm>
#include <real/real.hpp>
#include<math.h>

namespace boost {
    namespace real {
        namespace irrational {
            // used for extra precision. should be replaced with something more definitive in the future.
            inline const int PLACEHOLDER = 10; 

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

            /// @TODO: figure out how to avoid unnecessary recalculation by saving
            // some information in real_algorithm

            // e^x = sum_{k=0}^\infty x^k / k! = 1 + x + x^2/2! + x^3/3! + x^4/4!
            // calculates e^x, where x = m/l
            template<int m, int l = 1>
            int exponential_get_nth_digit(unsigned int n) {
                std::vector<int> const one {1};
                std::vector<int> const zero {0};

                exact_number one_n {one, 1};
                exact_number last_term {one, 1}; // x^k_n / k_n!
                exact_number current_value; // summation up to k_n

                exact_number k;
                exact_number k_fac;
                exact_number const mn (m);
                exact_number const ln (l);
                std::cout << mn.as_string() << ", " << ln.as_string() << '\n';
                exact_number x = mn;

                /// @TODO look into possible precision problems
                x.divide_vector(ln, n+PLACEHOLDER);

                // prepare to calculate k=2 term
                k = one_n + one_n;
                last_term = x;
                current_value = one_n + x;

                // keep getting terms from the taylor series until the terms go below our precision bound
                /// @TODO: ensure cast doesn't overflow
                while(last_term.exponent >= 1-(int)(n+PLACEHOLDER)) {
                    last_term *= x;
                    last_term.divide_vector(k, n+PLACEHOLDER);
                    current_value += last_term;

                    k = k + one_n;
                }

                return current_value.digits[n];
            }
        }
    }
}

#endif //BOOST_REAL_IRRATIONAL_HELPERS_HPP
