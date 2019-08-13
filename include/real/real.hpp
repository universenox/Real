#ifndef BOOST_REAL_HPP
#define BOOST_REAL_HPP

#include <iostream>
#include <optional>
#include <vector>
#include <regex>
#include <initializer_list>
#include <sstream>
#include <utility>
#include <memory> // shared_ptr
#include <variant>

#include <real/real_exception.hpp>
#include <real/real_explicit.hpp>
#include <real/real_algorithm.hpp>
#include <real/real_operation.hpp>
#include <real/const_precision_iterator.hpp>
#include <real/real_data.hpp>


namespace boost {
    namespace real {

        /**
         * @brief boost::real::real represents real numbers as iterable intervals. Iterating the interval
         * increases the precision. This is done as needed (until a set maximum precision) to obtain
         * results from operations between reals. Reals support the basic operations addition, subtraction,
         * multiplication, division, and comparison (>, <, ==, etc.).
         *
         * @details A boost::real::real essentially acts as a wrapper around one of the next three representations:
         *
         *  1. Explicit
         *              Vector of digits with the most significant digits first. An exponent is used to determine
         *              the decimal place, so that the number is equal to {digits}*10^(-1 + exp). Essentially,
         *              the decimal starts to the left of the given digits, and the exponent shifts it. A
         *              boolean is used to set the number as positive (True) or negative (False)
         *
         *  2. Algorithm (@ref boost::real::real_algorithm)
         *               This representation is equivalent to the Explicit number but instead of using a vector
         *               of digits, a function pointer must be provided. The function takes an unsigned integer
         *               "n" as parameter and returns the n-th digit of the number.
         *
         *  3. Operation (@ref boost::real::real_operation)
         *              Composition of two numbers related by an operator (+, -, *, /). Such compositions
         *              are a binary tree whose leaves are operands, and parent nodes are real_operations
         *
         * Two boost::real::real numbers can be compared by the standard comparison operators, such as
         * '<'. In the case that the result cannot be determined using a precision <= maximum precision,
         * a precision_exception is thrown, such as when comparing something like .999...9 and .999...8.
         */


        /// @todo replace T with something more descriptive
        /// @todo Move constructors to move directly from the ctors in real_explicit to the values in real_data
        /// @todo do we need different ctors to be more efficient? rvalue and lvalue ref ctors?
        /// @todo power operation
        template <typename T = int>
        class real {
        private:
            std::shared_ptr<real_data<T>> _real_p;

            // ctor from shared_ptr to (already init) real_data. used in check_and_distribute.
            real(std::shared_ptr<real_data<T>> x) : _real_p(x){};

            // ctors from the 3 underlying types. private for now as I believe
            // the base change will not work if using the "inner" constructors directly.
            real(real_explicit<T> x) : _real_p(std::make_shared<real_data<T>>(x)) {};
            real(real_algorithm<T> x) : _real_p(std::make_shared<real_data<T>>(x)) {};
            real(real_operation<T> x) : _real_p(std::make_shared<real_data<T>>(x)) {};

