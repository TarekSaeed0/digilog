#include <expression.h>

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct expression expression_operation(enum operation_type type, ...) {
	va_list arguments;
	va_start(arguments, type);

	size_t arity = operation_type_arity(type);

	struct expression *operands = malloc(arity * sizeof(*operands));
	for (size_t i = 0; i < arity; i++) {
		operands[i] = va_arg(arguments, struct expression);
	}

	va_end(arguments);

	return (struct expression){
		.type = expression_type_operation,
		.operation = { .type = type, .operands = operands },
	};
}

struct expression expression_clone(const struct expression *expression) {
	assert(expression != NULL);

	switch (expression->type) {
		case expression_type_constant:
		case expression_type_variable: return *expression;
		case expression_type_operation: {
			size_t arity = operation_type_arity(expression->operation.type);
			struct expression *operands = malloc(arity * sizeof(*operands));
			for (size_t i = 0; i < arity; i++) {
				operands[i] = expression_clone(&expression->operation.operands[i]);
			}
			return (struct expression){
				.type = expression_type_operation,
				.operation = { .type = expression->operation.type, .operands = operands },
			};
		} break;
		default: assert(false);
	}

	return (struct expression){ 0 };
}

void expression_drop(struct expression *expression) {
	assert(expression != NULL);

	switch (expression->type) {
		case expression_type_constant:
		case expression_type_variable: break;
		case expression_type_operation: {
			size_t arity = operation_type_arity(expression->operation.type);
			for (size_t i = 0; i < arity; i++) {
				expression_drop(&expression->operation.operands[i]);
			}
			free(expression->operation.operands);
		} break;
		default: assert(false);
	}
}

bool expression_equals(
	const struct expression *expression_1,
	const struct expression *expression_2
) {
	assert(expression_1 != NULL && expression_2 != NULL);

	if (expression_1->type != expression_2->type) {
		return false;
	}

	switch (expression_1->type) {
		case expression_type_constant:
			return expression_1->constant.value == expression_2->constant.value;
		case expression_type_variable:
			return expression_1->variable.name == expression_2->variable.name;
		case expression_type_operation: {
			if (expression_1->operation.type != expression_2->operation.type) {
				return false;
			}

			size_t arity = operation_type_arity(expression_1->operation.type);
			for (size_t i = 0; i < arity; i++) {
				if (!expression_equals(
						&expression_1->operation.operands[i],
						&expression_2->operation.operands[i]
					)) {
					return false;
				}
			}

			return true;
		}
		default: assert(false);
	}
}

static struct expression expression_from_string_expression(const char **string);
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static struct expression expression_from_string_atom(const char **string) {
	assert(string != NULL && *string != NULL);

	struct expression atom;

	while (isspace((unsigned char)**string)) {
		++*string;
	}

	if (**string == '(') {
		++*string;

		atom = expression_from_string_expression(string);

		while (isspace((unsigned char)**string)) {
			++*string;
		}

		if (**string == ')') {
			++*string;
		} else {
			(void)fprintf(stderr, "Warning: unclosed parentheses \"%s\"\n", *string);
		}
	} else if (isalpha((unsigned char)**string)) {
		char name = **string;

		++*string;

		atom = expression_variable(name);
	} else {
		char *end = NULL;
		long value = strtol(*string, &end, 10);
		if (end == *string) {
			(void)fprintf(stderr, "Error: failed to parse constant from \"%s\"\n", *string);
			value = false;
		}

		if (errno == ERANGE) {
			(void
			)fprintf(stderr, "Warning: constant parsed from \"%s\" is out of range\n", *string);
		}

		if (value != 0 && value != 1) {
			(void)fprintf(
				stderr,
				"Warning: non-zero constant parsed from \"%s\" will be implicitly converted into a "
				"1\n",
				*string
			);
		}

		*string = end;

		atom = expression_constant(value);
	}

	return atom;
}
static struct expression expression_from_string_primary(const char **string) {
	assert(string != NULL && *string != NULL);

	struct expression primary;

	while (isspace((unsigned char)**string)) {
		++*string;
	}

	if (**string == '!') {
		++*string;

		primary =
			expression_operation(operation_type_negation, expression_from_string_primary(string));
	} else {
		primary = expression_from_string_atom(string);
	}

	while (isspace((unsigned char)**string)) {
		++*string;
	}

