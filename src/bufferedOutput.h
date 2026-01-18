#include <iostream>
#include <vector>

class BufferedOutput {
public:
    explicit BufferedOutput(std::ostream& stream)
        : stream_(stream), offset_(0) {
        buffer_.resize(BUFFER_SIZE);
    }

    ~BufferedOutput() {
        flush();
    }

    void write(const uint8_t* data, size_t size) {
        if (offset_ + size > BUFFER_SIZE) {
            flush();
        }
        memcpy(buffer_.data() + offset_, data, size);
        offset_ += size;
    }

    void flush() {
        if (offset_ > 0) {
            stream_.write(reinterpret_cast<const char*>(buffer_.data()), offset_);
            offset_ = 0;
        }
    }

private:
    static constexpr size_t BUFFER_SIZE = 512 * 1024; // 512KiB, not too big, not too small
    std::ostream& stream_;
    std::vector<uint8_t> buffer_;
    size_t offset_;
};
