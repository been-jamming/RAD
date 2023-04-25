#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include "rad.h"

static int rad_order_of_operations[] = {-1, -1, 0, 0, 1, 1};

static rad_func *rad_parse_internal(const char **c, unsigned int *num_args);

static void skip_whitespace(const char **c){
	while(**c == ' ' || **c == '\t' || **c == '\n'){
		++*c;
	}
}

static rad_func *rad_parse_value(const char **c, unsigned int *num_args){
	char *end;
	double const_value;
	long index;
	rad_func *output;

	skip_whitespace(c);
	if(**c >= '0' && **c <= '9'){
		const_value = strtod(*c, &end);
		*c = end;
		return rad_const(const_value);
	} else if(**c == '{'){
		++*c;
		skip_whitespace(c);
		if(**c < '0' || **c > '9'){
			return NULL;
		}
		index = strtol(*c, &end, 10);
		if(index < 0 || index > UINT_MAX - 1){
			return NULL;
		}
		*c = end;
		skip_whitespace(c);
		if(**c != '}'){
			return NULL;
		}
		++*c;

		if(index >= *num_args){
			*num_args = index + 1;
		}

		output = rad_create_func(ARG, 1);
		output->arg_id = index;
		return output;
	} else if(**c == '['){
		++*c;
		skip_whitespace(c);
		if(**c < '0' || **c > '9'){
			return NULL;
		}
		index = strtol(*c, &end, 10);
		if(index < 0 || index > UINT_MAX - 1){
			return NULL;
		}
		*c = end;
		skip_whitespace(c);
		if(**c != ']'){
			return NULL;
		}
		++*c;
		return rad_input(index);
	} else if(**c == '('){
		++*c;
		output = rad_parse_internal(c, num_args);
		skip_whitespace(c);
		if(**c != ')'){
			rad_discard(output);
			return NULL;
		}
		++*c;
		return output;
	} else {
		return NULL;
	}
}

static enum rad_oper rad_parse_operation(const char **c){
	if(**c == '+'){
		return ADD;
	} else if(**c == '-'){
		return SUBTRACT;
	} else if(**c == '*'){
		return MULTIPLY;
	} else if(**c == '/'){
		return DIVIDE;
	} else {
		return -1;
	}
}

//Recursively parses an expression into a rad_func, taking into account the order of operations
static rad_func *rad_parse_recursive(const char **c, rad_func *value0, int order, unsigned int *num_args){
	rad_func *value1;
	enum rad_oper operation;
	rad_func *operation_func;

	skip_whitespace(c);
	if(**c == '\0' || **c == ')'){
		return value0;
	}
	operation = rad_parse_operation(c);
	if(operation == -1){
		rad_discard(value0);
		return NULL;
	}
	while(rad_order_of_operations[operation] > order){
		++*c;
		skip_whitespace(c);
		value1 = rad_parse_value(c, num_args);
		if(value1 == NULL){
			rad_discard(value0);
			return NULL;
		}
		value1 = rad_parse_recursive(c, value1, rad_order_of_operations[operation], num_args);
		operation_func = rad_create_func(operation, 1);
		operation_func->operand0 = value0;
		operation_func->operand1 = value1;
		value0 = operation_func;
		skip_whitespace(c);
		if(**c == '\0' || **c == ')'){
			return value0;
		}
		operation = rad_parse_operation(c);
		if(operation == -1){
			rad_discard(value0);
			return NULL;
		}
	}

	return value0;
}

static rad_func *rad_parse_internal(const char **c, unsigned int *num_args){
	rad_func *output;
	rad_func *value;

	skip_whitespace(c);
	value = rad_parse_value(c, num_args);
	if(value == NULL){
		return NULL;
	}
	output = rad_parse_recursive(c, value, -1, num_args);

	return output;
}

static void rad_substitute_args(rad_func **func, rad_func **args){
	unsigned int arg_id;

	switch((*func)->operation){
		case ARG:
			arg_id = (*func)->arg_id;
			rad_discard(*func);
			*func = rad_copy(args[arg_id]);
			return;
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			rad_substitute_args(&((*func)->operand0), args);
			rad_substitute_args(&((*func)->operand1), args);
			return;
		default:
			return;
	}
}

rad_func *rad_parse(const char *c, ...){
	rad_func *output;
	rad_func **arg_list;
	unsigned int num_args = 0;
	unsigned int i;
	va_list args;

	va_start(args, c);

	output = rad_parse_internal(&c, &num_args);
	if(num_args == 0){
		va_end(args);
		return output;
	}

	arg_list = malloc(sizeof(rad_func *)*num_args);
	for(i = 0; i < num_args; i++){
		arg_list[i] = va_arg(args, rad_func *);
	}

	va_end(args);

	rad_substitute_args(&output, arg_list);

	for(i = 0; i < num_args; i++){
		rad_discard(arg_list[i]);
	}

	free(arg_list);

	return output;
}

void rad_print(rad_func *func){
	switch(func->operation){
		case CONSTANT:
			printf("%lf", func->const_value);
			return;
		case INPUT:
			printf("[%u]", func->input_id);
			return;
		case ADD:
			printf("(");
			rad_print(func->operand0);
			printf("+");
			rad_print(func->operand1);
			printf(")");
			return;
		case SUBTRACT:
			printf("(");
			rad_print(func->operand0);
			printf("-");
			rad_print(func->operand1);
			printf(")");
			return;
		case MULTIPLY:
			printf("(");
			rad_print(func->operand0);
			printf("*");
			rad_print(func->operand1);
			printf(")");
			return;
		case DIVIDE:
			printf("(");
			rad_print(func->operand0);
			printf("/");
			rad_print(func->operand1);
			printf(")");
			return;
		default:
			return;
	}
}

int main(int argc, char **argv){
	rad_func *f;
	rad_func *g;
	rad_func *h;
	double inputs[] = {2, 3};
	double derivatives[] = {0, 0};
	double deriv;
	double value;

	g = rad_parse("[0]*[0] + [0]*[1]");
	f = rad_parse("{0}/({0} + 1)", rad_input(0));

	h = rad_composition(f, 1, rad_copy(g));
	h = rad_add(h, g);

	value = rad_backward_diff(h, inputs, derivatives);

	printf("\nvalue: %lf\nderiv x: %lf\nderiv y: %lf\n", value, derivatives[0], derivatives[1]);
	rad_discard(h);

	return 0;
}