	while (1) {
		while (isspace((unsigned char)**string)) {
			++*string;
		}

		if (**string == '\'') {
			++*string;
			primary = expression_operation(operation_type_negation, primary);
		} else {
			break;
		}
	}

	return primary;
}
static struct expression expression_from_string_factor(const char **string) {
	assert(string != NULL && *string != NULL);

	struct expression factor = expression_from_string_primary(string);

	while (1) {
		while (isspace((unsigned char)**string)) {
			++*string;
		}

		if (**string == '!' || **string == '(' || isalpha((unsigned char)**string)) {
			factor = expression_operation(
				operation_type_conjunction,
				factor,
				expression_from_string_primary(string)
			);
		} else {
			break;
		}
	}

	return factor;
}
static struct expression expression_from_string_term(const char **string) {
	assert(string != NULL && *string != NULL);

	struct expression expression = expression_from_string_factor(string);

	while (1) {
		while (isspace((unsigned char)**string)) {
			++*string;
		}

		switch (**string) {
			case '&':
			case '*': {
				++*string;

				expression = expression_operation(
					operation_type_conjunction,
					expression,
					expression_from_string_factor(string)
				);
			} break;
			default: return expression;
		}
	}
}
static struct expression expression_from_string_expression(const char **string) {
	assert(string != NULL && *string != NULL);

	struct expression expression = expression_from_string_term(string);

	while (1) {
		while (isspace((unsigned char)**string)) {
			++*string;
		}

		switch (**string) {
			case '|':
			case '+': {
				++*string;

				expression = expression_operation(
					operation_type_disjunction,
					expression,
					expression_from_string_term(string)
				);
			} break;
			default: return expression;
		}
	}
}

struct expression expression_from_string(const char *string) {
	assert(string != NULL);

	struct expression expression = expression_from_string_expression(&string);

	while (isspace((unsigned char)*string)) {
		++string;
	}

	if (*string != '\0') {
		(void)fprintf(stderr, "Warning: trailing characters \"%s\" after expression\n", string);
	}

	return expression;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static int expression_to_string_(
	char *string,
	size_t maximum_length,
	const struct expression *expression
) {
	assert(expression != NULL);

	int total_length = 0;
#define print(function, ...)                                                                       \
	do {                                                                                           \
		int length = function(string, maximum_length, __VA_ARGS__);                                \
		if (length < 0) {                                                                          \
			return length;                                                                         \
		}                                                                                          \
		if ((size_t)length >= maximum_length) {                                                    \
			if (string != NULL) {                                                                  \
				string += maximum_length;                                                          \
			}                                                                                      \
			maximum_length = 0;                                                                    \
		} else {                                                                                   \
			if (string != NULL) {                                                                  \
				string += length;                                                                  \
			}                                                                                      \
			maximum_length -= (size_t)length;                                                      \
		}                                                                                          \
		total_length += length;                                                                    \
	} while (0)

	switch (expression->type) {
		case expression_type_constant: print(snprintf, "%d", expression->constant.value); break;
		case expression_type_variable: print(snprintf, "%c", expression->variable.name); break;
		case expression_type_operation: {
			switch (expression->operation.type) {
				case operation_type_conjunction:
				case operation_type_disjunction: {
					if (expression->operation.operands[0].type == expression_type_operation &&
						(operation_type_precedence(expression->operation.operands[0].operation.type
						 ) < operation_type_precedence(expression->operation.type))) {
						print(snprintf, "(");
						print(expression_to_string_, &expression->operation.operands[0]);
						print(snprintf, ")");
					} else {
						print(expression_to_string_, &expression->operation.operands[0]);
					}

					switch (expression->operation.type) {
						case operation_type_conjunction: {
							if (expression->operation.operands[0].type ==
									expression_type_constant ||
								expression->operation.operands[1].type ==
									expression_type_constant) {
								print(snprintf, " * ");
							}
						} break;
						case operation_type_disjunction: print(snprintf, " + "); break;
						// we have already checked the operation's type before
						default: __builtin_unreachable();
					}

					if (expression->operation.operands[1].type == expression_type_operation &&
						(operation_type_precedence(expression->operation.operands[1].operation.type
						 ) <= operation_type_precedence(expression->operation.type))) {
						print(snprintf, "(");
						print(expression_to_string_, &expression->operation.operands[1]);
						print(snprintf, ")");
					} else {
						print(expression_to_string_, &expression->operation.operands[1]);
					}
				} break;
				case operation_type_negation: {
					if (expression->operation.operands[0].type == expression_type_operation &&
						(operation_type_precedence(expression->operation.operands[0].operation.type
						 ) < operation_type_precedence(expression->operation.type))) {
						print(snprintf, "(");
						print(expression_to_string_, &expression->operation.operands[0]);
						print(snprintf, ")");
					} else {
						print(expression_to_string_, &expression->operation.operands[0]);
					}
					print(snprintf, "'");

				} break;
				default: assert(false);
			}
		} break;
		default: assert(false);
	}

	return total_length;
#undef print
}
char *expression_to_string(const struct expression *expression) {
	assert(expression != NULL);

	int length = expression_to_string_(NULL, 0, expression);
	if (length < 0) {
		return NULL;
	}

	char *string = malloc((size_t)length + 1);
	if (string == NULL) {
		return NULL;
	}

	if (expression_to_string_(string, (size_t)length + 1, expression) < 0) {
		free(string);
		return NULL;
	}

	return string;
}

void variables_drop(struct variables *variables) {
	assert(variables != NULL);

	free(variables->data);
}

void expression_variables_(const struct expression *expression, struct environment *environment) {
	assert(expression != NULL && environment != NULL);

	switch (expression->type) {
		case expression_type_constant: break;
		case expression_type_variable: {
			environment_set_variable(environment, expression->variable.name, true);
		} break;
		case expression_type_operation: {
			size_t arity = operation_type_arity(expression->operation.type);
			for (size_t i = 0; i < arity; i++) {
				expression_variables_(&expression->operation.operands[i], environment);
			}
		} break;
		default: assert(false);
	}
}
struct variables variables_from_expression(const struct expression *expression) {
	assert(expression != NULL);

