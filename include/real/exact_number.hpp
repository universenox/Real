#ifndef BOOST_REAL_EXACT_NUMBER
#define BOOST_REAL_EXACT_NUMBER

#include <vector>
#include <string>
#include <algorithm>

namespace boost {
    namespace real {
        struct exact_number {
            std::vector<int> digits = {};
            int exponent = 0;
            bool positive = true;

            static bool aligned_vectors_is_lower(const std::vector<int> &lhs, const std::vector<int> &rhs) {

                // Check if lhs is lower than rhs
                auto lhs_it = lhs.cbegin();
                auto rhs_it = rhs.cbegin();
                while (rhs_it != rhs.end() && lhs_it != lhs.end() && *lhs_it == *rhs_it) {
                    ++lhs_it;
                    ++rhs_it;
                }

                if (rhs_it != rhs.cend() && lhs_it != lhs.cend()) {
                    return *lhs_it < *rhs_it;
                }

                bool lhs_all_zero = std::all_of(lhs_it, lhs.cend(), [](int i){ return i == 0; });
                bool rhs_all_zero = std::all_of(rhs_it, rhs.cend(), [](int i){ return i == 0; });

                return lhs_all_zero && !rhs_all_zero;
            }

            /// adds other to *this. disregards sign -- that's taken care of in the operators.
            void add_vector(exact_number &other){
                int carry = 0;
                std::vector<int> temp;
                int fractional_length = std::max((int)digits.size() - exponent, (int)other.size() - other.exponent);
                int integral_length = std::max(this->exponent, other.exponent);

                // we walk the numbers from the lowest to the highest digit
                for (int i = fractional_length - 1; i >= -integral_length; i--) {

                    int lhs_digit = 0;
                    if (0 <= this->exponent + i && this->exponent + i < (int)this->digits.size()) {
                        lhs_digit = this->digits[this->exponent + i];
                    }

                    int rhs_digit = 0;
                    if (0 <= other.exponent + i && other.exponent + i < (int)other.digits.size()) {
                        rhs_digit = other.digits[other.exponent + i];
                    }
                    int digit = carry + lhs_digit + rhs_digit;

                    if (digit > 9) {
                        carry = 1;
                        digit -= 10;
                    } else {
                        carry = 0;
                    }

                    temp.insert(temp.cbegin(), digit);
                    }
                if (carry == 1) {
                    temp.insert(temp.cbegin(), 1);
                    integral_length++;
                }
                this->digits = temp;
                this->exponent = integral_length;
                this->normalize();
            }

            /// subtracts other from *this, disregards sign -- that's taken care of in the operators
            void subtract_vector(exact_number &other) {
                std::vector<int> result;
                int fractional_length = std::max((int)digits.size() - exponent, (int)other.size() - other.exponent);
                int integral_length = std::max(this->exponent, other.exponent);
                int borrow = 0;


                // we walk the numbers from the lowest to the highest digit
                for (int i = fractional_length - 1; i >= -integral_length; i--) {

                    int lhs_digit = 0;
                    if (0 <= this->exponent + i && this->exponent + i < (int)this->digits.size()) {
                        lhs_digit = this->digits[this->exponent + i];
                    }

                    int rhs_digit = 0;
                    if (0 <= other.exponent + i && other.exponent + i < (int)other.digits.size()) {
                        rhs_digit = other.digits[other.exponent + i];
                    }

                    if (lhs_digit < borrow) {
                        lhs_digit += (10 - borrow);
                    } else {
                        lhs_digit -= borrow;
                        borrow = 0;
                    }

                    if (lhs_digit < rhs_digit) {
                        lhs_digit += 10;
                        borrow++;
                    }

                    result.insert(result.cbegin(), lhs_digit - rhs_digit);
                }
                this->digits = result;
                this->exponent = integral_length;
                this->normalize();
            }