            /**
             * @brief a helper function for distributing when performing addition/subtraction
             * @param assign_and_return_void is true when we wish to assign the result to *this
             * and not pass back a value for the std::optional<real>
             *
             * @returns a std::pair with a bool that is true if we distributed, false otherwise, and a std::real<optional>
             * that has a value when we distributed, and !assign_and_return_void.
             */
            std::pair<bool, std::optional<real>> check_and_distribute(real & other, bool assign_and_return_void, OPERATION op) {
                // The following simplifies using the distributive property, when numbers have the same pointers.
                // We could do comparison by value, but this may force more computation than is necessary for the user,
                // since it's difficult to determine whether values are the same

                std::shared_ptr<real_data<T>> a;
                std::shared_ptr<real_data<T>> b;
                std::shared_ptr<real_data<T>> x;

                if(auto op_ptr = std::get_if<real_operation<T>>(this->_real_p->get_real_ptr())) {
                    if (auto op_ptr2 = std::get_if<real_operation<T>>(other._real_p->get_real_ptr())) { // both of real_operation
                        if ((op_ptr->get_operation() == OPERATION::MULTIPLICATION) && op_ptr2->get_operation() == OPERATION::MULTIPLICATION) {
                            if (op_ptr->lhs() == op_ptr2->lhs()) { // x * a + x * b = (a + b) * x
                                a = op_ptr->rhs();
                                b = op_ptr2->rhs();
                                x = op_ptr->lhs();

                            } else if (op_ptr->lhs() == op_ptr2->rhs()) { // x * a + b * x = (a + b) * x
                                a = op_ptr->rhs();
                                b = op_ptr2->lhs();
                                x = op_ptr->lhs();

                            } else if (op_ptr->rhs() == op_ptr2->lhs()) { // a * x + x * b = (a + b) * x
                                a = op_ptr->lhs();
                                b = op_ptr2->rhs();
                                x = op_ptr->rhs();

                            } else if (op_ptr->rhs() == op_ptr2->rhs()) { // a * x + b * x = (a + b) * x
                                a = op_ptr->lhs();
                                b = op_ptr2->lhs();
                                x = op_ptr->rhs();
                            } else {
                                return std::make_pair(false, std::nullopt);
                            }

                            real<T> a_op_b;

                            if(op == OPERATION::ADDITION) {
                                a_op_b = real(a) + real(b);
                            }
                            else if (op == OPERATION::SUBTRACTION) {
                                a_op_b = real(a) + real(b);
                            } else {
                                throw invalid_distribution_operation_exception();
                            }

                            if(assign_and_return_void) {
                                this->_real_p = std::make_shared<real_data<T>>(real_operation(a_op_b._real_p, x, OPERATION::MULTIPLICATION));
                                return std::make_pair(true, std::nullopt);
                            } else {
                                return std::make_pair(true, real(real_operation(a_op_b._real_p, x, OPERATION::MULTIPLICATION)));
                            }
                        }
                    } else { // rhs is not an operation
                        if (op_ptr->get_operation() == OPERATION::MULTIPLICATION) {
                            if (other._real_p == op_ptr->lhs()) { // (a * x) + a -> (x + 1) * a
                                a = op_ptr->lhs();
                                x = op_ptr->rhs();

                            } else if (other._real_p == op_ptr->rhs()) {// (x * a) + a -> (x + 1) * a
                                a = op_ptr->rhs();
                                x = op_ptr->lhs();
                            } else {
                                return std::make_pair(false, std::nullopt);
                            }

                            real<T> one ("1");
                            real<T> x_op_1;
                            if(op == OPERATION::ADDITION) {
                                x_op_1 = real(x) + one;
                            }
                            else if (op == OPERATION::SUBTRACTION) {
                                x_op_1 = real(x) + one;
                            } else {
                                throw invalid_distribution_operation_exception();
                            }

                            if(assign_and_return_void) {
                                this->_real_p = std::make_shared<real_data<T>>(real_operation(x_op_1._real_p, a, OPERATION::MULTIPLICATION));
                                return std::make_pair(true, std::nullopt);
                            } else {
                                return std::make_pair(true, real(real_operation(x_op_1._real_p, a, OPERATION::MULTIPLICATION)));
                            }
                        }
                    }
                } else if(auto op_ptr = std::get_if<real_operation<T>>(&other._real_p->get_real_number())) { // lhs is not an operation, but rhs is
                    if (op_ptr->get_operation() == OPERATION::MULTIPLICATION) {
                        if (this->_real_p == op_ptr->lhs()) { // a + (a * x) -> (x + 1) * a
                            a = this->_real_p;
                            x = op_ptr->rhs();

                        } else if (this->_real_p == op_ptr->rhs()) {// a + (x * a) -> (x + 1) * a
                            a = this->_real_p;
                            x = op_ptr->lhs();
                        } else {
                            return std::make_pair(false, std::nullopt);
                        }

                        real<T> x_op_1;
                        real<T> one ("1");

                        if(op == OPERATION::ADDITION) {
                            x_op_1 = real(x) + one;
                        }
                        else if (op == OPERATION::SUBTRACTION) {
                            x_op_1 = real(x) + one;
                        } else {
                            throw invalid_distribution_operation_exception();
                        }

                        if(assign_and_return_void) {
                            this->_real_p = std::make_shared<real_data<T>>(real_operation(x_op_1._real_p, a, OPERATION::MULTIPLICATION));
                            return std::make_pair(true, std::nullopt);
                        } else {
                            return std::make_pair(true, real(real_operation(x_op_1._real_p, a, OPERATION::MULTIPLICATION)));
                        }
                    }
                } else { // neither is an operation
                    if ((this->_real_p == other._real_p) && (op == OPERATION::ADDITION)) { // a + a = 2 * a
                        std::shared_ptr<real_data<T>> two = std::make_shared<real_data<T>>(real_explicit("2"));

                        if(assign_and_return_void) {
                            this->_real_p = std::make_shared<real_data<T>>(real_operation(two, this->_real_p, OPERATION::MULTIPLICATION));
                            return std::make_pair(true, std::nullopt);
                        } else {
                            return std::make_pair(true, real(real_operation(two, this->_real_p, OPERATION::MULTIPLICATION)));
                        }
                    }
                }

                // at this point, we cannot distribute.
                return std::make_pair(false, std::nullopt);
            }

        public:

            /**
             * @brief *Default constructor:* Construct a boost::real::real with undefined representation
             * and behaviour.
             *
             * @note This constructor exists to allow working with other libraries as std::map or std::tuple
             */
            real() = default;

            /**
             * @brief *Copy constructor:* Creates a copy of the boost::real::real number other
             *
             * @param other - the boost::real::real instance to copy.
             */
             real(const real<T>& other)  : _real_p(other._real_p) {};


