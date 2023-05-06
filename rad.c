#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include "rad.h"

static unsigned int global_invocation = 0;

rad_func *rad_create_func(enum rad_oper operation, unsigned int num_references){
	rad_func *output;

	output = malloc(sizeof(rad_func));
	output->operation = operation;
	output->num_references = num_references;
	output->invocation_id = global_invocation - 1;

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

rad_func *rad_composition(rad_func *func, unsigned int num_args, ...){
	va_list args;
	rad_func *output;
	unsigned int i;

	output = rad_create_func(COMPOSITION, 1);
	output->func = func;
	output->num_inputs = num_args;
	output->inputs = malloc(sizeof(rad_func *)*num_args);
	va_start(args, num_args);
	for(i = 0; i < num_args; i++){
		output->inputs[i] = va_arg(args, rad_func *);
	}
	va_end(args);

	output->input_values = malloc(sizeof(double)*num_args);
	output->input_derivatives = malloc(sizeof(double)*num_args);

	return output;
}

rad_func *rad_custom(double (*custom_eval)(double *, double *), unsigned int num_args, ...){
	va_list args;
	rad_func *output;
	unsigned int i;

	output = rad_create_func(CUSTOM, 1);
	output->custom_eval = custom_eval;
	output->num_inputs = num_args;
	output->inputs = malloc(sizeof(rad_func *)*num_args);
	va_start(args, num_args);
	for(i = 0; i < num_args; i++){
		output->inputs[i] = va_arg(args, rad_func *);
	}
	va_end(args);

	output->input_values = malloc(sizeof(double)*num_args);
	output->input_derivatives = malloc(sizeof(double)*num_args);
	output->input_grad = malloc(sizeof(double)*num_args);

	return output;
}

rad_func *rad_copy(/*not consumed*/rad_func *func){
	func->num_references++;
	return func;
}

void rad_discard(rad_func *func){
	unsigned int i;

	switch(func->operation){
		case CONSTANT:
		case INPUT:
		case ARG:
			func->num_references--;
			if(func->num_references == 0){
				free(func);
			}
			return;
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			func->num_references--;
			if(func->num_references == 0){
				rad_discard(func->operand0);
				rad_discard(func->operand1);
				free(func);
			}
			return;
		case COMPOSITION:
		case CUSTOM:
			func->num_references--;
			if(func->num_references == 0){
				if(func->operation == COMPOSITION){
					rad_discard(func->func);
				}
				for(i = 0; i < func->num_inputs; i++){
					rad_discard(func->inputs[i]);
				}
				free(func->inputs);
				free(func->input_values);
				free(func->input_derivatives);
				if(func->operation == CUSTOM){
					free(func->input_grad);
				}
				free(func);
			}
			return;
	}
}

rad_func *rad_deep_copy(/*not consumed*/rad_func *func){
	unsigned int i;
	rad_func *output;

	output = rad_create_func(func->operation, 1);

	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			output->operand0 = rad_deep_copy(func->operand0);
			output->operand1 = rad_deep_copy(func->operand1);
			return output;
		case CONSTANT:
			output->const_value = func->const_value;
			return output;
		case INPUT:
			output->input_id = func->input_id;
			return output;
		case ARG:
			output->arg_id = func->arg_id;
			return output;
		case COMPOSITION:
		case CUSTOM:
			if(func->operation == COMPOSITION){
				output->func = func->func;
			} else if(func->operation == CUSTOM){
				output->custom_eval = func->custom_eval;
			}
			output->num_inputs = func->num_inputs;
			output->inputs = malloc(sizeof(rad_func *)*func->num_inputs);
			for(i = 0; i < func->num_inputs; i++){
				output->inputs[i] = rad_deep_copy(func->inputs[i]);
			}
			output->input_values = malloc(sizeof(double)*func->num_inputs);
			output->input_derivatives = malloc(sizeof(double)*func->num_inputs);
			return output;
		default:
			return NULL;
	}
}

