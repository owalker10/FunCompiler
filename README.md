# FunCompiler
Takes programs written in simple language and compiles an assembly file. Written for an assignment in CS 429H Honors Computer Organization and Architecture.



The compiler looks for a file named \<name>.fun and compiles it,
producing an executable file named \<name>

For example:

let's say the file tx.fun contains

x = argc + 1
print x

running "./p4 tx" produces a file named tx

running "./tx" prints 1
running "./tx hello" prints 2

"make clean \<name>.test" compiles and runs \<name>.fun and compares the result to \<name>.ok and passes or fails the test.

## The Fun Langauge:

- Lexical rules: The language has the following tokens.

  * keywords: "if" "else" "while" "fun" and "print"

  * operators and spacial characters: = == ( ) { } + *

  * identifiers: start with lower case letters followed by a sequence of
                 lower case letters and numbers.

     examples: x hello h123 xy3yt abc123

  * immediate values: sequences of digits representing integers. They always
                 start with a digit but could contain '_' characters that are
                 ignored

     examples: 12 0 12_234

- Syntax: Our syntax is C-like. You're given most of the code
  but you need to change it in order to implement the "if" and "while"
  statements. Things to keep in mind about our language:

  * we only have assignment, if, while, function declarations/calls, and print statements

  * the following operators are supported '*', '+', '=='

### Language semantics:

- All variables are 64 bit unsigned integers

- '+' performs unsigned integer addition (mod 2^64)

- '*' performs unsigned integer multiplication (mod 2^64)

- 'x == y' returns 1 if x and y contain the same bit pattern and 0 otherwise

- 'x = <exp>' assigns the result of evaluating '<exp>' to 'x' and prints the new
  value of 'x'

- The 'if' statement has one of the forms:

      if <exp> <statement>
     
   or
 
      if <exp> <statement> else <statement>

   <exp> is considered true if it evaluates to a non-zero value, false otherwise

- The 'while' statement has the form:

      while <exp> <statement>

   <exp> is considered true if it evaluates to a non-zero value, false otherwise

- '{' and '}' are used to create block statements, making a sequence of
  statements appear like a single one

- The 'print' statement has the form:

      print <exp>

  It prints the numerical value of the expression followed by a new line



### Defining a function:


The function expression is as follows:

    fun <statement>

A "fun" expression returns an opaque 64 bit quantity. The only meaningful
thing you can do with the result is to store in a variable and use it later
to call the function. You can't make any assumptions about the returned value
from defining a function (function handle) other than it will be a 64 bit
quantity.

For example:

    x = fun print 125

   defines a function with "print 125" as its body and stores its handle in "x"

    y = fun {
          abc = 100
          pqr = 200
        }    

   defines a function that sets the values of "abc" and "pqr" and stores its
   handle in "y"

Calling a function:

   A call statement is added to the language:

    <identitifer> ()

   <identitifer> has to contain a value returned by "fun".

   The call statement executes the body associated with the function then
   continues

For example:

    x()
    x()

  prints

    125
    125
