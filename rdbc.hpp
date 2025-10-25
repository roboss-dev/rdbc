// Copyright 2025 Zoltan Resi

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <iostream>
#include <type_traits>
#include <utility>
#include <cstddef>

#define CHECK(PREDICATE) roboss::dbc::internal::check(PREDICATE, #PREDICATE, __FILE__, __LINE__)

#ifndef RDBC_TESTING
#define RDBC_NOEXCEPT true
#else
#define RDBC_NOEXCEPT false
#define RDBC_EXCEPTIONS
#endif

#ifdef RDBC_EXCEPTIONS
#include <stdexcept>
#endif


namespace roboss {
namespace dbc {

struct ContractViolation {
  const char * condition = nullptr;
  const char * file = nullptr;
  std::size_t line = 0;
};


namespace internal {

struct CheckResult
{
  bool has_violation = false;
  ContractViolation violation;
};

inline thread_local CheckResult current_result = {false, nullptr, nullptr, 0};

constexpr bool check(bool predicate, const char* predicate_string, const char* file, std::size_t line) noexcept(RDBC_NOEXCEPT) {
  current_result = {predicate, predicate_string, file, line};
  return predicate;
}


}

enum Optimization {
  NoOpt,
  Opt
};

#ifdef NDEBUG
constexpr auto SKIP_PRE_IN_RELEASE = Opt;
constexpr auto SKIP_POST_IN_RELEASE = Opt;
#else
constexpr auto SKIP_PRE_IN_RELEASE = NoOpt;
constexpr auto SKIP_POST_IN_RELEASE = NoOpt;
#endif

struct AnyCondition
{};

template <typename ... Arg>
using ConditionFunction = bool(*)(Arg ... arg);

template <typename Object, typename ... Arg>
using ConditionMemberFunction = bool(Object::*)(Arg ... arg)const;

template <typename T>
struct is_condition_function : std::false_type {};

template <typename ... Arg>
struct is_condition_function<ConditionFunction<Arg...>> : std::true_type {};

template <typename Object, typename ... Arg>
struct is_condition_function<ConditionMemberFunction<Object, Arg...>> : std::true_type {};

template <typename ConditionFunction>
struct signature_helper;

template <typename Obj, typename Arg1, typename ... Arg>
struct signature_helper<bool(Obj::*)(Arg1, Arg...)const> {
  using FirstArg = Arg1;
  using Object = Obj;
  using Args = std::tuple<Arg...>;
};

template <typename Obj>
struct signature_helper<bool(Obj::*)()const> {
  using FirstArg = void;
  using Object = Obj;
  using Args = std::tuple<>;
};

template <typename Arg1, typename ... Arg>
struct signature_helper<bool(*)(Arg1, Arg...)> {
  using FirstArg = Arg1;
  using Args = std::tuple<Arg...>;
};

template <>
struct signature_helper<bool(*)()> {
  using FirstArg = void;
  using Args = std::tuple<>;
};

template <auto condition_function>
using signature = signature_helper<decltype(condition_function)>;

template <auto condition_function>
struct Condition : AnyCondition {
  static_assert(is_condition_function<decltype(condition_function)>::value);
  static constexpr auto condition_function_ = condition_function;
  using Ret = typename signature<condition_function>::FirstArg;
  using Args = typename signature<condition_function>::Args;
};

struct NoCondition : AnyCondition
{
  static constexpr bool condition_function_() { return true; }
  using Ret = void;
  using Args = std::tuple<>;
};

template <auto condition_function, typename Arg1, typename ... Arg>
constexpr bool invoke_condition_function(Arg1 && arg1, Arg &&... arg) {
  if constexpr(std::is_member_function_pointer_v<decltype(condition_function)>) {
    return (arg1->*condition_function)(arg...);
  }
  else {
    return condition_function(arg1, arg...);
  }
}

template <auto condition_function>
constexpr bool invoke_condition_function() {
  return condition_function();
}

#ifdef RDBC_TESTING_INTERNAL
thread_local bool terminate_called = false;
#endif

template <auto condition_function, bool cpp17_constexpr = false>
struct ContractCondition
{
  template <typename ... Arg>
  constexpr void check(Optimization optimization, Arg &&... arg) {
    if(optimization != Opt && !invoke_condition_function<condition_function>(arg...)) {
      if constexpr(RDBC_NOEXCEPT) {
        std::cerr << "CONTRACT VIOLATION - the following condition was not true:\n\t" << internal::current_result.violation.condition
          << "\nin file:\n\t " << internal::current_result.violation.file << "\nat line:\n\t " << internal::current_result.violation.line << std::endl;
        std::terminate();
      }
      else {
        throw internal::current_result.violation;
      }
    }
    checked_ = true;
  }

  inline ~ContractCondition() {
    if(!checked_ && std::uncaught_exceptions() == 0) {
#ifndef RDBC_TESTING_INTERNAL
      std::cerr << "CONTRACT NOT CHECKED" << std::endl;
      std::terminate();
#else
      terminate_called = true;
#endif
    }
  }

  bool checked_ = false;
};

template <auto condition_function>
struct ContractCondition<condition_function, true>
{
  template <typename ... Arg>
  constexpr void check(Optimization optimization, Arg &&... arg) {
    if(optimization != Opt && !invoke_condition_function<condition_function>(arg...)) {
      std::terminate();
    }
  }
};

template <typename Condition, bool cpp17_constexpr>
struct ContractPrecondition;

template <typename ... Arg, ConditionFunction<Arg...> condition_function, bool cpp17_constexpr>
struct ContractPrecondition<Condition<condition_function>, cpp17_constexpr>
{
  constexpr ContractPrecondition(Optimization precondition_optimization)
  : precondition_optimization_(precondition_optimization)
  { }