double rad_eval(rad_func *func, double *inputs){
	double input0;
	double input1;
	double output;
	int i;

	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			input0 = rad_eval(func->operand0, inputs);
			input1 = rad_eval(func->operand1, inputs);
			break;
		case COMPOSITION:
		case CUSTOM:
			for(i = 0; i < func->num_inputs; i++){
				func->input_values[i] = rad_eval(func->inputs[i], inputs);
			}
			break;
		default:
			break;
	}

	switch(func->operation){
		case CONSTANT:
			output = func->const_value;
			break;
		case INPUT:
			output = inputs[func->input_id];
			break;
		case ADD:
			output = input0 + input1;
			break;
		case SUBTRACT:
			output = input0 - input1;
			break;
		case MULTIPLY:
			output = input0*input1;
			break;
		case DIVIDE:
			output = input0/input1;
			break;
		case COMPOSITION:
			output = rad_eval(func->func, func->input_values);
			break;
		case CUSTOM:
			output = func->custom_eval(func->input_values, func->input_derivatives);
			break;
		default:
			break;
	}

	func->value = output;

	return output;
}

static double rad_backward_diff_eval(rad_func *func, double *inputs, unsigned int invocation_id){
	double input0;
	double input1;
	rad_func *new_func;
	int i;

	func->invocation_id = invocation_id;

	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			input0 = rad_backward_diff_eval(func->operand0, inputs, invocation_id);
			input1 = rad_backward_diff_eval(func->operand1, inputs, invocation_id);
			break;
		case COMPOSITION:
		case CUSTOM:
			for(i = 0; i < func->num_inputs; i++){
				func->input_values[i] = rad_backward_diff_eval(func->inputs[i], inputs, invocation_id);
				func->input_derivatives[i] = 0;
			}
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
		case COMPOSITION:
			//This actually doesn't work...
			/*if(func->func->invocation_id == invocation_id){
				new_func = rad_deep_copy(func->func);
				rad_discard(func->func);
				func->func = new_func;
			}*/
			func->value = rad_backward_diff(func->func, func->input_values, func->input_derivatives);
			break;
		case CUSTOM:
			func->value = func->custom_eval(func->input_values, func->input_derivatives);
			break;
		default:
			break;
	}

	return func->value;
}

double rad_forward_grad(rad_func *func, double *inputs, double *derivatives, double *value){
	unsigned int i;
	double value0;
	double deriv0;
	double value1;
	double deriv1;
	double out_value = 0;
	double out_deriv = 0;

	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			deriv0 = rad_forward_grad(func->operand0, inputs, derivatives, &value0);
			deriv1 = rad_forward_grad(func->operand1, inputs, derivatives, &value1);
			break;
		case COMPOSITION:
		case CUSTOM:
			for(i = 0; i < func->num_inputs; i++){
				func->input_derivatives[i] = rad_forward_grad(func->inputs[i], inputs, derivatives, func->input_values + i);
			}
			break;
		default:
			break;
	}

	switch(func->operation){
		case CONSTANT:
			out_value = func->const_value;
			out_deriv = 0;
			break;
		case INPUT:
			out_value = inputs[func->input_id];
			out_deriv = derivatives[func->input_id];
			break;
		case ADD:
			out_value = value0 + value1;
			out_deriv = deriv0 + deriv1;
			break;
		case SUBTRACT:
			out_value = value0 - value1;
			out_deriv = deriv0 - deriv1;
			break;
		case MULTIPLY:
			out_value = value0*value1;
			out_deriv = value0*deriv1 + value1*deriv0;
			break;
		case DIVIDE:
			out_value = value0/value1;
			out_deriv = (deriv0*value1 - deriv1*value0)/(value1*value1);
			break;
		case COMPOSITION:
			out_deriv = rad_forward_grad(func->func, func->input_values, func->input_derivatives, &out_value);
			break;
		case CUSTOM:
			out_value = func->custom_eval(func->input_values, func->input_grad);
			out_deriv = 0;
			for(i = 0; i < func->num_inputs; i++){
				out_deriv += func->input_derivatives[i]*func->input_grad[i];
			}
			break;
		default:
			break;
	}

	func->value = out_value;
	func->deriv = out_deriv;

	if(value != NULL){
		*value = out_value;
	}

	return out_deriv;
}

