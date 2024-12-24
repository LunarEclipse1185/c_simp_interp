# A simple interpreter of a (tweaked) subset of C

## The language

- Variables
  Only int and int array (support high dimensions). Declare one at a time: `int a; int b[3][4];`. Dimensions in declaration must be positive integer literal. Global variables and local variables act similarly like those in C, except that: 1) Only the globals declared before `main()` are available, and they don't even need to be after the functions in which they are accessed; 2) Locals can be used after the block ends, and are only destroyed on function exit.
  
- Functions
  Functions should and can only return `int`, and their arguments can only be of type `int`. Functions has implicit return value `0`. Recursion works.
  
- Statements
  Support expression statement, if-else, while and return statement. They act like those in C.
  
- IO
    Implementing `scanf` and `printf` are quite tedious, so the C++ `cin` and `cout` are adopted, used like `cin >> a >> b` and `cout << <EXPR> << endl`. `cin`, `cout` and `endl` are implemented as special variables, and operator `<<` and `>>` can only be used if the left operand evaluates to `cout` and `cin`, respectively, and the subexpression evaluates to `cout` and `cin` like they do in C++.
  
- Operators
  Only a limited set of operators are available. The only operator with side effect is `=`. Precedence is listed below:
`! +(unary) -(unary)
* / %
+ -
<= >= < >
== !=
^
&&
||
= << >>` <br>
  Note that `^` is *logical* XOR, not bitwise XOR.

## Program structure

- main
  Read code file, tokenize, build AST and then run in CVM. Usage: `./main <c-code-file> [<input-file> [<output file>]]`. A sample code and input are provided.
  
- Tokenizer
  Outputs an array of `Token` which is just a string view. No additional token type information is stored.
  
- ast\_builder
  Builds what is strictly called CST, no name table. Most syntaxes are checked at this stage, except number of subscripts in array element access, function argument count and expression typecheck. The builder basically performs a massive pattern matching. <br>
  Error handling in ast\_builder are yet to be completed. Now error happens in leaf node will be overwritten by ancestors when bubbling up.
  
- CVM (C Virtual Machine)
  Not really a virtual machine though. There is no translation to internal assembly code, instead it executes the code while traversing the AST. The callstack is just a dynamic array. Syntax errors will abort execution and no concrete error message are generated. There is no array out-of-bound access check.
  
  <br><br>
  
  This project will probably soon be improved.
  
