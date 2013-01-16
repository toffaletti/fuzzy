#ifndef FUZZY_MAP_HH
#define FUZZY_MAP_HH
#include "mmap_array.hh"
#include "bits/hash_bytes.h"

// fuzzy map hashes the key NK times and stores
// the value at NK locations
// retrieval merges the values at all the locations

template <class Key, class Value, class Merge, size_t NK=1, size_t Seed=1938943857>
class fuzzy_map : private mmap_array<Value> {
    static_assert(NK >= 1, "NK must be >= 1");

    static size_t fnv(size_t h) {
        return std::_Fnv_hash_bytes(&h, sizeof(h), Seed);
    }

    template <size_t> struct index {};
public:
    fuzzy_map(size_t size) : mmap_array<Value>(size) {}
    fuzzy_map(const char *filename, size_t size) : mmap_array<Value>(filename, size) {}

    const Value operator[](const Key &key) const {
        std::hash<Key> hash;
        Value v;
        _get(v, fnv(hash(key)), index<NK>());
        return v;
    }

    void set(const Key &key, const Value &v) {
        std::hash<Key> hash;
        _set(fnv(hash(key)), v, index<NK>());
    }
private:
    Value &at(size_t hash) const {
        return mmap_array<Value>::operator[](hash % mmap_array<Value>::size());
    }

    void _get(Value &, size_t, index<0>) const {
        return;
    }

    template <size_t N>
        void _get(Value &v, size_t hash, index<N>) const {
            if (N == NK) {
                v = at(hash);
            } else {
                Merge merge;
                merge(v, at(hash));
            }
            _get(v, fnv(hash), index<N-1>());
        }

    void _set(size_t, const Value &, index<0>) {
        return;
    }

    template <size_t N>
        void _set(size_t hash, const Value &v, index<N>) {
            at(hash) = v;
            _set(fnv(hash), v, index<N-1>());
        }
};
#endif