            /**
             * @brief String constructor. Returns an exact number if possible to be represented in internal base. Else division number is returned.
             *
             * @param number - a valid string representing a number.
             *
             * @throws boost::real::invalid_string_number exception if string doesn't represent a valid number
             */
            real(const std::string& number) {
                auto [integer_part, decimal_part, exponent, positive] = exact_number<>::number_from_string(number);

                if ((int)(decimal_part.length() + integer_part.length()) <= exponent) {
                    this->_real_p = std::make_shared<real_data<T>>(real_explicit<T>(integer_part, decimal_part, exponent, positive));
                } else {
                    int zeroes = decimal_part.length() + integer_part.length() - exponent;
                    std::string denominator = "1";
                    for (int i = 0; i<zeroes; ++i)
                        denominator = denominator + "0";

                    // source of inefficiency. copying, casting.
                    std::string numerator = (std::string) std::string(integer_part).c_str() + (std::string) std::string(decimal_part);
                    if (!positive)
                        numerator = "-" + numerator;
                    std::shared_ptr<real_data<T>> lhs = std::make_shared<real_data<T>>(real_explicit<T>(numerator));
                    std::shared_ptr<real_data<T>> rhs = std::make_shared<real_data<T>>(real_explicit<T>(denominator));

                    this->_real_p  = std::make_shared<real_data<T>>(real_operation(lhs, rhs, OPERATION::DIVISION));
                }
            }

            /**
             * @brief Initializer list constructor
             *
             * @param digits - a initializer_list<T> that represents the number digits.
             */
            real(std::initializer_list<T> digits)
                    : _real_p(std::make_shared<real_data<T>>(real_explicit<T>(digits, digits.size())))
                {};

            /**
             * @brief *Signed initializer list constructor:* Creates a boost::real::real
             * instance that represents the number where the positive parameter is used to set the
             * number sign and the elements of the digits parameter list are the number digits in
             * the same order.
             *
             * @param digits - an initializer_list<T> that represent the number digits.
             * @param positive - a bool that represents the number sign. If positive is set to true,
             * the number is positive, otherwise is negative.
             */
            real(std::initializer_list<T> digits, bool positive)
                    : _real_p(std::make_shared<real_data<T>>(real_explicit<T>(digits, digits.size(), positive)))
                    {};

            /**
             * @brief *Initializer list constructor with exponent:* Creates a boost::real::real
             * instance that represents the number where the exponent is used to set the number
             * integer part and the elements of the digits list are the digits the number in the same order.
             * The number is set as positive.
             *
             * @param digits - an initializer_list<T> that represent the number digits.
             * @param exponent - an integer representing the number exponent.
             */
            real(std::initializer_list<T> digits, int exponent)
                    : _real_p(std::make_shared<real_data<T>>(real_explicit<T>(digits, exponent)))
                    {};

            /**
             * @brief *Initializer list constructor with exponent and sign:* Creates a boost::real::real instance
             * that represents the number where the exponent is used to set the number integer part
             * and the elements of the digit list are the digits the number in the same order.
             * This constructor uses the sign to determine if the number is positive or negative.
             *
             * @param digits - an initializer_list<T> that represent the number digits.
             * @param exponent - an integer representing the number exponent.
             * @param positive - a bool that represent the number sign. If positive is set to true,
             * the number is positive, otherwise is negative.
             */
            real(::std::initializer_list<T> digits, int exponent, bool positive)
                    : _real_p(std::make_shared<real_data<T>>(real_explicit<T>(digits, exponent, positive)))
                    {};

            /**
             * @brief *Lambda function constructor with exponent:* Creates a boost::real::real
             * instance that represents the number where the exponent is used to set the number
             * integer part and the lambda function digits is used to know the number digits,
             * this function returns the n-th number digit.
             *
             * @param get_nth_digit - a function pointer or lambda function that given an unsigned
             * int "n" as parameter, it returns the number n-th digit.
             * @param exponent - an integer representing the number exponent.
             */
            real(T (*get_nth_digit)(unsigned int), int exponent)
                    : _real_p(std::make_shared<real_data<T>>(real_algorithm<T>(get_nth_digit, exponent)))
                    {};

            /**
             * @brief *Lambda function constructor with exponent and sign:* Creates a boost::real::real instance
             * that represents the number where the exponent is used to set the number integer part
             * and the lambda function digits is used to know the number digit, this function returns
             * the n-th number digit. This constructor uses the sign to determine if the number
             * is positive or negative.
             *
             * @param get_nth_digit - a function pointer or lambda function that given an unsigned
             * int "n" as parameter, it returns the number n-th digit.
             * @param exponent - an integer representing the number exponent.
             * @param positive - a bool that represent the number sign. If positive is set to true,
             * the number is positive, otherwise is negative.
             */
            real(T (*get_nth_digit)(unsigned int), int exponent, bool positive)
                 : _real_p(::std::make_shared<real_data<T>>(real_algorithm<T>(get_nth_digit, exponent, positive))) {};