            /// multiplies *this by other
            void multiply_vector(exact_number &other) {
                // will keep the result number in vector in reverse order
                // Digits: .123 | Exponent: -3 | .000123 <--- Number size is the Digits size less the exponent
                // Digits: .123 | Exponent: 2  | 12.3
                std::vector<int> temp;
                size_t new_size = digits.size() + other.digits.size();
                if (this->exponent < 0) new_size -= this->exponent; // <--- Less the exponent
                if (other.exponent < 0) new_size -= other.exponent; // <--- Less the exponent

                for (int i = 0; i < (int)new_size; i++) temp.push_back(0);
                // TODO: Check why the assign method crashes.
                //result.assign(new_size, 0);

                // Below two indexes are used to find positions
                // in result.
                auto i_n1 = (int) temp.size() - 1;
                // Go from right to left in digits
                for (int i = (int)digits.size()-1; i>=0; i--) {
                    int carry = 0;

                    // To shift position to left after every
                    // multiplication of a digit in rhs
                    int i_n2 = 0;

                    // Go from right to left in rhs
                    for (int j = (int)other.digits.size()-1; j>=0; j--) {

                        // Multiply current digit of second number with current digit of first number
                        // and add result to previously stored result at current position.
                        int sum = digits[i]*other.digits[j] + temp[i_n1 - i_n2] + carry;

                        // Carry for next iteration
                        carry = sum / 10;

                        // Store result
                        temp[i_n1 - i_n2] = sum % 10;

                        i_n2++;
                    }

                    // store carry in next cell
                    if (carry > 0) {
                        temp[i_n1 - i_n2] += carry;
                    }

                    // To shift position to left after every
                    // multiplication of a digit in digits.
                    i_n1--;
                }

                int fractional_part = ((int)digits.size() - this->exponent) + ((int)other.digits.size() - other.exponent);
                int result_exponent = (int)temp.size() - fractional_part;
                
                digits = temp;
                exponent = result_exponent;
                this->positive = this->positive == other.positive;
                this->normalize();
            }

