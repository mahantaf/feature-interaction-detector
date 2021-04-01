#  Symbol Table

This class keeps the mapping of variables to its symbols.
In addition in generates new symbols based on symbolic execution.

Main functions of this class are:

1. `addVariableSymbol()`
    
    This method is the main method of this class.
    It takes a variable and its type (**LVALUE** or **RVALUE**) and
    generates its symbol. 
    
    It also checks whether the variable is **global variable**,
    **function parameter**, **local variable**, or **class property**
    and based on those generates the symbol.
    
    **Function Parameter:**
    
    ```c++
    // Suppose we want to symbolize bar() function
   
    int foo(int c) {
      return c * 2;
    }
    
    void bar() {
      int a = 2;
      int b = foo(a);
    }
    
    // Output
   
    s0_b = s0_a * 2
    s0_a = 2
    ```
   
    **Local Variable:**
    
    ```c++
    // Suppose we want to symbolize bar() function
   
    int foo(int c) {
      int d = 2;
      return d * 2;
    }
    
    void bar() {
      int a = 2;
      int b = foo(a);
    }
    
    // Output
   
    s0_b = s0_d * 2
    s0_d = 2
    s0_a = 2
    ```
   
    **Class Property:**
    
    ```c++
    // Suppose Class Test has a property with name p 
    // We want to symbolize bar function
    
    void Test::bar() {
      int a = 2;
      int b = this->c + 2;
    }
    
    // Output
   
    s0_b = s0_this_c + 2
    s0_a = 2
    ```
   
2. `setParams()` and `setPassParams()`
    
    These two methods are used to set current function parameters
    and passed parameters which in order to separate different type
    of variable.
    
3. `saveState()` and `loadState()`
    
    When a function call appears traversing the CFG, we have to 
    go inside that function and collect the corresponding constraint
    of that function. Thus we have to save the current symbol table
    using `saveState()`.
    
    After a function call is finished, we have to load the symbol table
    and also update it with new symbols from that function. Thus we
    use `loadState()`.