	struct environment environment = environment_new();
	expression_variables_(expression, &environment);

	struct variables variables = {
		.data = malloc(VARIABLES_COUNT),
	};
	for (size_t i = 0; i < VARIABLES_COUNT; i++) {
		if ((environment.variables >> i) & 1U) {
			variables.data[variables.length++] =
				(char)(i < ('z' - 'a' + 1) ? 'a' + i : 'A' + (i - ('z' - 'a' + 1)));
		}
	}

	return variables;
}

void minterms_drop(struct minterms *minterms) {
	assert(minterms != NULL);

	variables_drop(&minterms->variables);
	free(minterms->data);
}

struct minterms minterms_from_expression(const struct expression *expression) {
	assert(expression != NULL);

	struct minterms minterms = {
		.variables = variables_from_expression(expression),
	};

	struct expression simplified_expression = expression_clone(expression);
	expression_simplify(&simplified_expression, NULL);

	minterms.data = malloc((1U << minterms.variables.length) * sizeof(*minterms.data));
	minterms.length = 0;
	for (uint64_t i = 0; i < (1U << minterms.variables.length); i++) {
		struct environment environment = environment_new();
		for (size_t j = 0; j < minterms.variables.length; j++) {
			environment_set_variable(
				&environment,
				minterms.variables.data[minterms.variables.length - j - 1],
				(i >> j) & 1U
			);
		}

		if (expression_evaluate(&simplified_expression, &environment)) {
			minterms.data[minterms.length++] = i;
		}
	}

	expression_drop(&simplified_expression);

