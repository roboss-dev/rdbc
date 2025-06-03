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

#include "rdbc.hpp"
#include <gtest/gtest.h>

namespace rdbc = roboss::dbc;

constexpr bool void_pre() {
    return PRE(true);
}
constexpr bool void_post() {
    return POST(true);
}
inline void f_no_check(rdbc::Contract<rdbc::Pre<&void_pre>, rdbc::Post<&void_post>> c = {}) {
    // noop
}
inline void f_no_pre_check(rdbc::Contract<rdbc::Pre<&void_pre>, rdbc::Post<&void_post>> c = {}) {
    c.post_check();
}
inline void f_no_post_check(rdbc::Contract<rdbc::Pre<&void_pre>, rdbc::Post<&void_post>> c = {}) {
    c.pre_check();
}

#ifdef RDBC_TESTING_INTERNAL
TEST(Basic, ConditionChecksAreEnforced) {
    rdbc::terminate_called = false;
    f_no_check();
    EXPECT_TRUE(rdbc::terminate_called);

    rdbc::terminate_called = false;
    f_void_to_void_no_pre_check();
    EXPECT_TRUE(rdbc::terminate_called);

    rdbc::terminate_called = false;
    f_no_post_check();
    EXPECT_TRUE(rdbc::terminate_called);
}

#ifndef NDEBUG
TEST(Optimized, ConditionChecksAreEnforcedInDebug) {
    rdbc::terminate_called = false;
    f_no_check(rdbc::SKIP_PRE_IN_RELEASE);
    EXPECT_TRUE(rdbc::terminate_called);

    rdbc::terminate_called = false;
    f_void_to_void_no_pre_check(rdbc::SKIP_PRE_IN_RELEASE);
    EXPECT_TRUE(rdbc::terminate_called);

    rdbc::terminate_called = false;
    f_no_post_check(rdbc::SKIP_PRE_IN_RELEASE);
    EXPECT_TRUE(rdbc::terminate_called);
}
#endif
#endif

constexpr bool int_pre(int input) {
    return PRE(input > 0);
}
constexpr bool int_post(int ret) {
    return POST(ret > 0);
}
inline int f_failing_pre(int input = 0, rdbc::Contract<rdbc::Pre<&int_pre>, rdbc::Post<&int_post>> c = {}) {
    c.pre_check(input);
    return c.post_check(1);
}
inline int f_failing_post(int input = 1, rdbc::Contract<rdbc::Pre<&int_pre>, rdbc::Post<&int_post>> c = {}) {
    c.pre_check(input);
    return c.post_check(0);
}

TEST(Basic, ProgramTerminatesUponContractViolation) {
    EXPECT_THROW([&]{
        try {
            f_failing_pre();
        }
        catch(rdbc::ContractViolation e) {
            EXPECT_STREQ(e.condition, "PRECONDITION");
            throw e;
        }
    }(), rdbc::ContractViolation);

    EXPECT_THROW([&]{
        try {
            f_failing_post();
        }
        catch(rdbc::ContractViolation e) {
            EXPECT_STREQ(e.condition, "POSTCONDITION");
            throw e;
        }
    }(), rdbc::ContractViolation);
}