double rad_forward_diff(rad_func *func, double *inputs, unsigned int input_id, double *value){
	unsigned int i;
	double value0;
	double deriv0;
	double value1;
	double deriv1;
	double out_value = 0;
	double out_deriv = 0;

	switch(func->operation){
		case ADD:
		case SUBTRACT:
		case MULTIPLY:
		case DIVIDE:
			deriv0 = rad_forward_diff(func->operand0, inputs, input_id, &value0);
			deriv1 = rad_forward_diff(func->operand1, inputs, input_id, &value1);
			break;
		case COMPOSITION:
		case CUSTOM:
			for(i = 0; i < func->num_inputs; i++){
				func->input_derivatives[i] = rad_forward_diff(func->inputs[i], inputs, input_id, func->input_values + i);
			}
			break;
		default:
			break;
	}

	switch(func->operation){
		case CONSTANT:
			out_value = func->const_value;
			out_deriv = 0;
			break;
		case INPUT:
			out_value = inputs[func->input_id];
			out_deriv = (func->input_id == input_id);
			break;
		case ADD:
			out_value = value0 + value1;
			out_deriv = deriv0 + deriv1;
			break;
		case SUBTRACT:
			out_value = value0 - value1;
			out_deriv = deriv0 - deriv1;
			break;
		case MULTIPLY:
			out_value = value0*value1;
			out_deriv = value0*deriv1 + value1*deriv0;
			break;
		case DIVIDE:
			out_value = value0/value1;
			out_deriv = (deriv0*value1 - deriv1*value0)/(value1*value1);
			break;
		case COMPOSITION:
			out_deriv = rad_forward_grad(func->func, func->input_values, func->input_derivatives, &out_value);
			break;
		case CUSTOM:
			out_value = func->custom_eval(func->input_values, func->input_grad);
			out_deriv = 0;
			for(i = 0; i < func->num_inputs; i++){
				out_deriv += func->input_derivatives[i]*func->input_grad[i];
			}
			break;
		default:
			break;
	}

	func->value = out_value;
	func->deriv = out_deriv;

	if(value != NULL){
		*value = out_value;
	}

	return out_deriv;
}

static void rad_backward_diff_recursive(rad_func *func, double deriv, double *derivatives){
	unsigned int i;

	switch(func->operation){
		case CONSTANT:
			return;
		case INPUT:
			derivatives[func->input_id] += deriv;
			return;
		case ADD:
			rad_backward_diff_recursive(func->operand0, deriv, derivatives);
			rad_backward_diff_recursive(func->operand1, deriv, derivatives);
			return;
		case SUBTRACT:
			rad_backward_diff_recursive(func->operand0, deriv, derivatives);
			rad_backward_diff_recursive(func->operand1, -deriv, derivatives);
			return;
		case MULTIPLY:
			rad_backward_diff_recursive(func->operand0, deriv*func->operand1->value, derivatives);
			rad_backward_diff_recursive(func->operand1, deriv*func->operand0->value, derivatives);
			return;
		case DIVIDE:
			rad_backward_diff_recursive(func->operand0, deriv/func->operand1->value, derivatives);
			rad_backward_diff_recursive(func->operand1, -deriv*func->operand0->value/(func->operand1->value*func->operand1->value), derivatives);
			return;
		case COMPOSITION:
		case CUSTOM:
			for(i = 0; i < func->num_inputs; i++){
				rad_backward_diff_recursive(func->inputs[i], deriv*func->input_derivatives[i], derivatives);
			}
			return;
		default:
			return;
	}
}

double rad_backward_diff(rad_func *func, double *inputs, double *derivatives){
	double output;

	output = rad_backward_diff_eval(func, inputs, global_invocation);
	global_invocation++;
	rad_backward_diff_recursive(func, 1, derivatives);
	return output;
}

