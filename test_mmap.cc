#define BOOST_TEST_MODULE mmap test
#include <boost/test/unit_test.hpp>
#include "mmap_array.hh"
#include "fuzzy_map.hh"
#include "ewma_tuple.hh"

using namespace std::chrono;

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

struct rate : public ewma_tuple<seconds, seconds, 36> {
    uint64_t trended = 0;
    uint64_t padding = 0;
} __attribute__((packed));

struct merge_rate {
    void operator()(rate &a, const rate &b) {
        std::get<0>(a.rates) = std::min(std::get<0>(a.rates), std::get<0>(b.rates));
        a.updated = std::min(a.updated, b.updated);
        a.trended = std::min(a.trended, b.trended);
    }
};

BOOST_AUTO_TEST_CASE(fuzzy_rate_test) {
    fuzzy_map<std::string, rate, merge_rate, 3> a(1024);

    rate r = a["hi"];
    steady_clock::time_point now = steady_clock::now();
    for (unsigned i=0; i<36*5; ++i) {
        r.update(10, now+seconds{i});
    }
    a.set("hi", r);
    BOOST_CHECK_CLOSE(10.0, std::get<0>(a["hi"].rates), 1.0);
}

BOOST_AUTO_TEST_CASE(fuzzy_dyn_rate_test) {
    ewma_tuple<seconds, seconds, 60, 5*60, 10*60> dr;
    steady_clock::time_point now = steady_clock::now();
    for (unsigned i=0; i<60*60; ++i) {
        dr.update(10.0, now+seconds{i});
    }
    BOOST_CHECK_CLOSE(10.0, std::get<0>(dr.rates), 1.0);
    BOOST_CHECK_CLOSE(10.0, std::get<1>(dr.rates), 1.0);
    BOOST_CHECK_CLOSE(10.0, std::get<2>(dr.rates), 1.0);
}

BOOST_AUTO_TEST_CASE(fuzzy_dyn_rate2_test) {
    ewma_tuple<seconds, minutes, 1, 5, 10> dr;
    steady_clock::time_point now = steady_clock::now();
    for (unsigned i=0; i<60*60; ++i) {
        dr.update(10.0, now+seconds(i));
    }
    BOOST_CHECK_CLOSE(10.0, dr.rate<0>(now+seconds(60*60)), 1.0);
    BOOST_CHECK_CLOSE(10.0, dr.rate<1>(now+seconds(60*60)), 1.0);
    BOOST_CHECK_CLOSE(10.0, dr.rate<2>(now+seconds(60*60)), 1.0);

    BOOST_CHECK_GT(1.0, dr.rate<0>(now+seconds(60*60*2)));
    BOOST_CHECK_GT(1.0, dr.rate<1>(now+seconds(60*60*2)));
    BOOST_CHECK_GT(1.0, dr.rate<2>(now+seconds(60*60*2)));

}