  constexpr bool pre_check(Arg const& ... arg) {
    impl_.check(precondition_optimization_, arg...);
    return true;
  }

  ContractCondition<condition_function, cpp17_constexpr> impl_;
  Optimization precondition_optimization_;
};

template <typename Object, typename ... Arg, ConditionMemberFunction<Object, Arg...> condition_function, bool cpp17_constexpr>
struct ContractPrecondition<Condition<condition_function>, cpp17_constexpr>
{
  constexpr ContractPrecondition(Optimization precondition_optimization)
  : precondition_optimization_(precondition_optimization)
  { }

  constexpr bool pre_check(Object const* object, Arg const& ... arg) {
    impl_.check(precondition_optimization_, object, arg...);
    return true;
  }

  ContractCondition<condition_function, cpp17_constexpr> impl_;
  Optimization precondition_optimization_;
};

template <bool cpp17_constexpr>
struct ContractPrecondition<NoCondition, cpp17_constexpr>
{ 
  constexpr ContractPrecondition(Optimization)
  { }
};

template <typename Condition, typename Ret, typename Args, bool cpp17_constexpr>
struct ContractPostcondition;

template <typename Ret, typename ... Arg, ConditionFunction<Ret, Arg...> condition_function, bool cpp17_constexpr>
struct ContractPostcondition<Condition<condition_function>, Ret, std::tuple<Arg...>, cpp17_constexpr>
{
  using RetT = std::remove_cv_t<std::remove_reference_t<Ret>>;
  constexpr void post_check(RetT const& arg1, Arg  ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, arg1, arg...);
  }
  [[nodiscard]]
  constexpr RetT post_check_ret(RetT && ret, Arg  ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, ret, arg...);
    return ret; // TODO support RVO
  }
  [[nodiscard]]
  constexpr RetT post_check_ret(RetT & ret, Arg ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, ret, arg...);
    return std::move(ret); // TODO support RVO
  }

  ContractCondition<condition_function, cpp17_constexpr> impl_;
};

template <typename Object, typename Ret, typename ... Arg, ConditionMemberFunction<Object, Ret, Arg...> condition_function, bool cpp17_constexpr>
struct ContractPostcondition<Condition<condition_function>, Ret, std::tuple<Arg...>, cpp17_constexpr>
{
  using RetT = std::remove_cv_t<std::remove_reference_t<Ret>>;
  constexpr void post_check(Object const* object, RetT const& arg1, Arg  ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, object, arg1, arg...);
  }
  [[nodiscard]]
  constexpr RetT post_check_ret(Object const* object, RetT && ret, Arg ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, object, ret, arg...);
    return ret; // TODO support RVO
  }
  [[nodiscard]]
  constexpr RetT post_check_ret(Object const* object, RetT & ret, Arg ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, object, ret, arg...);
    return std::move(ret); // TODO support RVO
  }

  ContractCondition<condition_function, cpp17_constexpr> impl_;
};

template <ConditionFunction<> condition_function, bool cpp17_constexpr>
struct ContractPostcondition<Condition<condition_function>, void, std::tuple<>, cpp17_constexpr>
{
  constexpr void post_check(Optimization optimization = NoOpt) {
    impl_.check(optimization);
  }

  ContractCondition<condition_function, cpp17_constexpr> impl_;
};

template <bool cpp17_constexpr>
struct ContractPostcondition<NoCondition, void, std::tuple<>, cpp17_constexpr>
{ };

template <typename Condition, bool cpp17_constexpr>
using contract_precondition_t = ContractPrecondition<Condition, cpp17_constexpr>;

template <typename Condition, bool cpp17_constexpr>
using contract_postcondition_t = ContractPostcondition<Condition, typename Condition::Ret, typename Condition::Args, cpp17_constexpr>;

template <typename PreCondition, typename PostCondition, bool cpp17_constexpr>
struct Contract : contract_precondition_t<PreCondition, cpp17_constexpr>,
                  contract_postcondition_t<PostCondition, cpp17_constexpr>
{
  static_assert(std::is_base_of_v<AnyCondition, PreCondition>, "PreCondition invalid");
  static_assert(std::is_base_of_v<AnyCondition, PostCondition>, "PostCondition invalid");

  constexpr Contract(Optimization precondition_optimization)
  : ContractPrecondition<PreCondition, cpp17_constexpr>(precondition_optimization)
  { }

  constexpr Contract()
  : Contract(NoOpt)
  { }
};

template <auto precondition_function, bool cpp17_constexpr = false>
using Pre = Contract<Condition<precondition_function>, NoCondition, cpp17_constexpr>;

template <auto postcondition_function, bool cpp17_constexpr = false>
using Post = Contract<NoCondition, Condition<postcondition_function>, cpp17_constexpr>;

template <auto precondition_function, auto postcondition_function, bool cpp17_constexpr = false>
using PrePost = Contract<Condition<precondition_function>, Condition<postcondition_function>, cpp17_constexpr>;

struct Contractual {
  template <typename ConstructorContract, typename ... Arg>
  constexpr Contractual(ConstructorContract && contract, Arg &&... arg) noexcept {
    contract.pre_check(std::forward<Arg&&>(arg)...);
  }

  constexpr Contractual() noexcept = default;
};


}
}