            /**
             * @brief Default destructor
             */
            ~real() = default;

            /// @returns the variant containing the underlying real number
            const real_number<T>& get_real_number() {
                return _real_p->get_real_number();
            }

            /**
             * @returns the @ref boost::real::const_precision_iterator of the
             * underlying real number
             */
            const_precision_iterator<T> get_real_itr() const {
                return _real_p->get_precision_itr();
            }

            /**
             * @returns an integer with the maximum allowed precision.
             * @ref boost::real::const_precision_iterator::maximum_precision()
             */
            unsigned int maximum_precision() const {
                return get_real_itr().maximum_precision();
            }

            /// set max precision for the underlying iterator
            void set_maximum_precision(precision_t maximum_precision) {
                this->_real_p->get_precision_itr().set_maximum_precision(maximum_precision);
            }

            /// set global max precision
            static void set_global_maximum_precision(precision_t maximum_precision) {
                const_precision_iterator<T>::set_global_maximum_precision(maximum_precision);
            }

            /************** Operators ******************/

            /**
             * @brief If the number is an explicit or algorithm number it returns the n-th digit
             * of the represented number. If the number is an operation it throws an invalid_representation_exception
             *
             * @param n - an unsigned int number indicating the index of the requested digit.
             * @return an integer with the value of the number n-th digit.
             *
             * @throws boost::real::invalid_representation_exception
             */
            T operator[](unsigned int n) const {
                T ret;

                std::visit( overloaded { // perform operation on whatever is held in variant
                    [&n, &ret] (const real_explicit<T>& real)  {
                        ret = real[n];
                    },
                    [&n, &ret] (const real_algorithm<T>& real) {
                        ret = real[n];
                    },
                    [] (const real_operation<T>& real) {
                        throw boost::real::bad_variant_access_exception();
                    },
                    [] (auto& real) {
                        throw boost::real::bad_variant_access_exception();
                    }
                }, _real_p->get_real_number());
                return ret;
            }


            /// a constant used in the print_tree helper function
            static const int PRINT_SPACE = 5;
            /**
             * @brief a helper function for observing the tree within a real
             * @details the tree is displayed with a left 90 degree rotation
             * with the root on the left, and nodes on the next leftmost level
             */
            void print_tree(int space = PRINT_SPACE) {
                std::visit(overloaded {
                    [space] (const real_explicit<T>& real)  {
                        for (int i = PRINT_SPACE; i < space; i++)
                            std::cout << ' ';
                        std::cout << real.get_exact_number().as_string() << '\n';
                    },
                    [space] (const real_algorithm<T>& real) {
                        for (int i = PRINT_SPACE; i < space; i++)
                            std::cout << ' ';
                        std::cout << "alg\n";
                    },
                    [&space] (const real_operation<T>& real) {
                        ((boost::real::real<T>) real.rhs()).print_tree(space + PRINT_SPACE);
                        std::cout << '\n';

                        for (int i = PRINT_SPACE; i < space; i++)
                            std::cout << ' ';

                        switch(real.get_operation()) {
                            case OPERATION::ADDITION:
                                std::cout << "+";
                                break;
                            case OPERATION::SUBTRACTION:
                                std::cout << "-";
                                break;
                            case OPERATION::MULTIPLICATION:
                                std::cout << "*";
                                break;
                            case OPERATION::DIVISION:
                                std::cout << "/";
                                break;
                        }
                        std::cout << '\n';

                        ((boost::real::real<T>) real.lhs()).print_tree(space + PRINT_SPACE);
                    },
                    [] (auto& real) {
                        throw boost::real::bad_variant_access_exception();
                    }
                }, _real_p->get_real_number());
            }

            /* @brief used to control the amount of recursion that goes on in the distribution. Essentially, when we do a + b, we may
             * look at one level below them (if applicable, i.e., a, b, are also operations)

             * @details We at most need to look at two levels. This is because in the case we have, say, x*a+x*b -> (a+b)*x,
             * we may want to look at a and b's operands, if applicable, to see if they may be distributed as well. However,
             * we are not interested in a, b's operands' operands.
             */
            enum class RECURSION_LEVEL{ZERO, ONE, TWO};

