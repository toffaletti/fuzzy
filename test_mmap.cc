#define BOOST_TEST_MODULE mmap test
#include <boost/test/unit_test.hpp>
#include "mmap_array.hh"
#include "fuzzy_map.hh"
#include <array>
#include <cmath>

BOOST_AUTO_TEST_CASE(mmap_anon_test) {
    mmap_array<uint32_t> a(1024);
    a[0] = 42;
    BOOST_CHECK_EQUAL(42, a[0]);
}

BOOST_AUTO_TEST_CASE(mmap_shared_test) {
    {
        mmap_array<uint32_t> a("/tmp/mmap_test", 1024);
        a[0] = 42;
        BOOST_CHECK_EQUAL(42, a[0]);
    }

    {
        mmap_array<uint32_t> a("/tmp/mmap_test", 1024);
        BOOST_CHECK_EQUAL(42, a[0]);
    }
    unlink("/tmp/mmap_test");
}

struct merge_int {
    void operator()(uint32_t &a, const uint32_t &b) {
        a = std::min(a, b);
    }
};

BOOST_AUTO_TEST_CASE(fuzzy_map_test) {
    fuzzy_map<uint32_t, uint32_t, merge_int, 3> a(2*1024);
    for (uint32_t i=0; i<150; ++i) {
        a.set(i, i);
    }

    for (uint32_t i=0; i<150; ++i) {
        BOOST_CHECK_EQUAL(i, a[i]);
        //BOOST_CHECK(i <= a[i]);
    }
}

struct rate {
    static constexpr double a = 36;

    double rate = 0.0;
    int32_t updated = 0;
    uint32_t trended = 0;

    void update(double v, uint32_t now) {
        double z = -(double)(now - updated);
        double m = std::exp(z / a);
        double r = rate;
        r *= m;
        r += v / a;
        rate = r;
        updated = now;
    }
} __attribute__((packed));

struct merge_rate {
    void operator()(rate &a, const rate &b) {
        a.rate = std::min(a.rate, b.rate);
        a.updated = std::min(a.updated, b.updated);
        a.trended = std::min(a.trended, b.trended);
    }
};

BOOST_AUTO_TEST_CASE(fuzzy_rate_test) {
    fuzzy_map<std::string, rate, merge_rate, 3> a(1024);

    rate r = a["hi"];
    for (unsigned i=0; i<36*5; ++i) {
        r.update(10, i);
    }
    a.set("hi", r);
    BOOST_CHECK_CLOSE(10.0, a["hi"].rate, 1.0);
}

template <time_t ...Decays>
struct dyn_rate {
    std::array<double, sizeof...(Decays)> rates = {{}};
    uint64_t updated = 0;

    template <time_t D> struct decay {};

    void update(double v, uint64_t now) {
        double z = -(double)(now - updated);
        update_rates(v, z, decay<Decays>()...);
        updated = now;
    }

    template <time_t D, time_t ...Ds>
    void update_rates(double v, double z, decay<D>, decay<Ds>...) {
        constexpr size_t i = sizeof...(Decays) - (sizeof...(Ds)+1);
        double m = std::exp(z / D);
        double r = std::get<i>(rates);
        r *= m;
        r += v / D;
        std::get<i>(rates) = r;
        update_rates(v, z, decay<Ds>()...);
    }

    template <time_t D>
    void update_rates(double v, double z, decay<D>) {
        constexpr size_t i = sizeof...(Decays)-1;
        double m = std::exp(z / D);
        double r = std::get<i>(rates);
        r *= m;
        r += v / D;
        std::get<i>(rates) = r;
    }

};

BOOST_AUTO_TEST_CASE(fuzzy_dyn_rate_test) {
    dyn_rate<60, 5*60, 10*60> dr;
    for (unsigned i=0; i<60*60; ++i) {
        dr.update(10.0, i);
    }
    BOOST_CHECK_CLOSE(10.0, std::get<0>(dr.rates), 1.0);
    BOOST_CHECK_CLOSE(10.0, std::get<1>(dr.rates), 1.0);
    BOOST_CHECK_CLOSE(10.0, std::get<2>(dr.rates), 1.0);
}

