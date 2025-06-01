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
#include <cassert>

namespace rdbc = roboss::dbc;
constexpr bool f_pre(int i) {
    return PRE(i > 0);
}
constexpr bool f_post(double d) {
    return POST(d > 0);
}
inline double f(int i, rdbc::Contract<rdbc::Pre<&f_pre>, rdbc::Post<&f_post>> c = {}) {
    c.pre_check(i);

    return c.post_check(2, rdbc::SKIP_POST_IN_RELEASE);
}
static_assert(rdbc::Pre<&f_pre>::condition_function_ == &f_pre);


struct Ret {
    Ret() = default;
    Ret(Ret const& other) 
    {
        fprintf(stdout, "Copy\n");
    }
    Ret(Ret && other) 
    {
        fprintf(stdout, "Move\n");
    }
};

struct Object {
    using This = Object;
    constexpr bool f_pre(int i) const {
        return PRE(i > 0);
    }
    constexpr bool f_post(Ret const& ret, int dummy, double dummy2) const {
        return POST(true);
    }
    inline Ret f(int i, rdbc::Contract<rdbc::Pre<&This::f_pre>, rdbc::Post<&This::f_post>> c = {}) {
        c.pre_check(this, i);
        return c.post_check(this, {}, 10, 11);
    }
};

int main() {
    Object o;
    o.f(2, rdbc::SKIP_PRE_IN_RELEASE);
}