            /// calculates *this / divisor 
            /// 
            ///  @brief a binary-search type method for dividing exact_numbers.
            ///  @param is_upper true: returns result with an error of +epsilon, while
            ///                  false: returns result with an error of -epsilon
            void divide_vector(exact_number divisor, bool is_upper, unsigned int max_precision) {
                /// @TODO: replace this with something more efficient, like newton-raphson method
                // it also completely recalculates on each precision increase
                // instead, could use previous information to make better "guesses"
                // for our iteration scheme.

                /// @TODO: convert div by negative exponents 
                // 1/.001 = 1/(1/1000) = 1000
                // 1.46 / .12 = 1.46 * 100 * 1/12
                // 1 / .23 = 1 * 100 * (1/23)
                // etc.,
                // after this, no division by 0 < D < 1
                boost::real::exact_number numerator;
                boost::real::exact_number left;
                boost::real::exact_number right;
                boost::real::exact_number residual;
                boost::real::exact_number tmp;
                boost::real::exact_number half;
                boost::real::exact_number distance;
                boost::real::exact_number min_boundary_n;
                boost::real::exact_number min_boundary_p;

                bool positive = ((*this).positive == divisor.positive);
                numerator = (*this).abs();
                divisor = divisor.abs();

                min_boundary_n.digits = {1};
                ///@TODO ensure exponent doesn't overflow
                min_boundary_n.exponent = -1 * (max_precision);
                min_boundary_n.positive = false;

                min_boundary_p.digits = {1};
                min_boundary_p.exponent = -1 * (max_precision);

                half.digits = {5};
                half.exponent = 0;

                tmp.digits = {1};
                tmp.exponent = 1;

                if (divisor == tmp) {
                    (*this).positive = positive;
                    return;
                }

                if (divisor == (*this)) { 
                    (*this) = tmp;
                    (*this).positive = positive;
                    return;
                }

                ///@TODO: remember signs at the end of this function

                exact_number zero = exact_number(); 

                if (divisor == zero)
                    throw(boost::real::divide_by_zero());
                else if ((divisor > zero) && (divisor < tmp)) // 0 < d < 1
                    throw(boost::real::invalid_denominator());

                // N < D --> 0 < abs(Q) < 1
                if ((*this) < divisor) {
                        left = exact_number(); // 0
                        right = tmp; // 1
                    } else { // assuming D > 1. N > D ---> 1 < N / D < N
                        left = tmp; // 1
                        right = (*this);
                    }

                // Example: say we have 144 / 12. At min precision, this is
                // [100, 200] / [10, 20]
                // so, our quotient upper bound would be 200/10, and 
                // the lower bound would be 100/20.

                /// @TODO: the following
                // if (*this) == 0, return 0 
                // if divisor == 1, return (*this)
                // if divisor == 0, throw error

                // distance = (right - left) / 2
                distance = (right - left) * half;
                (*this) = left + distance;

                // residual = (*this) * denom - num, equals zero if numerator/divisor = (*this)
                residual = (*this) * divisor - numerator;

                // calculate the result
                // continue the loop while we are still inaccurate (up to max precision), or while
                // we are on the wrong side of the answer
                while ((residual.abs() > min_boundary_p) || 
                        // (is_upper && (residual < min_boundary_n)) || (!is_upper && (residual > min_boundary_p))) &&
                        (distance.exponent > min_boundary_p.exponent)) {
                    /// TODO: we might exit the loop early due to the last statement. 
                    /// Verify our answers are within +- epsilon of the solution.

                    // result too small, try halfway between (*this) and (*this) 
                    if (residual < min_boundary_n) {
                        left = (*this);
                    }
                    // distance is halved
                    distance = half * distance;
                    distance.normalize();

                    // truncate insignificant digits of distance
                    // NOTE: The loop will not terminate if you truncate too much, for certain
                    // divisions. 5 seems to work well, so I'm leaving it as that.
                    // we may want to look at this in closer detail
                    while (distance.size() > max_precision + 5)
                        distance.digits.pop_back();

                    // iterate (*this)
                    (*this) = left + distance;

                    // truncate insignificant digits of (*this)
                    while ((*this).size() > max_precision + 5)
                        (*this).digits.pop_back();

                    // recalculate residual  N/D = Q ---> QD - N = residual
                    residual = ((*this) * divisor) - numerator;
                    residual.normalize();
                } // end while
                // now (*this) is correct, or at least within +-epsilon of correct value 
                // truncate (*this)
                this->normalize();

                while ((*this).size() > max_precision)
                    (*this).digits.pop_back();


                // recalculate residual for the final (*this) value
                exact_number residual_o = ((*this) * divisor) - numerator;

                // note we have to normalize before comparison, because -0.0 != zero ..
                residual_o.normalize();

                if (residual_o != zero) { // then, we are not fully accurate
                    // we try seeing if we can make the residual equal zero by adding/subtracting epsilon
                    exact_number tmp_lower = (*this);
                    tmp_lower.round_down();
                    exact_number tmp_upper = (*this);
                    tmp_upper.round_up();

                    residual = tmp_lower * divisor - numerator;
                    residual.normalize();

                    if (residual == zero) {
                        (*this) = tmp_lower;

                        if (positive)
                            (*this).positive = true;
                        else
                            (*this).positive = false;

                        (*this).normalize();
                        return;
                    } 


                    residual = tmp_upper * divisor - numerator;
                    residual.normalize();

                    if (residual == zero) {
                        (*this) = tmp_upper;

                        if (positive)
                            (*this).positive = true;
                        else
                            (*this).positive = false;

                        (*this).normalize();
                        return;
                    }
                    // at this point, it is impossible to make the residual 0
                    if (is_upper)
                        (*this) = tmp_upper;
                } else { // residual_o == 0
                    if (positive)
                        (*this).positive = true;
                    else
                        (*this).positive = false;
                    (*this).normalize();
                }
            }

            void round_up() {
                int index = digits.size() - 1;
                bool keep_carrying = true;

                while((index > 0) && keep_carrying) { // bring the carry back
                    if(this->digits[index] != 9) {
                        ++this->digits[index];
                        keep_carrying = false;
                    } else // digits[index] == 9, we keep carrying
                        this->digits[index] = 0;
                    --index;
                }

                if ((index == 0) && keep_carrying) { // i.e., .999 should become 1.000
                    if(this->digits[index] == 9) {
                        this->digits[index] = 0;
                        this->push_front(1);
                        ++this->exponent;
                    }
                    else
                        ++this->digits[index];
                }
            }

            void round_down() {
                int index = digits.size() - 1;
                bool keep_carrying = true;

                while((index > 0) && keep_carrying) { // bring the carry back
                    if(this->digits[index] != 0) {
                        --this->digits[index];
                        keep_carrying = false;
                    } else // digits[index] == 0, we keep carrying
                        this->digits[index] = 9;
                    --index;
                }
                // we should be ok at this point because the first number in digits should != 0
                if (index == 0 && keep_carrying)
                    --this->digits[index];
            }

