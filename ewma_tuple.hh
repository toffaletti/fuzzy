#ifndef EWMA_TUPLE_HH
#define EWMA_TUPLE_HH
#include <chrono>
#include <array>
#include <cmath>

template <class TickResolution, class DecayResolution, size_t ...Decays>
struct ewma_tuple {
    typedef std::chrono::steady_clock Clock;

    std::array<double, sizeof...(Decays)> rates = {{}};
    Clock::time_point updated = {};

    template <size_t D> struct decay {};
    template <size_t N> struct index {};

    void update(double v, Clock::time_point now) {
        using namespace std::chrono;
        double z = -(double)duration_cast<TickResolution>(now - updated).count();
        update_rates(v, z, decay<Decays>()...);
        updated = now;
    }

    template <size_t D, size_t ...Ds>
    void update_rates(double v, double z, decay<D>, decay<Ds>...) {
        using namespace std::chrono;
        constexpr size_t i = sizeof...(Decays) - (sizeof...(Ds)+1);
        double r = rate(z, index<i>(), decay<D>());
        r += v / duration_cast<TickResolution>(DecayResolution{D}).count();
        std::get<i>(rates) = r;
        update_rates(v, z, decay<Ds>()...);
    }

    template <size_t D>
    void update_rates(double v, double z, decay<D>) {
        using namespace std::chrono;
        constexpr size_t i = sizeof...(Decays)-1;
        double r = rate(z, index<i>(), decay<D>());
        r += v / duration_cast<TickResolution>(DecayResolution{D}).count();
        std::get<i>(rates) = r;
    }

    template <size_t N, class RateResolution=TickResolution>
    double rate(const Clock::time_point &now) const {
        using namespace std::chrono;
        double z = -(double)duration_cast<TickResolution>(now - updated).count();
        return rate(z, index<N>(), decay<Decays>()...) *
            (double)duration_cast<TickResolution>(RateResolution{1}).count();
    }

    template <size_t N, size_t D, size_t ...Ds>
    double rate(double z, index<N>, decay<D>, decay<Ds>...) const {
        constexpr size_t i = sizeof...(Decays) - (sizeof...(Ds)+1);
        if (i == N) {
            return rate(z, index<N>(), decay<D>());
        }
        return rate(z, index<N>(), decay<Ds>()...);
    }

    template <size_t N, size_t D, size_t ...Ds>
    double rate(double z, index<N>, decay<D>) const {
        using namespace std::chrono;
        double m = std::exp(z / duration_cast<TickResolution>(DecayResolution{D}).count());
        double r = std::get<N>(rates);
        return r * m;
    }
};

#endif