	return minterms;
}

void expression_simplify(struct expression *expression, const struct environment *environment) {
	assert(expression != NULL);

	switch (expression->type) {
		case expression_type_constant: break;
		case expression_type_variable: {
			if (environment != NULL) {
				*expression = expression_constant(
					environment_get_variable(environment, expression->variable.name)
				);
			}
		} break;
		case expression_type_operation: {
			size_t arity = operation_type_arity(expression->operation.type);
			for (size_t i = 0; i < arity; i++) {
				expression_simplify(&expression->operation.operands[i], environment);
			}

			switch (expression->operation.type) {
				case operation_type_conjunction: {
					if (expression->operation.operands[0].type == expression_type_constant) {
						struct expression *operands = expression->operation.operands;
						if (expression->operation.operands[0].constant.value) {
							*expression = expression->operation.operands[1];
						} else {
							expression_drop(&expression->operation.operands[1]);
							*expression = expression->operation.operands[0];
						}
						free(operands);
					} else if (expression->operation.operands[1].type == expression_type_constant) {
						struct expression *operands = expression->operation.operands;
						if (expression->operation.operands[1].constant.value) {
							*expression = expression->operation.operands[0];
						} else {
							expression_drop(&expression->operation.operands[0]);
							*expression = expression->operation.operands[1];
						}
						free(operands);
					}
				} break;
				case operation_type_disjunction: {
					if (expression->operation.operands[0].type == expression_type_constant) {
						struct expression *operands = expression->operation.operands;
						if (expression->operation.operands[0].constant.value) {
							expression_drop(&expression->operation.operands[1]);
							*expression = expression->operation.operands[0];
						} else {
							*expression = expression->operation.operands[1];
						}
						free(operands);
					} else if (expression->operation.operands[1].type == expression_type_constant) {
						struct expression *operands = expression->operation.operands;
						if (expression->operation.operands[1].constant.value) {
							expression_drop(&expression->operation.operands[0]);
							*expression = expression->operation.operands[1];
						} else {
							*expression = expression->operation.operands[0];
						}
						free(operands);
					}
				} break;
				case operation_type_negation: {
					if (expression->operation.operands[0].type == expression_type_constant) {
						struct expression *operands = expression->operation.operands;
						*expression =
							expression_constant(!expression->operation.operands[0].constant.value);
						free(operands);
					}
				} break;
				default: assert(false);
			}
		} break;
		default: assert(false);
	}
}

void expression_print(const struct expression *expression) {
	assert(expression != 0);

	char *string = expression_to_string(expression);

	printf("%s", string);

	free(string);
}

void expression_debug_print(const struct expression *expression) {
	assert(expression != 0);

	switch (expression->type) {
		case expression_type_constant: printf("constant(%d)", expression->constant.value); break;
		case expression_type_variable: printf("variable(%c)", expression->variable.name); break;
		case expression_type_operation: {
			printf("operation(");
			switch (expression->operation.type) {
				case operation_type_conjunction: printf("conjunction("); break;
				case operation_type_disjunction: printf("disjunction("); break;
				case operation_type_negation: printf("negation("); break;
				default: assert(false);
			}
			size_t arity = operation_type_arity(expression->operation.type);
			for (size_t i = 0; i < arity; i++) {
				if (i != 0) {
					printf(", ");
				}
				expression_debug_print(&expression->operation.operands[i]);
			}
			printf("))");
		} break;
		default: assert(false);
	}
}

bool expression_evaluate(
	const struct expression *expression,
	const struct environment *environment
) {
	assert(expression != NULL);

	switch (expression->type) {
		case expression_type_constant: return expression->constant.value;
		case expression_type_variable: {
			if (environment == NULL) {
				return false;
			}

			return environment_get_variable(environment, expression->variable.name);
		} break;
		case expression_type_operation: {
			switch (expression->operation.type) {
				case operation_type_conjunction: {
					bool value =
						expression_evaluate(&expression->operation.operands[0], environment);
					if (!value) {
						return false;
					}
					return expression_evaluate(&expression->operation.operands[1], environment);
				}
				case operation_type_disjunction: {
					bool value =
						expression_evaluate(&expression->operation.operands[0], environment);
					if (value) {
						return true;
					}
					return expression_evaluate(&expression->operation.operands[1], environment);
				}
				case operation_type_negation: {
					return !expression_evaluate(&expression->operation.operands[0], environment);
				}
				default: assert(false);
			}
		} break;
		default: assert(false);
	}
}

bool implicant_combinable(struct implicant implicant_1, struct implicant implicant_2) {
	// two implicants can possibly be combined if their masks are equal
	if (implicant_1.mask != implicant_2.mask) {
		return false;
	}

	// two implicants with equal masks can be combined if they only differ in one bit
	uint64_t difference = (implicant_1.value ^ implicant_2.value) & implicant_1.mask;
	int ones_count = __builtin_popcountll(difference);

	return ones_count == 1;
}

struct implicant implicant_combine(struct implicant implicant_1, struct implicant implicant_2) {
	assert(implicant_combinable(implicant_1, implicant_2));

