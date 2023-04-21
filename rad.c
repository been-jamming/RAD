#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rad.h"

rad_func *rad_create_func(enum rad_oper operation, unsigned int num_references){
	rad_func *output;

	output = malloc(sizeof(rad_func));
	output->operation = operation;
	output->num_references = num_references;

	return output;
}

rad_func *rad_const(double const_value){
	rad_func *output;

	output = rad_create_func(CONSTANT, 1);
	output->const_value = const_value;
	return output;
}

rad_func *rad_input(unsigned int input_id){
	rad_func *output;

	output = rad_create_func(INPUT, 1);
	output->input_id = input_id;
	return output;
}

rad_func *rad_add(rad_func *operand0, rad_func *operand1){
	rad_func *output;

	output = rad_create_func(ADD, 1);
	output->operand0 = operand0;
	output->operand1 = operand1;
	return output;
}

rad_func *rad_subtract(rad_func *operand0, rad_func *operand1){
	rad_func *output;

	output = rad_create_func(SUBTRACT, 1);
	output->operand0 = operand0;
	output->operand1 = operand1;
	return output;
}

rad_func *rad_multiply(rad_func *operand0, rad_func *operand1){
	rad_func *output;

	output = rad_create_func(MULTIPLY, 1);
	output->operand0 = operand0;
	output->operand1 = operand1;
	return output;
}

rad_func *rad_divide(rad_func *operand0, rad_func *operand1){
	rad_func *output;

	output = rad_create_func(DIVIDE, 1);
	output->operand0 = operand0;
	output->operand1 = operand1;
	return output;
}

rad_func *rad_copy(rad_func *func){
	func->num_references++;
	return func;
}

void rad_free(rad_func *func){
	switch(func->operation){
		case CONSTANT:
		case INPUT:
			free(func);
			return;
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			rad_free(func->operand0);
			rad_free(func->operand1);
			free(func);
			return;
	}
}

void rad_discard(rad_func *func){
	switch(func->operation){
		case CONSTANT:
		case INPUT:
			func->num_references--;
			if(func->num_references == 0){
				rad_free(func);
			}
			return;
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			rad_discard(func->operand0);
			rad_discard(func->operand1);
			func->num_references--;
			if(func->num_references == 0){
				free(func);
			}
			return;
	}
}

double rad_eval(rad_func *func, double *inputs){
	double input0;
	double input1;

	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			input0 = rad_eval(func->operand0, inputs);
			input1 = rad_eval(func->operand1, inputs);
			break;
		default:
			break;
	}

	switch(func->operation){
		case CONSTANT:
			func->value = func->const_value;
			break;
		case INPUT:
			func->value = inputs[func->input_id];
			break;
		case ADD:
			func->value = input0 + input1;
			break;
		case SUBTRACT:
			func->value = input0 - input1;
			break;
		case MULTIPLY:
			func->value = input0*input1;
			break;
		case DIVIDE:
			func->value = input0/input1;
			break;
	}

	return func->value;
}

void rad_forward_diff(rad_func *func, double *inputs, unsigned int input_id, double *value, double *deriv){
	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			rad_forward_diff(func->operand0, inputs, input_id, NULL, NULL);
			rad_forward_diff(func->operand1, inputs, input_id, NULL, NULL);
			break;
		default:
			break;
	}

	switch(func->operation){
		case CONSTANT:
			func->value = func->const_value;
			func->deriv = 0;
			break;
		case INPUT:
			func->value = inputs[func->input_id];
			func->deriv = (input_id == func->input_id);
			break;
		case ADD:
			func->value = func->operand0->value + func->operand1->value;
			func->deriv = func->operand0->deriv + func->operand1->deriv;
			break;
		case SUBTRACT:
			func->value = func->operand0->value - func->operand1->value;
			func->deriv = func->operand0->deriv - func->operand1->deriv;
			break;
		case MULTIPLY:
			func->value = func->operand0->value*func->operand1->value;
			func->deriv = func->operand0->value*func->operand1->deriv + func->operand1->value*func->operand0->deriv;
			break;
		case DIVIDE:
			func->value = func->operand0->value/func->operand1->value;
			func->deriv = (func->operand0->deriv*func->operand1->value - func->operand1->deriv*func->operand0->value)/(func->operand1->value*func->operand1->value);
			break;
	}

	if(value){
		*value = func->value;
	}

	if(deriv){
		*deriv = func->deriv;
	}
}

static void rad_reset_deriv(rad_func *func){
	func->deriv = 0;
	func->traversed = false;
	func->sum_traversed = false;

	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			rad_reset_deriv(func->operand0);
			rad_reset_deriv(func->operand1);
			return;
		default:
			return;
	}
}

static void rad_backward_diff_recursive(rad_func *func){
	if(func->traversed){
		return;
	}

	func->traversed = true;

	switch(func->operation){
		case INPUT:
		case CONSTANT:
			return;
		case ADD:
			func->operand0->deriv += func->deriv;
			func->operand1->deriv += func->deriv;
			break;
		case SUBTRACT:
			func->operand0->deriv += func->deriv;
			func->operand1->deriv -= func->deriv;
			break;
		case MULTIPLY:
			func->operand0->deriv += func->deriv*func->operand1->value;
			func->operand1->deriv += func->deriv*func->operand0->value;
			break;
		case DIVIDE:
			func->operand0->deriv += func->deriv/func->operand1->value;
			func->operand1->deriv -= func->deriv*func->operand0->value/(func->operand1->value*func->operand1->value);
			break;
	}

	rad_backward_diff_recursive(func->operand0);
	rad_backward_diff_recursive(func->operand1);
}

static void rad_sum_input_deriv(rad_func *func, double *derivatives){
	if(func->sum_traversed){
		return;
	}

	func->sum_traversed = true;

	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			rad_sum_input_deriv(func->operand0, derivatives);
			rad_sum_input_deriv(func->operand1, derivatives);
			return;
		case INPUT:
			derivatives[func->input_id] += func->deriv;
			return;
		default:
			return;
	}
}

void rad_backward_diff(rad_func *func, double *derivatives){
	rad_reset_deriv(func);

	func->deriv = 1;
	rad_backward_diff_recursive(func);
	rad_sum_input_deriv(func, derivatives);
}

int main(int argc, char **argv){
	rad_func *x = rad_input(0);
	rad_func *y = rad_input(1);

	rad_func *f = rad_divide(rad_copy(x), rad_add(rad_multiply(rad_copy(x), x), rad_multiply(rad_copy(y), y)));

	double value;
	double deriv[2] = {0, 0};
	double inputs[2] = {3, 4};

	//rad_forward_diff(f, inputs, 0, &value, deriv);
	value = rad_eval(f, inputs);
	rad_backward_diff(f, deriv);

	printf("value: %lf\nderivative: %lf\n", value, deriv[0]);
	rad_discard(f);

	return 0;
}

