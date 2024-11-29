#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdbool.h>
#include <stdint.h>

#define VARIABLES_COUNT (('z' - 'a' + 1) + ('Z' - 'A' + 1))

/**
 * @brief an evaluation environment.
 *
 * This data structure represents the environment in which an expression is evaluated.
 */
struct environment {
	uint64_t variables; ///< The values of the variables packed into a 64-bit number
};

/**
 * @brief Creates a new environment.
 *
 * Initalizes a new environment with all its variables set to false.
 *
 * @return The newly created environment.
 *
 * @memberof environment
 */
struct environment environment_new(void);

/**
 * @brief Retrieves the value of a variable.
 *
 * Returns the value stored from the variable with the given name in the given environment.
 *
 * @param[in] environment Pointer to the environment.
 * @param[in] name The variable's name, must be an alphabet letter.
 * @return The value of the variable.
 *
 * @memberof environment
 */
bool environment_get_variable(const struct environment *environment, char name);

/**
 * @brief Assigns a value to a variable.
 *
 * Stores the given value to the variable with the given name in the given environment.
 *
 * @param[in,out] environment Pointer to the environment.
 * @param[in] name The variable's name, must be an alphabet letter.
 * @param[in] value The value to be stored.
 *
 * @memberof environment
 */
void environment_set_variable(struct environment *environment, char name, bool value);

#endif