            /**
             * @brief *default constructor*: It constructs a representation of the number zero.
             */
            exact_number() = default;

            /// ctor from vector of digits, integer exponent, and optional bool positive
            exact_number(std::vector<int> vec, int exp, bool pos = true) : digits(vec), exponent(exp), positive(pos) {};

            /**
             * @brief *Copy constructor:* It constructs a new boost::real::exact_number that is a copy of the
             * other boost::real::exact_number.
             *
             * @param other - The boost::real::exact_number to copy.
             */
            exact_number(const exact_number &other) = default;

            /// TODO: move ctor

            /**
             * @brief Default asignment operator.
             *
             * @param other - The boost::real::exact_number to copy.
             */
            exact_number &operator=(const exact_number& other) = default;

            /**
             * @brief *Lower comparator operator:* It compares the *this boost::real::exact_number with the other
             * boost::real::exact_number to determine if *this is lower than other.
             *
             * @param other - The right side operand boost::real::exact_number to compare with *this.
             * @return a bool that is true if and only if *this is lower than other.
             */
            bool operator<(const exact_number& other) const {
                std::vector<int> zero = {0};
                std::vector<int> empty = {};
                if (this->digits == zero || this->digits == empty) {
                    if (other.digits == zero || !other.positive || other.digits == empty)
                        return false;
                    else
                        return true;
                } else {
                    if ((other.digits == zero || other.digits == empty) && !this->positive)
                        return true;
                }

                if (this->positive != other.positive) {
                    return !this->positive;
                }

                if (this->positive) {
                    if (this->exponent == other.exponent) {
                        return aligned_vectors_is_lower(this->digits, other.digits);
                    }

                    return this->exponent < other.exponent;
                }

                if (this->exponent == other.exponent) {
                    return aligned_vectors_is_lower(other.digits, this->digits);
                }

                return other.exponent < this->exponent;
            }

            /**
             * @brief *Greater comparator operator:* It compares the *this boost::real::exact_number with the other
             * boost::real::exact_number to determine if *this is greater than other.
             *
             * @param other - The right side operand boost::real::exact_number to compare with *this.
             * @return a bool that is true if and only if *this is greater than other.
             */
            bool operator>(const exact_number& other) const {
                std::vector<int> zero = {0};
                std::vector<int> empty = {};
                if (this->digits == zero || this->digits == empty) {
                    if (other.digits == zero || other.positive || other.digits == empty)
                        return false;
                    else
                        return true;
                } else {
                    if ((other.digits == zero || other.digits == empty) && this->positive)
                        return true;
                }

                if (this->positive != other.positive) {
                    return this->positive;
                }

                if (this->positive) {
                    if (this->exponent == other.exponent) {
                        return aligned_vectors_is_lower(other.digits, this->digits);
                    }

                    return this->exponent > other.exponent;
                }

                if (this->exponent == other.exponent) {
                    return aligned_vectors_is_lower(this->digits, other.digits);
                }

                return other.exponent > this->exponent;
            }

            /**
             * @brief *Equality comparator operator:* It compares the *this boost::real::exact_number with the other
             * boost::real::exact_number to determine if *this and other are equals or not.
             *
             * @param other - The right side operand boost::real::exact_number to compare with *this.
             * @return a bool that is true if and only if *this is equal to other.
             */
            bool operator==(const exact_number& other) const {
                return !(*this < other || other < *this);
            }

            bool operator!=(const exact_number& other) const {
                return !(*this == other);
            }



            exact_number abs() {
                exact_number result = (*this);
                result.positive = true;
                return result;
            }

            exact_number operator+(exact_number other) {
                exact_number result = *this;
                result.add_vector(other);

                if (positive == other.positive) {
                        result = *this;
                        result.add_vector(other);
                } else if (other.abs() < this->abs()) {
                        result = *this;
                        result.subtract_vector(other);
                } else {
                    result = other;
                    result.subtract_vector(*this);
                }
                return result;
            }

            void operator+=(exact_number &other) {
                *this = *this + other;
            }

            exact_number operator-(exact_number other) {
                exact_number result;

                if (this->positive != other.positive) {
                    result = *this;
                    result.add_vector(other);
                } else {
                    if (other.abs() < this->abs()) {
                        result = *this;
                        result.subtract_vector(other);
                    } else {
                        result = other;
                        result.subtract_vector(*this);
                        result.positive = !this->positive;
                    }
                }

                return result;
            }

