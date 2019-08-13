#ifndef BOOST_REAL_REAL_OPERATION
#define BOOST_REAL_REAL_OPERATION

#include <memory> // shared_ptr

#include <real/real_algorithm.hpp>
#include <real/real_explicit.hpp>

namespace boost{
    namespace real{

        // necessary forward declarations
        template <typename T>
        class real_data;

        template <typename T>
        class const_precision_iterator;


        /**
         * @brief an enum used to determine the kind of real_operation we will
         * perform between the operands
         * 
         * @todo add more operations, such as those found in math.h
         */
        enum class OPERATION{ADDITION, SUBTRACTION, MULTIPLICATION, DIVISION}; 

        /*
         * @brief real_operation is an unbalanced binary tree representation of operations, where
         * the leaves are the operands and the nodes store the type of operation
         * 
         * @todo consider ways to make the representation better. an unbalanced binary tree is inefficient.
         * @warning due to the recursive nature of real_operation, stack overflow is a possible issue
         */
        template <typename T = int>
        class real_operation{
        private:
            std::shared_ptr<real_data<T>> _lhs; ///< points to the lhs operand
            std::shared_ptr<real_data<T>> _rhs; ///< points to the rhs operand
            OPERATION _operation; ///< the type of operation to perform between the operands

        public:

            /*
             * @brief Constructor from two operands and an operation
             * @param lhs - left operand
             * @param rhs - right operand
             * @param op  - operation between the operands
             */
            real_operation(std::shared_ptr<real_data<T>> &lhs, std::shared_ptr<real_data<T>> &rhs, OPERATION op) : _lhs(lhs), _rhs(rhs), _operation(op) {};

            OPERATION get_operation() const {
                return _operation;
            }

            // fwd decl'd, defined in real_data
            const_precision_iterator<T>& get_lhs_itr();
            
            // fwd decl'd, defined in real_data
            const_precision_iterator<T>& get_rhs_itr();

            /**
             * @returns a pointer to the rhs operand
             */
            std::shared_ptr<real_data<T>> rhs() const {
                return _rhs;
            }

            /**
             *  @returns a pointer to the lhs operand
             */
            std::shared_ptr<real_data<T>> lhs() const {
                return _lhs;
            }
        };
    }
}

#endif // BOOST_REAL_REAL_OPERATION
