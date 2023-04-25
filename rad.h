#include <stdarg.h>

enum rad_oper{
	CONSTANT,
	INPUT,
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVIDE,
	ARG,
	COMPOSITION
};

typedef struct rad_func rad_func;

struct rad_func{
	enum rad_oper operation;
	union{
		struct{
			struct rad_func *operand0;
			struct rad_func *operand1;
		};
		double const_value;
		unsigned int input_id;
		unsigned int arg_id;
		struct{
			unsigned int num_inputs;
			rad_func *func;
			rad_func **inputs;
			double *input_values;
			double *input_derivatives;
		};
	};
	unsigned int num_references;
	double value;
	double deriv;
	bool traversed;
	bool sum_traversed;
};

rad_func *rad_create_func(enum rad_oper operation, unsigned int num_references);
rad_func *rad_const(double const_value);
rad_func *rad_input(unsigned int input_id);
rad_func *rad_add(rad_func *operand0, rad_func *operand1);
rad_func *rad_subtract(rad_func *operand0, rad_func *operand1);
rad_func *rad_multiply(rad_func *operand0, rad_func *operand1);
rad_func *rad_divide(rad_func *operand0, rad_func *operand1);
rad_func *rad_composition(rad_func *func, unsigned int num_args, ...);
rad_func *rad_copy(rad_func *func);
void rad_free(rad_func *func);
void rad_discard(rad_func *func);
double rad_eval(rad_func *func, double *inputs);
double rad_forward_grad(rad_func *func, double *inputs, double *derivatives, double *value);
double rad_forward_diff(rad_func *func, double *inputs, unsigned int input_id, double *value);
double rad_backward_diff(rad_func *func, double *inputs, double *derivatives);
rad_func *rad_parse(const char *c, ...);
void rad_print(rad_func *func);
