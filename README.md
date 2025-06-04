# About

Design by Contract (or contract-driven development) helps in ensuring software correctness without compromising performance* and system complexity. The checks are defined as preconditions, postconditions and invariants. For more information on Design By Contract in general, see section [Further Resources](#resources).

*Effect on performance depends on enforcement semantics. Also, strong typing may be treated as a form of design by contract.

# Basic Usage

Free functions:
```c++
using namespace rdbc = roboss::dbc;
constexpr bool f_pre(int input1) {
  return PRE(input1 > 2);
}
constexpr bool f_post(double ret) {
  return POST(ret < 0.5);
}
double f(int input1, rdbc::PrePost<&f_pre, &f_post> contract = {}) {
  contract.pre_check(input1);
  // ...my function body...
  return c.post_check(0.2);
}
```

Member functions:
```c++
using namespace rdbc = roboss::dbc;
class MyClass {
public:
  constexpr bool f_pre(int input1) {
    return PRE(input1 > 2)
        && PRE(a_member_variable_ == 2);
  }
  constexpr bool f_post(double ret) {
    return POST(ret < 0.5)
        && POST(a_member_variable_ == 3);
  }
  double f(int input1, rdbc::PrePost<&f_pre, &f_post> contract = {}) {
    contract.pre_check(this, input1);
    // ...my function body...
    a_member_variable_ = 3;
    return c.post_check(this, 0.2);
  }

private:
  int a_member_variable_ = 2;
};
```


# Features

- Contract is (optionally) part of the function declaration
- Program termination if the pre- or postconditions of the function are not checked
- Preconditions may be switched off on the call site
- Contract semantics configurable via compiler flags (TODO) 

# Further Resources <a name="resources"></a>

- [Introduction to DbC](https://drdobbs.com/an-exception-or-a-bug/184401686)
- [Contracts for C++](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2900r11.pdf)
- [Boost.Contract](https://github.com/boostorg/contract/tree/develop)
- [DbC vs Defensive Programming](https://softwareengineering.stackexchange.com/a/125417)
- [Dbc in Eiffel](https://www.eiffel.org/doc/eiffel/ET-_Design_by_Contract_%28tm%29%2C_Assertions_and_Exceptions)