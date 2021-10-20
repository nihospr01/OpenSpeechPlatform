#ifndef OSP_FILE_RECORD_HPP
#define OSP_FILE_RECORD_HPP
#include <string>
#include <thread>
// #include <OSP/fileio/sema.hpp>
#include <OSP/GarbageCollector/GarbageCollector.hpp>
#include <atomic>
#include <stdio.h>
#include <sys/time.h>

#include "async_write.cpp"

class file_record {
  public:
    std::string record_path;
    std::string file_path;
    async_buf *abuf = nullptr;
    std::ostream *astream;

    void set_params(std::string filename, int record_mode);
    void get_params(int &record_mode);
    void rthma_record(float **in, float **out, const int buf_size);
    file_record(void);
    ~file_record(void);

  private:
    int sock;
    struct record_param_t {
        std::string filename;
        int record_mode;
    };
    std::shared_ptr<record_param_t> currentParam;
    GarbageCollector releasePool;
};

#endif // OSP_FILE_RECORD_HPP
