# Transpiler

The main functionality of this class is to replace statements by their symbols. 
In other words this class methods transpile main statements to symbolized statements.

Main functions of this class are:

1. `replaceVariables()`

    This method is a very common method among the project classes.
    It gets a `BinaryOperator` statement, real variables of statement, and symbols 
    of those variables as an input and returns transpiled statement
    as an output.
    
    Check this example:
    
    ```c++
    // Example statement:
   
    a = b + c;
   
    // Suppose we have the following pairs of variables and symbols:
   
    (a, s_a), (b, s_b), (c, s_c)
   
    // Output:
   
    s_a = s_b + s_c
    ```
   
   Type of statements that this method handles are all `BinaryOperatorStmt` type
   like these:
   
   ```c++
   a = b + c;
   a = b || c && d;
   ```
   
2. `replaceDeclaration()`
    
    This method only used for declaration with definitions in c++ language
    It only omits the declaration keyword from the declaration:
    
    ```c++
    // Example statement:
    
    int a = b + c;
    char c = 'c';
   
    // Output:
    
    a = b + c
    c = 'c'
    ``` 

3. `replaceFunctionCallByReturnValue()`
    
    This is a very important method. It is used while
    a function call appears in the code. This method is called
    after the function call constraints collected.
    
    ```c++
    // Example statement:
     
    a = foo();
    
    // Output: Suppose we know that the return value is b
   
    a = b
    ```

4. `replaceCompoundAssignment()`
    
    This method change compound assignments to assignments:
    
    ```c++
    // Example statement:
   
    a += b;
   
    // Output:
   
    a = a + b
    ```      