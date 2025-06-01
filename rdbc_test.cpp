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