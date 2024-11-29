#include <environment.h>

#include <assert.h>
#include <ctype.h>
#include <stddef.h>

#define VARIABLE_INDEX(name)                                                                       \
	(islower(name) ? (size_t)((name) - 'a') : (size_t)((name) - 'A' + ('z' - 'a' + 1)))

struct environment environment_new(void) {
	return (struct environment){
		.variables = 0,
	};
}

bool environment_get_variable(const struct environment *environment, char name) {
	assert(environment != NULL && isalpha((unsigned char)name));

	return (environment->variables >> VARIABLE_INDEX(name)) & 1U;
}

void environment_set_variable(struct environment *environment, char name, bool value) {
	assert(environment != NULL && isalpha((unsigned char)name));

	environment->variables &= ~(1U << VARIABLE_INDEX(name));
	environment->variables |= ((unsigned)value << VARIABLE_INDEX(name));
}
