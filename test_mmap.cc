#define BOOST_TEST_MODULE mmap test
#include <boost/test/unit_test.hpp>
#include "mmap_array.hh"
#include "fuzzy_map.hh"

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
        a = std::max(a, b);
    }
};

BOOST_AUTO_TEST_CASE(fuzzy_map_test) {
    fuzzy_map<uint32_t, uint32_t, merge_int, 3> a(4*1024*30);
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
        double m = exp(z / a);
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

