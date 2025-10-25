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

constexpr bool int_pre(int input) {
    return CHECK(input > 0);
}
constexpr bool int_post(int ret) {
    return CHECK(ret > 2);
}
inline int f(int input, rdbc::PrePost<&int_pre, &int_post> c = {rdbc::THROW}) {
    c.pre_check(input);
    return c.post_check_ret(input+1, rdbc::THROW);
}

TEST(Basic, ThrowModeThrows) {
    EXPECT_THROW([&]{
        try {
            f(0);
        }
        catch(rdbc::ContractViolation e) {
            EXPECT_STREQ(e.condition, "input > 0");
            throw e;
        }
    }(), rdbc::ContractViolation);

    EXPECT_THROW([&]{
        try {
            f(1);
        }
        catch(rdbc::ContractViolation e) {
            EXPECT_STREQ(e.condition, "ret > 2");
            throw e;
        }
    }(), rdbc::ContractViolation);
}