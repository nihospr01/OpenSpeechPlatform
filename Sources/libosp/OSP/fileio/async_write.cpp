#include <condition_variable>
#include <fstream>
#include <mutex>
#include <queue>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

/*
 * async c++ output stream
 */

struct async_buf : std::streambuf {
    std::ofstream out;
    std::mutex mutex;
    std::condition_variable condition;
    std::queue<std::vector<char>> queue;
    std::vector<char> buffer;
    bool done;
    std::thread thread;

    void worker() {
        bool local_done(false);
        std::vector<char> buf;
        while (!local_done) {
            {
                std::unique_lock<std::mutex> guard(this->mutex);
                this->condition.wait(guard, [this]() { return !this->queue.empty() || this->done; });
                if (!this->queue.empty()) {
                    buf.swap(queue.front());
                    queue.pop();
                }
                local_done = this->queue.empty() && this->done;
            }
            if (!buf.empty()) {
                out.write(buf.data(), std::streamsize(buf.size()));
                buf.clear();
            }
        }
        out.flush();
    }

   public:
    async_buf(std::string const &name) : out(name), buffer(128), done(false), thread(&async_buf::worker, this) {
        this->setp(this->buffer.data(), this->buffer.data() + this->buffer.size() - 1);
    }
    ~async_buf() {
        std::cout << "Deleting async_buf" << std::endl;
        std::unique_lock<std::mutex>(this->mutex), (this->done = true);
        this->condition.notify_one();
        this->thread.join();
    }
    int overflow(int c) {
        if (c != std::char_traits<char>::eof()) {
            *this->pptr() = std::char_traits<char>::to_char_type(c);
            this->pbump(1);
        }
        return this->sync() != -1 ? std::char_traits<char>::not_eof(c) : std::char_traits<char>::eof();
    }
    int sync() {
        if (this->pbase() != this->pptr()) {
            this->buffer.resize(std::size_t(this->pptr() - this->pbase()));
            {
                std::unique_lock<std::mutex> guard(this->mutex);
                this->queue.push(std::move(this->buffer));
            }
            this->condition.notify_one();
            this->buffer = std::vector<char>(128);
            this->setp(this->buffer.data(), this->buffer.data() + this->buffer.size() - 1);
        }
        return 0;
    }
};

// int main() {
//     pthread_setname_np(pthread_self(), "ABUF");
//     async_buf sbuf("async.out");
//     pthread_setname_np(pthread_self(), "async");

//     std::ostream astream(&sbuf);
//     std::ifstream in("async_stream.cpp");
//     for (std::string line; std::getline(in, line);) {
//         astream << line << '\n' << std::flush;
//     }
// }
