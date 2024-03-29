#ifndef MMAP_ARRAY_HH
#define MMAP_ARRAY_HH
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <sstream>
#include <stdexcept>

class mmap_array_error : public std::runtime_error {
    public:
        mmap_array_error(const std::ostringstream &ss) : std::runtime_error(ss.str()) {}
};

template <class T>
class mmap_array {
private:
    size_t _size = 0;
    int _fd = -1;
    T *_map = (T *)MAP_FAILED;
public:
    static_assert(4096 % sizeof(T) == 0, "sizeof(T) not page aligned");
    // non-copyable
    mmap_array(const mmap_array&) = delete;
    mmap_array &operator=(const mmap_array &) = delete;

    mmap_array(size_t size) : _size(size) {
        _map = (T *)::mmap(NULL, size * sizeof(T), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (_map == MAP_FAILED) {
            std::ostringstream ss;
            ss << "mmap failed " << sys_errlist[errno];
            throw mmap_array_error(ss);
        }
    }

    mmap_array(const char *filename, size_t size, int flags=0700) : _size(size) {
        _fd = ::open(filename, O_CREAT | O_RDWR, flags);
        if (_fd == -1) {
            std::ostringstream ss;
            ss << "opening " << filename << " failed " << sys_errlist[errno];
            throw mmap_array_error(ss);
        }
        int status = ::posix_fallocate(_fd, 0, size * sizeof(T));
        if (status != 0) {
            std::ostringstream ss;
            ss << "fallocate " << filename << " failed " << sys_errlist[errno];
            throw mmap_array_error(ss);
        }
        _map = (T *)::mmap(NULL, size * sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
        if (_map == MAP_FAILED) {
            close();
            std::ostringstream ss;
            ss << "mmap " << filename << " failed " << sys_errlist[errno];
            throw mmap_array_error(ss);
        }
    }

    mmap_array(mmap_array &&other) {
        std::swap(_size, other._size);
        std::swap(_fd, other._fd);
        std::swap(_map, other._map);
    }

    mmap_array &operator= (mmap_array &&other) {
        if (this != &other) {
            std::swap(_size, other._size);
            std::swap(_fd, other._fd);
            std::swap(_map, other._map);
        }
        return *this;
    }

    T &operator[] (size_t n) const {
        return _map[n];
    }

    void close() {
        if (_map != MAP_FAILED) {
            ::munmap(_map, _size * sizeof(T));
            _map = (T *)MAP_FAILED;
        }

        if (_fd != -1) {
            ::close(_fd);
            _fd = -1;
        }
    }

    size_t size() const { return _size; }

    ~mmap_array() {
        close();
    }
};

#endif

