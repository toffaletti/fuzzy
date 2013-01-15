#ifndef MMAP_ARRAY_HH
#define MMAP_ARRAY_HH
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <stdexcept>

template <class T>
class mmap_array {
private:
    size_t _size;
    int _fd=-1;
    T *_map = (T *)MAP_FAILED;
public:
    static_assert(4096 % sizeof(T) == 0, "sizeof(T) not page aligned");
    // non-copyable
    mmap_array(const mmap_array&) = delete;
    mmap_array &operator=(const mmap_array &) = delete;

    mmap_array(size_t size) : _size(size) {
        _map = (T *)::mmap(NULL, size * sizeof(T), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (_map == MAP_FAILED) {
            throw std::runtime_error(std::string("could not mmap ") + 
                    sys_errlist[errno]);
        }
    }

    mmap_array(const char *filename, size_t size, int flags=0700) : _size(size) {
        _fd = ::open(filename, O_CREAT | O_RDWR, flags);
        if (_fd == -1) {
            throw std::runtime_error(std::string("could not open ") + filename);
        }
        int status = ::posix_fallocate(_fd, 0, size * sizeof(T));
        if (status != 0) {
            throw std::runtime_error(std::string("could not allocate ") + filename);
        }
        _map = (T *)::mmap(NULL, size * sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
        if (_map == MAP_FAILED) {
            close();
            throw std::runtime_error(std::string("could not mmap ") + filename);
        }
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