	return (struct implicant){
		.value = implicant_1.value,
		.mask = implicant_1.mask & ~(implicant_1.value ^ implicant_2.value),
	};
}

struct implicants implicants_new(void) {
	return (struct implicants){
		.data = NULL,
		.length = 0,
		.capacity = 0,
	};
}

void implicants_drop(struct implicants *implicants) {
	assert(implicants != NULL);

	free(implicants->data);
}

void implicants_add(struct implicants *implicants, struct implicant implicant) {
	assert(implicants != NULL);

	if (implicants->length == implicants->capacity) {
		if (implicants->capacity == 0) {
			implicants->capacity = 1;
		} else {
			assert(implicants->capacity < SIZE_MAX / 2);
			implicants->capacity *= 2;
		}
		implicants->data =
			realloc(implicants->data, implicants->capacity * sizeof(*implicants->data));
		assert(implicants->data != NULL);
	}

	implicants->data[implicants->length++] = implicant;
}

struct expression expression_from_implicant(
	struct implicant implicant,
	const struct variables *variables
) {
	assert(variables != NULL);

	size_t i = 0;
	while (i < variables->length && ((implicant.mask >> (variables->length - i - 1)) & 1U) == 0) {
		i++;
	}

	if (i >= variables->length) {
		return expression_constant(true);
	}

	struct expression expression = expression_variable(variables->data[i]);
	if (((implicant.value >> (variables->length - i - 1)) & 1U) == 0) {
		expression = expression_operation(operation_type_negation, expression);
	}
	for (i++; i < variables->length; i++) {
		if (((implicant.mask >> (variables->length - i - 1)) & 1U) == 0) {
			continue;
		}

		struct expression expression_ = expression_variable(variables->data[i]);
		if (((implicant.value >> (variables->length - i - 1)) & 1U) == 0) {
			expression_ = expression_operation(operation_type_negation, expression_);
		}

		expression = expression_operation(operation_type_conjunction, expression, expression_);
	}

	return expression;
}
struct expression implicants_to_expression(
	const struct implicants *implicants,
	const struct variables *variables
) {
	assert(implicants != NULL && variables != NULL);

	if (implicants->length == 0) {
		return expression_constant(false);
	}

	struct expression expression = expression_from_implicant(implicants->data[0], variables);
	for (size_t i = 1; i < implicants->length; i++) {
		expression = expression_operation(
			operation_type_disjunction,
			expression,
			expression_from_implicant(implicants->data[i], variables)
		);
	}

	return expression;
}

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

struct table {
	struct group {
		struct term {
			struct implicant implicant;
			bool combined;
		} *terms;
		size_t terms_count;
		size_t terms_capacity;
	} *groups;
	size_t groups_count;
};
struct table table_new(size_t groups_count) {
	struct table table = {
		.groups = malloc(groups_count * sizeof(*table.groups)),
		.groups_count = groups_count,
	};

	for (size_t j = 0; j < groups_count; j++) {
		table.groups[j] = (struct group){
			.terms = NULL,
			.terms_count = 0,
			.terms_capacity = 0,
		};
	}

	return table;
}
void table_drop(struct table *table) {
	assert(table != NULL);

	for (size_t i = 0; i < table->groups_count; i++) {
		free(table->groups[i].terms);
	}
	free(table->groups);
}
void table_add_implicant(struct table *table, struct implicant implicant) {
	assert(table != NULL);

	int ones_count = __builtin_popcountll((implicant.value & implicant.mask));
	assert(0 <= ones_count && (size_t)ones_count < table->groups_count);

	// if implicant is already in group, then don't add it
	bool is_duplicate = false;
	for (size_t i = 0; i < table->groups[ones_count].terms_count; i++) {
		if (table->groups[ones_count].terms[i].implicant.mask == implicant.mask &&
			table->groups[ones_count].terms[i].implicant.value == implicant.value) {
			is_duplicate = true;
			break;
		}
	}
	if (!is_duplicate) {
		// if there isn't enough space for a new term, reallocate.
		if (table->groups[ones_count].terms_count == table->groups[ones_count].terms_capacity) {
			if (table->groups[ones_count].terms_capacity == 0) {
				table->groups[ones_count].terms_capacity = 1;
			} else {
				assert(table->groups[ones_count].terms_capacity < SIZE_MAX / 2);
				table->groups[ones_count].terms_capacity *= 2;
			}
			table->groups[ones_count].terms = realloc(
				table->groups[ones_count].terms,
				table->groups[ones_count].terms_capacity * sizeof(*table->groups[ones_count].terms)
			);
			assert(table->groups[ones_count].terms != NULL);
		}
		table->groups[ones_count].terms[table->groups[ones_count].terms_count++] = (struct term){
			.implicant = implicant,
			.combined = false,
		};
	}
}

struct implicants minterms_to_prime_implicants(const struct minterms *minterms) {
	assert(minterms != NULL);

