enum rad_oper{
	CONSTANT,
	ADD,
	SUBTRACT,
	MULTIPLY,
	DIVIDE,
	INPUT
};

struct rad_func{
	enum rad_oper operation;
	union{
		struct{
			struct rad_func *operand0;
			struct rad_func *operand1;
			double input0;
			double input1;
			double d_input0;
			double d_input1;
		};
		double const_value;
		unsigned int input_id;
	};
	unsigned int num_references;
	double value;
	double deriv;
	bool traversed;
	bool sum_traversed;
};

typedef struct rad_func rad_func;

