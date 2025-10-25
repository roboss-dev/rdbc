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
    return CHECK(true);
}
constexpr bool void_post() {
    return CHECK(true);
}
inline void f_no_check(rdbc::PrePost<&void_pre, &void_post> c = {}) {
    // noop
}
inline void f_no_pre_check(rdbc::PrePost<&void_pre, &void_post> c = {}) {
    c.post_check();
}
inline void f_no_post_check(rdbc::PrePost<&void_pre, &void_post> c = {}) {
    c.pre_check();
}

TEST(Basic, ConditionChecksAreEnforced) {
    rdbc::internal::terminate_called = false;
    f_no_check();
    EXPECT_TRUE(rdbc::internal::terminate_called);

    rdbc::internal::terminate_called = false;
    f_no_pre_check();
    EXPECT_TRUE(rdbc::internal::terminate_called);

    rdbc::internal::terminate_called = false;
    f_no_post_check();
    EXPECT_TRUE(rdbc::internal::terminate_called);
}

#ifndef NDEBUG
TEST(Optimized, ConditionChecksAreEnforcedInDebug) {
    rdbc::internal::terminate_called = false;
    f_no_check(rdbc::internal::SKIP_PRE_IN_RELEASE);
    EXPECT_TRUE(rdbc::internal::terminate_called);

    rdbc::internal::terminate_called = false;
    f_no_pre_check(rdbc::internal::SKIP_PRE_IN_RELEASE);
    EXPECT_TRUE(rdbc::internal::terminate_called);

    rdbc::internal::terminate_called = false;
    f_no_post_check(rdbc::internal::SKIP_PRE_IN_RELEASE);
    EXPECT_TRUE(rdbc::internal::terminate_called);
}
#endif
