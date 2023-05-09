# RAD - Rad Auto-Differentiator
A C library for automatic foward and backward differentiation.

## How to Use
Use RAD by dynamically allocating expressions, or RAD functions, typed `rad_func *`. There are several functions available for this, such as `rad_const`, `rad_input`, and `rad_multiply`.
Each function has inputs indexed by an `unsigned int`. For example, `rad_input(n)` creates a RAD function which outputs the input with index `n`.
The functions `rad_eval`, `rad_forward_grad`, `rad_forward_diff`, and `rad_backward_diff` are used for evaluating and computing derivatives of RAD functions, and they accept buffers for derivatives indexed by the inputs.
RAD comes with a built-in reference counter to assist with garbage collection.
All library functions except for `rad_copy` and `rad_deep_copy` consume each input RAD function, so a `rad_func *` value should not be reused after being passed as an argument.
By instead passing the output of `rad_copy` as an argument, the user can indicate to the library that they plan on continuing to use the RAD function.
`rad_discard` may be used to indicate that the user no longer needs a RAD function, and the library will free memory if there are no other references to the RAD function.

## Example Program
An example program `neurons.c` is included. In less than 100 lines, the program uses RAD to create a neural network which may be optimized using backpropogation.
The neural network is then optimized for 100000 epochs to evaluate XOR.

## TODO
- Allow the user to specify their own functions to allocate memory. Currently, the library uses `malloc`.
- Fix an edge case involving compositions of rad functions. Currently, if `f` and `g` are `rad_func *`, then differentiation of the function `rad_add(rad_copy(f), rad_composition(f, 1, g))` will work incorrectly, while `rad_add(f, rad_composition(rad_deep_copy(f), 1, g))` works correctly.
