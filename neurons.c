#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include "rad.h"

rad_func *activation;

rad_func **new_layer(unsigned int num_neurons, rad_func **prev_layer, unsigned int prev_neurons, unsigned int *parameter){
	rad_func **output;
	rad_func *neuron;
	unsigned int i;
	unsigned int j;

	output = malloc(sizeof(rad_func *)*num_neurons);
	for(i = 0; i < num_neurons; i++){
		neuron = rad_multiply(rad_copy(prev_layer[0]), rad_input(*parameter));
		++*parameter;
		for(j = 1; j < prev_neurons; j++){
			neuron = rad_add(neuron, rad_multiply(rad_copy(prev_layer[j]), rad_input(*parameter)));
			++*parameter;
		}
		neuron = rad_add(neuron, rad_input(*parameter));
		++*parameter;
		output[i] = rad_composition(rad_copy(activation), 1, neuron);
	}

	for(i = 0; i < prev_neurons; i++){
		rad_discard(prev_layer[i]);
	}

	return output;
}

rad_func **input_layer(unsigned int num_neurons, unsigned int input_start){
	rad_func **output;
	unsigned int i;

	output = malloc(sizeof(rad_func *)*num_neurons);
	for(i = 0; i < num_neurons; i++){
		output[i] = rad_input(i + input_start);
	}

	return output;
}

rad_func *net_error(/*not consumed*/rad_func **layer, unsigned int num_outputs){
	rad_func *output;
	rad_func *prod;
	unsigned int i;

	output = rad_subtract(rad_copy(layer[0]), rad_input(0));
	output = rad_multiply(rad_copy(output), output);
	for(i = 1; i < num_outputs; i++){
		prod = rad_subtract(rad_copy(layer[i]), rad_input(i));
		prod = rad_multiply(rad_copy(prod), prod);
		output = rad_add(output, prod);
	}

	return output;
}

double rad_teach(rad_func *error_func, double *parameters, double *derivs, double c, unsigned int num_parameters, unsigned int parameter_start){
	double error;
	unsigned int i;

	for(i = parameter_start; i < num_parameters; i++){
		derivs[i] *= 0.75;
	}
	error = rad_backward_diff(error_func, parameters, derivs);
	for(i = parameter_start; i < num_parameters; i++){
		parameters[i] -= derivs[i]*c;
	}

	return error;
}

double custom_exp(double *input, double *grad){
	*grad = exp(*input);
	return *grad;
}

int main(int argc, char **argv){
	rad_func **layer0;
	rad_func **layer1;
	rad_func **layer2;
	rad_func *error_func;
	unsigned int parameter = 3;
	unsigned int i;
	unsigned int in0;
	unsigned int in1;
	double *parameters;
	double *derivatives;
	double error;

	srand(time(NULL));
	activation = rad_parse("1/(1 + {0})", rad_custom(custom_exp, 1, rad_parse("0.0 - [0]")));

	layer0 = input_layer(2, 1);
	layer1 = new_layer(3, layer0, 2, &parameter);
	layer2 = new_layer(1, layer1, 3, &parameter);
	error_func = net_error(layer2, 1);

	parameters = malloc(sizeof(double)*parameter);
	derivatives = malloc(sizeof(double)*parameter);

	for(i = 0; i < parameter; i++){
		parameters[i] = ((double) rand())/UINT_MAX;
		derivatives[i] = 0.0;
	}

	for(i = 0; i < 100000; i++){
		in0 = rand()%2;
		in1 = rand()%2;
		if(in0 == in1){
			parameters[0] = 0;
		} else {
			parameters[0] = 1;
		}
		parameters[1] = in0;
		parameters[2] = in1;
		error = rad_teach(error_func, parameters, derivatives, 0.05, parameter, 3);
		printf("%u %u %d %lf - error: %lf\n", in0, in1, in0 != in1, layer2[0]->value, error);
	}

	printf("parameters: ");
	for(i = 3; i < parameter; i++){
		printf("%lf ", parameters[i]);
	}
	printf("\n");

	free(parameters);
	free(derivatives);
	rad_discard(error_func);
	rad_discard(layer2[0]);
	rad_discard(activation);
	free(layer2);
	free(layer1);
	free(layer0);

	return 0;
}
