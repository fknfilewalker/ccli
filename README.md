# Cool Command Line Interface

```c++
// declare a bool var that can be set with either -b or --bool
ccli::Var<bool> boolVar("b"sv, "bool"sv, false, ccli::NONE, "First bool Var"sv);
```