            /*
             * @brief a helper function for distributing when performing addition/subtraction
             * @param other - the rhs operand
             * @param assign_and_return_void - if true, and if we can distribute, assigns the result
             * to *this. Else, returns the result in the std::optional<real>.
             * @param op - The operation. Only supports addition and subtraction.
             * @param rc_lvl - How many more levels down we have to look. We decrement the recursion
             * level every time we recurse.
             *
             * @returns a std::pair with a bool and std::optional<real>. The bool tells us whether
             * we distributed or not, which is useful when assign_and_return_void is true. If we did,
             * not distribute, then the option == std::nullopt. The std::optional<real> returns the
             * value from distributing the operation if !assign_and_return_void and if we distributed.
             */
            std::pair<bool, std::optional<real>> check_and_distribute(const real & other, const bool assign_and_return_void, const OPERATION op, const RECURSION_LEVEL rc_lvl) {
                // The following simplifies using the distributive property, when numbers have the same pointers.
                // We could do comparison by value, but this may force more computation than is necessary for the user,
                // since it's difficult to determine whether values are the same

                std::shared_ptr<real_data<T>> a;
                std::shared_ptr<real_data<T>> b;
                std::shared_ptr<real_data<T>> x;

                if(auto op_ptr = std::get_if<real_operation<T>>(this->_real_p->get_real_ptr())) { // lhs real_operation
                    if (auto op_ptr2 = std::get_if<real_operation<T>>(other._real_p->get_real_ptr())) { // lhs, rhs real_operation
                        if ((op_ptr->get_operation() == OPERATION::MULTIPLICATION) && op_ptr2->get_operation() == OPERATION::MULTIPLICATION) {
                            if (op_ptr->lhs() == op_ptr2->lhs()) { // x * a + x * b = (a + b) * x
                                a = op_ptr->rhs();
                                b = op_ptr2->rhs();
                                x = op_ptr->lhs();

                            } else if (op_ptr->lhs() == op_ptr2->rhs()) { // x * a + b * x = (a + b) * x
                                a = op_ptr->rhs();
                                b = op_ptr2->lhs();
                                x = op_ptr->lhs();

                            } else if (op_ptr->rhs() == op_ptr2->lhs()) { // a * x + x * b = (a + b) * x
                                a = op_ptr->lhs();
                                b = op_ptr2->rhs();
                                x = op_ptr->rhs();

                            } else if (op_ptr->rhs() == op_ptr2->rhs()) { // a * x + b * x = (a + b) * x
                                a = op_ptr->lhs();
                                b = op_ptr2->lhs();
                                x = op_ptr->rhs();
                            } else {
                                return std::make_pair(false, std::nullopt);
                            }

                            real<T> a_op_b;

                            if(op == OPERATION::ADDITION) {
                                switch(rc_lvl) {
                                    case (RECURSION_LEVEL::TWO):
                                        a_op_b = real(a).add(real(b), RECURSION_LEVEL::ONE);
                                        break;
                                    case (RECURSION_LEVEL::ONE):
                                        a_op_b = real(a).add(real(b), RECURSION_LEVEL::ZERO);
                                        break;
                                    default:
                                        throw invalid_recursion_level_exception();
                                }
                            } else if (op == OPERATION::SUBTRACTION) {
                                switch(rc_lvl) {
                                    case (RECURSION_LEVEL::TWO):
                                        a_op_b = real(a).subtract(real(b), RECURSION_LEVEL::ONE);
                                        break;
                                    case (RECURSION_LEVEL::ONE):
                                        a_op_b = real(a).subtract(real(b), RECURSION_LEVEL::ZERO);
                                        break;
                                    default:
                                        throw invalid_recursion_level_exception();
                                }
                            } else {
                                throw invalid_distribution_operation_exception();
                            }

                            if(assign_and_return_void) {
                                this->_real_p = std::make_shared<real_data<T>>(real_operation<T>(a_op_b._real_p, x, OPERATION::MULTIPLICATION));
                                return std::make_pair(true, std::nullopt);
                            } else {
                                return std::make_pair(true, real(real_operation<T>(a_op_b._real_p, x, OPERATION::MULTIPLICATION)));
                            }
                        }
                    } else { // lhs is an operation, but rhs is not an operation
                        if (op_ptr->get_operation() == OPERATION::MULTIPLICATION) {
                            if (other._real_p == op_ptr->lhs()) { // (a * x) + a -> (x + 1) * a
                                a = op_ptr->lhs();
                                x = op_ptr->rhs();

                            } else if (other._real_p == op_ptr->rhs()) {// (x * a) + a -> (x + 1) * a
                                a = op_ptr->rhs();
                                x = op_ptr->lhs();
                            } else {
                                return std::make_pair(false, std::nullopt);
                            }

                            real<T> one ("1");
                            real<T> x_op_1;

                            if(op == OPERATION::ADDITION) {
                                switch(rc_lvl) {
                                    case (RECURSION_LEVEL::TWO):
                                        x_op_1 = real(x).add(real(one), RECURSION_LEVEL::ONE);
                                        break;
                                    case (RECURSION_LEVEL::ONE):
                                        x_op_1 = real(a).add(real(one), RECURSION_LEVEL::ZERO);
                                        break;
                                    default:
                                        throw invalid_recursion_level_exception();
                                }
                            }
                            else if (op == OPERATION::SUBTRACTION) {
                                switch(rc_lvl) {
                                    case (RECURSION_LEVEL::TWO):
                                        x_op_1 = real(a).subtract(real(b), RECURSION_LEVEL::ONE);
                                        break;
                                    case (RECURSION_LEVEL::ONE):
                                        x_op_1 = real(a).subtract(real(b), RECURSION_LEVEL::ZERO);
                                        break;
                                    default:
                                        throw invalid_recursion_level_exception();
                                }
                            } else {
                                throw invalid_distribution_operation_exception();
                            }

                            if(assign_and_return_void) {
                                this->_real_p = std::make_shared<real_data<T>>(real_operation<T>(x_op_1._real_p, a, OPERATION::MULTIPLICATION));
                                return std::make_pair(true, std::nullopt);
                            } else {
                                return std::make_pair(true, real(real_operation<T>(x_op_1._real_p, a, OPERATION::MULTIPLICATION)));
                            }
                        }
                    }
                } else if(auto op_ptr = std::get_if<real_operation<T>>(&other._real_p->get_real_number())) { // lhs is not an operation, but rhs is
                    if (op_ptr->get_operation() == OPERATION::MULTIPLICATION) {
                        if (this->_real_p == op_ptr->lhs()) { // a + (a * x) -> (x + 1) * a
                            a = this->_real_p;
                            x = op_ptr->rhs();

                        } else if (this->_real_p == op_ptr->rhs()) {// a + (x * a) -> (x + 1) * a
                            a = this->_real_p;
                            x = op_ptr->lhs();
                        } else {
                            return std::make_pair(false, std::nullopt);
                        }

                        real<T> x_op_1;
                        real<T> one ("1");

                        if(op == OPERATION::ADDITION) {
                            switch(rc_lvl) {
                                case (RECURSION_LEVEL::TWO):
                                    x_op_1 = real(x).add(real(one), RECURSION_LEVEL::ONE);
                                    break;
                                case (RECURSION_LEVEL::ONE):
                                    x_op_1 = real(x).add(real(one), RECURSION_LEVEL::ZERO);
                                    break;
                                default:
                                    throw invalid_recursion_level_exception();
                            }
                        }
                        else if (op == OPERATION::SUBTRACTION) {
                            switch(rc_lvl) {
                                case (RECURSION_LEVEL::TWO):
                                    x_op_1 = real(a).subtract(real(b), RECURSION_LEVEL::ONE);
                                    break;
                                case (RECURSION_LEVEL::ONE):
                                    x_op_1 = real(a).subtract(real(b), RECURSION_LEVEL::ZERO);
                                    break;
                                default:
                                    throw invalid_recursion_level_exception();
                            }
                        } else {
                            throw invalid_distribution_operation_exception();
                        }

                        if(assign_and_return_void) {
                            this->_real_p = std::make_shared<real_data<T>>(real_operation(x_op_1._real_p, a, OPERATION::MULTIPLICATION));
                            return std::make_pair(true, std::nullopt);
                        } else {
                            return std::make_pair(true, real(real_operation(x_op_1._real_p, a, OPERATION::MULTIPLICATION)));
                        }
                    }
                } else { // neither is an operation
                    if ((this->_real_p == other._real_p) && (op == OPERATION::ADDITION)) { // a + a = 2 * a
                        std::shared_ptr<real_data<T>> two = std::make_shared<real_data<T>>(real_explicit<T>("2"));

                        if(assign_and_return_void) {
                            this->_real_p = std::make_shared<real_data<T>>(real_operation(two, this->_real_p, OPERATION::MULTIPLICATION));
                            return std::make_pair(true, std::nullopt);
                        } else {
                            return std::make_pair(true, real(real_operation(two, this->_real_p, OPERATION::MULTIPLICATION)));
                        }
                    }
                }

                // at this point, we cannot distribute.
                return std::make_pair(false, std::nullopt);
            }