	struct table input_table = table_new(minterms->variables.length + 1);
	struct table output_table = table_new(minterms->variables.length + 1);

	for (size_t i = 0; i < minterms->length; i++) {
		table_add_implicant(
			&input_table,
			(struct implicant){
				.value = minterms->data[i],
				.mask = (UINT64_C(1) << minterms->variables.length) - 1U,
			}
		);
	}

	struct implicants prime_implicants = implicants_new();

	bool minimized = true;
	do {
		minimized = true;

		for (size_t i = 0; i < input_table.groups_count; i++) {
			for (size_t j = 0; j < input_table.groups[i].terms_count; j++) {
				if (i != input_table.groups_count - 1) {
					for (size_t k = 0; k < input_table.groups[i + 1].terms_count; k++) {
						// two implicants can be combined if their masks are equal
						if (implicant_combinable(
								input_table.groups[i].terms[j].implicant,
								input_table.groups[i + 1].terms[k].implicant
							)) {

							input_table.groups[i].terms[j].combined = true;
							input_table.groups[i + 1].terms[k].combined = true;

							minimized = false;

							table_add_implicant(
								&output_table,
								implicant_combine(
									input_table.groups[i].terms[j].implicant,
									input_table.groups[i + 1].terms[k].implicant
								)
							);
						}
					}
				}

				if (!input_table.groups[i].terms[j].combined) {
					implicants_add(&prime_implicants, input_table.groups[i].terms[j].implicant);
				}
			}
		}

		struct table table = input_table;
		input_table = output_table;
		output_table = table;
		for (size_t i = 0; i < output_table.groups_count; i++) {
			output_table.groups[i].terms_count = 0;
		}
	} while (!minimized);

	table_drop(&input_table);
	table_drop(&output_table);

	return prime_implicants;
}

void implicants_minimalize(struct implicants *implicants, const struct minterms *minterms) {
	assert(implicants != NULL && minterms != NULL);

	struct {
		size_t *data;
		size_t length;
	} *factors = malloc(minterms->length * sizeof(*factors));
	assert(factors != NULL);

	size_t *frequencies = malloc(implicants->length * sizeof(*frequencies));
	memset(frequencies, 0, implicants->length * sizeof(*frequencies));

	for (size_t i = 0; i < minterms->length; i++) {
		factors[i].data = malloc(implicants->length * sizeof(*factors[i].data));
		assert(factors[i].data != NULL);
		factors[i].length = 0;

		for (size_t j = 0; j < implicants->length; j++) {
			if (((implicants->data[j].value ^ minterms->data[i]) & implicants->data[j].mask) == 0) {
				factors[i].data[factors[i].length++] = j;
				frequencies[j]++;
			}
		}
	}

	bool *minimal = malloc(implicants->length * sizeof(*minimal));
	memset(minimal, false, implicants->length * sizeof(*minimal));
	for (size_t i = 0; i < minterms->length; i++) {
		bool absorbed = false;
		for (size_t j = 0; j < factors[i].length; j++) {
			if (minimal[factors[i].data[j]]) {
				absorbed = true;
				break;
			}
		}
		if (!absorbed) {
			size_t most_frequent = factors[i].data[0];
			for (size_t j = 1; j < factors[i].length; j++) {
				if (frequencies[factors[i].data[j]] > frequencies[most_frequent]) {
					most_frequent = factors[i].data[j];
				}
			}
			minimal[most_frequent] = true;
		}

		for (size_t j = 0; j < factors[i].length; j++) {
			frequencies[factors[i].data[j]]--;
		}
	}

	free(frequencies);

	for (size_t i = 0; i < minterms->length; i++) {
		free(factors[i].data);
	}
	free(factors);

	for (ptrdiff_t i = (ptrdiff_t)implicants->length - 1; i >= 0; i--) {
		if (!minimal[i]) {
			memmove(
				&implicants->data[i],
				&implicants->data[i + 1],
				(implicants->length - (size_t)i - 1) * sizeof(*implicants->data)
			);
			implicants->length--;
		}
	}

	free(minimal);
}
