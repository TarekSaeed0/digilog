#include <expression.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define MAXIMUM_INPUT_LENGTH (1023)

int main(void) {
	char input[MAXIMUM_INPUT_LENGTH + 1];
	assert(fgets(input, sizeof(input), stdin) != NULL);
	input[strcspn(input, "\n")] = '\0';

	struct expression expression = expression_from_string(input);
	expression_print(&expression);
	printf("\n");

	struct minterms minterms = minterms_from_expression(&expression);
	expression_drop(&expression);

	printf("f(");
	for (size_t i = 0; i < minterms.variables.length; i++) {
		if (i != 0) {
			printf(", ");
		}
		printf("%c", minterms.variables.data[i]);
	}
	printf(")");

	printf(" = Σm(");
	for (size_t i = 0; i < minterms.length; i++) {
		if (i != 0) {
			printf(", ");
		}
		printf("%" PRIu64, minterms.data[i]);
	}
	printf(") = ");

	struct implicants prime_implicants = minterms_to_prime_implicants(&minterms);
	implicants_minimalize(&prime_implicants, &minterms);

	struct expression minimal_expression =
		implicants_to_expression(&prime_implicants, &minterms.variables);
	minterms_drop(&minterms);
	implicants_drop(&prime_implicants);

	expression_print(&minimal_expression);
	printf("\n");

	expression_drop(&minimal_expression);
}