            // helper function used in check_and_distribute to limit recursion
            real recurse_op(real& other, RECURSION_LEVEL rc_lvl, OPERATION op) {
                switch (rc_lvl) {
                    case RECURSION_LEVEL::ONE: {
                        auto [is_simplified, result] = check_and_distribute(other, false, op, RECURSION_LEVEL::ONE);

                        if (!is_simplified) {
                            real ret = (*this);
                            ret._real_p =
                                std::make_shared<real_data<T>>(real_operation(this->_real_p, other._real_p, op));
                            return ret;
                        } else {
                            return result.value();
                        }
                        return *this;
                        break;
                    }
                    case RECURSION_LEVEL::ZERO: {
                        real ret = (*this);
                        ret._real_p =
                            std::make_shared<real_data<T>>(real_operation(this->_real_p, other._real_p, op));
                        return ret;
                        break;
                    }
                    default:
                        throw invalid_recursion_level_exception();
                }
            }

            /// obtains the result of *this += other, and returns it.
            real add(real other, RECURSION_LEVEL rc_lvl) {
                return recurse_op(other, rc_lvl, OPERATION::ADDITION);
            }

            /// performs *this -= other, and returns *this
            real subtract(real other, RECURSION_LEVEL rc_lvl) {
                return recurse_op(other, rc_lvl, OPERATION::SUBTRACTION);
            }

