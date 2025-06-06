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
#include <stdexcept>
#include <cstddef>

// TODO use stack trace instead
#define PRE(predicate) roboss::dbc::contract_violation_handler(predicate, "PRECONDITION", __FILE__, __LINE__)
#define POST(predicate) roboss::dbc::contract_violation_handler(predicate, "POSTCONDITION", __FILE__, __LINE__)
#define INV(predicate) roboss::dbc::contract_violation_handler(predicate, "INVARIANT", __FILE__, __LINE__)

#ifndef RDBC_TESTING
#define RDBC_NOEXCEPT true
#else
#define RDBC_NOEXCEPT false
#endif

namespace roboss {
namespace dbc {

struct ContractViolation {
  const char * condition;
  const char * file;
  int loc;
};

constexpr bool contract_violation_handler(bool predicate, const char* condition, const char* file, int loc) {
  if(!predicate) {
    if constexpr (RDBC_NOEXCEPT) {
      std::cerr << condition << " VIOLATION: " << file << ": " << loc << std::endl;
    }
    else {
      throw ContractViolation{condition, file, loc};
    }
  }
  return predicate;
}

enum struct ConditionType {
  PREC,
  POSTC,
  INVA,
  NOC
};

constexpr const char* ConditionName[] = {
  "PRECONDITION",
  "POSTCONDITION",
  "INVARIANT",
  "NOCONDITION"
};

constexpr auto condition_name(ConditionType type) {
  return ConditionName[static_cast<int>(type)];
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

template <auto condition_function, ConditionType condition_type>
struct Condition : AnyCondition {
  static_assert(is_condition_function<decltype(condition_function)>::value);
  static constexpr auto condition_function_ = condition_function;
  static constexpr auto type_ = condition_type;
  using Ret = typename signature<condition_function>::FirstArg;
  using Args = typename signature<condition_function>::Args;
};

template <auto condition_function>
using PreCondition = Condition<condition_function, ConditionType::PREC>;

template <auto condition_function>
using PostCondition = Condition<condition_function, ConditionType::POSTC>;

struct NoCondition : AnyCondition
{
  static constexpr bool condition_function_() { return true; }
  static constexpr auto type_ = ConditionType::NOC;
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

template <ConditionType condition_type, auto condition_function>
struct ContractCondition
{
  template <typename ... Arg>
  constexpr void check(Optimization optimization, Arg &&... arg) {
    if(optimization != Opt && !invoke_condition_function<condition_function>(arg...)) {
      std::terminate();
    }
    checked_ = true;
  }

  inline ~ContractCondition() {
    if(!checked_ && std::uncaught_exceptions() == 0) {
#ifndef RDBC_TESTING_INTERNAL
      std::cerr << condition_name(condition_type) << " NOT CHECKED" << std::endl;
      std::terminate();
#else
      terminate_called = true;
#endif
    }
  }

  bool checked_ = false;
};

template <typename Condition>
struct ContractPrecondition;

template <typename ... Arg, ConditionFunction<Arg...> condition_function>
struct ContractPrecondition<PreCondition<condition_function>>
{
  constexpr ContractPrecondition(Optimization precondition_optimization)
  : precondition_optimization_(precondition_optimization)
  { }

  constexpr bool pre_check(Arg const& ... arg) {
    impl_.check(precondition_optimization_, arg...);
    return true;
  }

  ContractCondition<ConditionType::PREC, condition_function> impl_;
  Optimization precondition_optimization_;
};

template <typename Object, typename ... Arg, ConditionMemberFunction<Object, Arg...> condition_function>
struct ContractPrecondition<PreCondition<condition_function>>
{
  constexpr ContractPrecondition(Optimization precondition_optimization)
  : precondition_optimization_(precondition_optimization)
  { }

  constexpr bool pre_check(Object const* object, Arg const& ... arg) {
    impl_.check(precondition_optimization_, object, arg...);
    return true;
  }

  ContractCondition<ConditionType::PREC, condition_function> impl_;
  Optimization precondition_optimization_;
};

template <>
struct ContractPrecondition<NoCondition>
{ 
  constexpr ContractPrecondition(Optimization)
  { }
};

template <typename Condition, typename Ret, typename Args>
struct ContractPostcondition;

template <typename Ret, typename ... Arg, ConditionFunction<Ret, Arg...> condition_function>
struct ContractPostcondition<PostCondition<condition_function>, Ret, std::tuple<Arg...>>
{
  using RetT = std::remove_cv_t<std::remove_reference_t<Ret>>;
  [[nodiscard]]
  constexpr RetT post_check(RetT && ret, Arg  ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, ret, arg...);
    return ret; // TODO support RVO
  }
  [[nodiscard]]
  constexpr RetT post_check(RetT & ret, Arg ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, ret, arg...);
    return std::move(ret); // TODO support RVO
  }

  ContractCondition<ConditionType::POSTC, condition_function> impl_;
};

template <typename Object, typename Ret, typename ... Arg, ConditionMemberFunction<Object, Ret, Arg...> condition_function>
struct ContractPostcondition<PostCondition<condition_function>, Ret, std::tuple<Arg...>>
{
  using RetT = std::remove_cv_t<std::remove_reference_t<Ret>>;
  [[nodiscard]]
  constexpr RetT post_check(Object const* object, RetT && ret, Arg ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, object, ret, arg...);
    return ret; // TODO support RVO
  }
  [[nodiscard]]
  constexpr RetT post_check(Object const* object, RetT & ret, Arg ... arg, Optimization optimization = NoOpt) {
    impl_.check(optimization, object, ret, arg...);
    return std::move(ret); // TODO support RVO
  }

  ContractCondition<ConditionType::POSTC, condition_function> impl_;
};

template <ConditionFunction<> condition_function>
struct ContractPostcondition<PostCondition<condition_function>, void, std::tuple<>>
{
  constexpr void post_check(Optimization optimization = NoOpt) {
    impl_.check(optimization);
  }

  ContractCondition<ConditionType::POSTC, condition_function> impl_;
};

template <>
struct ContractPostcondition<NoCondition, void, std::tuple<>>
{ };

template <typename Condition>
using contract_precondition_t = ContractPrecondition<Condition>;

template <typename Condition>
using contract_postcondition_t = ContractPostcondition<Condition, typename Condition::Ret, typename Condition::Args>;

template <typename PreCondition, typename PostCondition>
struct Contract : contract_precondition_t<PreCondition>,
                  contract_postcondition_t<PostCondition>
{
  static_assert(std::is_base_of_v<AnyCondition, PreCondition>, "PreCondition invalid");
  static_assert(std::is_base_of_v<AnyCondition, PostCondition>, "PostCondition invalid");

  constexpr Contract(Optimization precondition_optimization)
  : ContractPrecondition<PreCondition>(precondition_optimization)
  { }

  constexpr Contract()
  : Contract(NoOpt)
  { }
};

template <auto precondition_function>
using Pre = Contract<PreCondition<precondition_function>, NoCondition>;

template <auto postcondition_function>
using Post = Contract<NoCondition, PostCondition<postcondition_function>>;

template <auto precondition_function, auto postcondition_function>
using PrePost = Contract<PreCondition<precondition_function>, PostCondition<postcondition_function>>;


}
}