            void operator-=(exact_number &other) {
                *this = *this - other;
            }

            exact_number operator*(exact_number other) {
                exact_number result = *this;
                result.multiply_vector(other);
                return result;
            }

            void operator*=(exact_number &other) {
                *this = *this * other;
            }


            /**
             * @brief Generates a string representation of the boost::real::exact_number.
             *
             * @return a string that represents the state of the boost::real::exact_number
             */
            std::basic_string<char> as_string() const {
                std::basic_string<char> result = "";

                if (!this->positive) {
                    result = "-";
                }

                // If the number is too large, scientific notation is used to print it.
                if ((this->exponent < -10) || (this->exponent > (int)this->digits.size() + 10)) {
                    result += "0.";

                    for (const auto& d: this->digits) {
                        result += std::to_string(d);
                    }

                    result += "e" + std::to_string(this->exponent);
                    return result;
                }

                if (this->exponent <= 0) {
                    result += "0.";

                    for (int i = this->exponent; i < (int) this->digits.size(); ++i) {
                        if (i < 0) {
                            result += "0";
                        } else {
                            result += std::to_string(this->digits[i]);
                        }
                    }
                } else {
                    int digit_amount = std::max(this->exponent, (int) this->digits.size());

                    for (int i = 0; i < digit_amount; ++i) {

                        if (i == this->exponent) {
                            result += ".";
                        }

                        if (i < (int) this->digits.size()) {
                            result += std::to_string(this->digits[i]);
                        } else {
                            result += "0";
                        }
                    }

                    if (result.back() == '.') {
                        result.pop_back();
                    }
                }
                return result;
            }

            /**
             * @brief Swaps the boost::real::exact_number value with the value of the other boost::real::exact_number.
             * This operation is a more preformant form of swapping to boost::real::boundaries.
             *
             * @param other - The boost::real::exact_number to swap with *this.
             */
            void swap(exact_number &other) {
                this->digits.swap(other.digits);
                std::swap(this->exponent, other.exponent);
                std::swap(this->positive, other.positive);
            }

            /**
             * @brief add the digit parameter as a new digit of the boost::real::exact_number. The digit
             * is added in the right side of the number.
             *
             * @param digit - The new digit to add.
             */
            void push_back(int digit) {
                this->digits.push_back(digit);
            }

            /**
             * @brief add the digit parameter as a new digit of the boost::real::exact_number. The digit
             * is added in the left side of the number.
             *
             * @param digit - The new digit to add.
             */
            void push_front(int digit) {
                this->digits.insert(this->digits.cbegin(), digit);
            }

            /**
             * @brief Removes extra zeros at the sides to convert the number representation into a
             * normalized representation.
             */
            void normalize() {
                while (this->digits.size() > 1 && this->digits.front() == 0) {
                    this->digits.erase(this->digits.cbegin());
                    this->exponent--;
                }

                while (this->digits.size() > 1 && this->digits.back() == 0) {
                    this->digits.pop_back();
                }

                // Zero could have many representation, and the normalized is the next one.
                if (this->digits.size() == 1 && this->digits.front() == 0) {
                    this->exponent = 0;
                    this->positive = true;
                }
            }

            /**
             * @brief Removes extra zeros at the left side to convert the number representation
             * into a semi normalized representation.
             */
            void normalize_left() {
                while (this->digits.size() > 1 && this->digits.front() == 0) {
                    this->digits.erase(this->digits.cbegin());
                    this->exponent--;
                }
            }

            /**
             * @brief ir clears the number digits.
             */
            void clear() {
                this->digits.clear();
            }

            /**
             * @brief Returns the n-th digit of the boost::real::exact_number.
             *
             * @param n - an int number indicating the index of the requested digit.
             * @return an integer with the value of the number n-th digit.
             */
            int &operator[](int n) {
                return this->digits[n];
            }

            /**
             * @brief It returns the number of digits of the boost::real::exact_number
             *
             * @return an unsigned long representing the number of digits of the boost::real::exact_number
             */
            unsigned long size() {
                return this->digits.size();
            }

        };
    }
}

#endif // BOOST_REAL_EXACT_NUMBER