            /**
             * @brief Sets this real_data to that of the operation between this previous
             * real_data and other real_data.
             *
             * If this is being pointed to by another operation, a copy of this is created.
             *
             * @param other - the right side operand boost::real::real number.
             */
            void operator+=(real other) {
                auto [is_simplified,result] = check_and_distribute(other, true, OPERATION::ADDITION, RECURSION_LEVEL::TWO);

                if (!is_simplified) {
                    this->_real_p =
                        std::make_shared<real_data<T>>(real_operation(this->_real_p, other._real_p, OPERATION::ADDITION));
                }
            }

            /**
             * @brief Creates a new boost::real::real_operation representing the sum of the
             * two numbers, using pointers to each operands' data.
             *
             * @param other - the right side operand boost::real::real number.
             * @return A copy of the new boost::real::real number representation.
             */
            real operator+(real<T> other) {
                auto [is_simplified, result] = check_and_distribute(other, false, OPERATION::ADDITION, RECURSION_LEVEL::TWO);
                if (is_simplified)  {
                    return result.value();
                } else {
                    return real<T>(real_operation<T>(this->_real_p, other._real_p, OPERATION::ADDITION));
                }
            }

            /**
             * @brief Sets this real_data to that of the operation between this previous
             * real_data and other real_data.
             *
             * @param other - the right side operand boost::real::real number.
             */
            void operator-=(real<T> other) {
                auto [is_simplified, result] = check_and_distribute(other, true, OPERATION::SUBTRACTION, RECURSION_LEVEL::TWO);

                if(!is_simplified) {
                    this->_real_p =
                        std::make_shared<real_data<T>>(real_operation(this->_real_p, other._real_p, OPERATION::SUBTRACTION));
                }
            }

            /**
             * @brief Creates a new boost::real::real representing the subtraction
             * between *this and other
             *
             * @param other - the right side operand boost::real::real number.
             * @return A copy of the new boost::real::real number representation.
             */
            real operator-(real<T> other) {
                auto [is_simplified, result] = check_and_distribute(other, false, OPERATION::SUBTRACTION, RECURSION_LEVEL::TWO);
                if (is_simplified)  {
                    return result.value();
                } else {
                    return real(real_operation<T>(this->_real_p, other._real_p, OPERATION::SUBTRACTION));
                }
            }

            /**
             * @brief Sets this real_data to that of the operation between
             * this previous real_data and other real_data.
             *
             * @param other - the right side operand boost::real::real number.
             */
            void operator*=(real<T> other) {
                this->_real_p =
                    std::make_shared<real_data<T>>(real_operation<T>(this->_real_p, other._real_p, OPERATION::MULTIPLICATION));
            }

            /**
             * @brief Creates a new boost::real::real representing the product
             * of *this and other
             *
             * @param other - the right side operand boost::real::real number.
             * @return A copy of the new boost::real::real number representation.
             */
            real<T> operator*(real<T> other) {
                return real(real_operation<T>(this->_real_p, other._real_p, OPERATION::MULTIPLICATION));
            }

            /**
             * @brief Creates a new boost::real::real representing the product
             * of *this and other
             *
             * @param other - the right side operand boost::real::real number.
             * @return A copy of the new boost::real::real number representation.
             */
            real<T> operator/(real<T> other) {
                return real(real_operation<T>(this->_real_p, other._real_p, OPERATION::DIVISION));
            }

            /**
             * @brief Sets this real_data to that of the operation between
             * this previous real_data and other real_data.
             *
             * @param other - the right side operand boost::real::real number.
             */
            void operator/=(real<T> other) {
                this->_real_p =
                    std::make_shared<real_data<T>>(real_operation<T>(this->_real_p, other._real_p, OPERATION::DIVISION));
            }

            /**
             * @brief Assigns *this to other
             * @param other - the boost::real::real number to copy.
             */
            void operator=(real<T> other) {
                this->_real_p = other._real_p;
            }

            /**
             * @brief Assigns *this to the real number represented by the string number
             * @param number - a valid string representing a number.
             */
            void operator=(const std::string& number) {
                this->_real_p =
                    std::make_shared<real_data<T>>(real_explicit<T>(number));
            }

            /**
             * @brief Compares the *this boost::real::real number against the other boost::real::real number to
             * determine if the number represented by *this is lower than the number represented by other.
             * If the maximum precision is reached and the operator was not yet able to determine
             * the value of the result, a precision_exception is thrown.
             *
             * @param other - a boost::real::real number to compare against.
             * @return a bool that is true if *this < other and false in other cases.
             *
             * @throws boost::real::precision_exception
             */
            bool operator<(const real<T>& other) const {
                auto this_it = this->_real_p->get_precision_itr().cbegin();
                auto other_it = other._real_p->get_precision_itr().cbegin();

                if (this_it == other_it)
                    return false;

                unsigned int current_precision = std::max(this->maximum_precision(), other.maximum_precision());
                for (unsigned int p = 0; p < current_precision; ++p) {
                    // Get more precision
                    ++this_it;
                    ++other_it;

                    bool this_full_precision = this_it.get_interval().is_a_number();
                    bool other_full_precision = other_it.get_interval().is_a_number();
                    if (this_full_precision && other_full_precision) {
                        return this_it.get_interval() < other_it.get_interval();
                    }

                    if (this_it.get_interval()< other_it.get_interval()) {
                        return true;
                    }

                    if (other_it.get_interval()< this_it.get_interval()) {
                        return false;
                    }
                }

                // If the precision is reached and the number ranges still overlap, then we cannot
                // know if they are equals or other is less than this and we throw an error.
                throw boost::real::precision_exception();
            }

            /**
             * @brief Compares the *this boost::real::real number against the other boost::real::real number to
             * determine if the number represented by *this is greater than the number represented by other.
             * If the maximum precision is reached and the operator was not yet able to determine
             * the value of the result, a precision_exception is thrown.
             *
             * @param other - a boost::real::real number to compare against.
             * @return a bool that is true if *this > other and false in other cases.
             *
             * @throws boost::real::precision_exception
             */
            bool operator>(const real<T>& other) const {
                auto this_it = this->_real_p->get_precision_itr().cbegin();
                auto other_it = other._real_p->get_precision_itr().cbegin();

                if (this_it == other_it)
                    return false;

                unsigned int current_precision = std::max(this->maximum_precision(), other.maximum_precision());
                for (unsigned int p = 0; p < current_precision; ++p) {
                    // Get more precision
                    ++this_it;
                    ++other_it;

                    bool this_full_precision = this_it.get_interval().is_a_number();
                    bool other_full_precision = other_it.get_interval().is_a_number();

                    if (this_full_precision && other_full_precision) {
                        return this_it.get_interval() > other_it.get_interval();
                    }

                    if (this_it.get_interval()> other_it.get_interval()) {
                        return true;
                    }

                    if (other_it.get_interval()> this_it.get_interval()) {
                        return false;
                    }
                }

                // If the precision is reached and the number ranges still overlap, then we cannot
                // know if they are equals or other is less than this and we throw an error.
                throw boost::real::precision_exception();
            }

            /**
             * @brief Compares the *this boost::real::real number against the other boost::real::real number to
             * determine if the number represented by *this is the same than the number represented by other.
             * If the maximum precision is reached and the operator was not yet able to determine
             * the value of the result, a precision_exception is thrown.
             *
             * @param other - a boost::real::real number to compare against.
             * @return a bool that is true if *this < other and false in other cases.
             *
             * @throws boost::real::precision_exception
             */
            bool operator==(const real<T>& other) const {
                auto this_it = _real_p->get_precision_itr().cbegin();
                auto other_it = other._real_p->get_precision_itr().cbegin();

                unsigned int current_precision = std::max(this->maximum_precision(), other.maximum_precision());
                for (unsigned int p = 0; p < current_precision; ++p) {
                    // Get more precision
                    ++this_it;
                    ++other_it;

                    bool this_full_precision = this_it.get_interval().is_a_number();
                    bool other_full_precision = other_it.get_interval().is_a_number();
                    if (this_full_precision && other_full_precision) {
                        return this_it.get_interval()== other_it.get_interval();
                    }

                    bool this_is_lower = this_it.get_interval()< other_it.get_interval();
                    bool other_is_lower = other_it.get_interval()< this_it.get_interval();
                    if (this_is_lower || other_is_lower) {
                        return false;
                    }
                }

                // If the precision is reached and the numbers full precision is not reached, then
                // we cannot know if they are equals or not.
                throw boost::real::precision_exception();
            }
            /********* END OPERATORS *********/

            /**
             * @brief overload of the << operator for std::ostream and boost::real::real
             *
             * @param os - The std::ostream object where to print the r number.
             * @param r - the boost::real::real number to print
             * @return a reference of the modified os object.
             */
            friend std::ostream& operator<<(std::ostream& os, real r) {
                os << r.get_real_itr().cend().get_interval();
                return os;
            }

        }; // end real class
    }
}

//User Defined Literals
inline auto operator "" _r(long double x) {
    return boost::real::real<int>(std::to_string(x));
}

inline auto operator "" _r(unsigned long long x) {
    return boost::real::real<int>(std::to_string(x));
}

inline auto operator "" _r(const char* x, size_t len) {
    return boost::real::real<int>(x);
}

#endif //BOOST_REAL_HPP
