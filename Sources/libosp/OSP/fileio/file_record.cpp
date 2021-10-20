//
// Martin Hunt 17 Feb 2021
//

#include <assert.h>
#include <iostream>
#include <memory.h>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "file_record.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

// constructor
file_record::file_record() {
    char *rpath = getenv("OSP_REC");
    if (!rpath) {
        rpath = getenv("OSP_MEDIA");
        if (!rpath) {
            std::cout << "ERROR: OSP_REC and OSP_MEDIA not set.  No path for storing recordings." << std::endl;
            return;
        }
    }
    record_path = rpath;
    std::cout << "Record Path=" << record_path << std::endl;

    std::shared_ptr<record_param_t> data_next = std::make_shared<record_param_t>();
    data_next->filename = "recording";
    data_next->record_mode = 0;
    sock = -1;
    releasePool.add(data_next);
    std::atomic_store(&currentParam, data_next);
    releasePool.release();
}
std::vector<std::string> tokenize(std::string s, std::string del) {
    std::vector<std::string> toks{};
    int start = 0;
    int end = s.find(del);
    while (end != -1) {
        toks.push_back(s.substr(start, end - start));
        // std::cout << s.substr(start, end - start) << std::endl;
        start = end + del.size();
        end = s.find(del, start);
    }
    toks.push_back(s.substr(start, end - start));
    return toks;
    // std::cout << s.substr(start, end - start);
}

void file_record::set_params(std::string filename, int record_mode) {
    std::cout << "set_params audio_rfile=" << filename << std::endl;
    std::cout << "set_params audio_record=" << record_mode << std::endl;

    in_addr_t ip_addr = 0;
    int port = 0;

    std::shared_ptr<record_param_t> data_next = std::make_shared<record_param_t>();
    if (record_mode == 0) {
        if (abuf) {
            // stop any old recording threads
            delete astream;
            delete abuf;
            abuf = nullptr;
        }
        if (sock >= 0) {
            // close open
            close(sock);
            sock = -1;
        }
        data_next->record_mode = 0;
        releasePool.add(data_next);
        std::atomic_store(&currentParam, data_next);
        releasePool.release();
        return;
    }

    // is the filename really a tcp address?
    std::vector<std::string> toks = tokenize(filename, "://");
    if (toks.size() == 2 and toks[0] == "tcp") {
        toks = tokenize(toks[1], ":");
        // std::cout << "Size:" << toks.size() << std::endl;
        // for (auto it = toks.begin(); it != toks.end(); it++)
        //     std::cout << *it << " ";
        // std::cout << std::endl;
        if (toks.size() == 2) {
            ip_addr = inet_addr(toks[0].c_str());
            if (ip_addr == INADDR_NONE) {
                std::cout << "bad tcp address: " << toks[0] << std::endl;
            } else {
                errno = 0;
                port = strtoul(toks[1].c_str(), NULL, 10);
                if (errno) {
                    std::cout << "invalid port number: " << toks[1] << std::endl;
                    ip_addr = 0;
                }
            }
        }
    }

    if (ip_addr == 0) {
        if (filename[0] != '/') {
            file_path = record_path + "/" + filename;
        } else {
            file_path = filename;
        }
        std::cout << "WRITING to " << file_path << std::endl;
        if (!abuf) {
            // start an async thread for output
#ifdef __linux__
            pthread_setname_np(pthread_self(), "OSP: record");
#else
            pthread_setname_np("OSP: record");
#endif
            abuf = new async_buf(file_path);
            astream = new std::ostream(abuf);
#ifdef __linux__
            pthread_setname_np(pthread_self(), "OSP");
#else
            pthread_setname_np("OSP");
#endif
        }
    }

    if (ip_addr) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in servaddr;
        // assign IP, PORT
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = ip_addr;
        servaddr.sin_port = htons(port);
        // connect the client socket to server socket
        if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
            std::cout << "Connection to " << inet_ntoa(servaddr.sin_addr) << ":" << port << " failed" << std::endl;
            close(sock);
            sock = -1;
        } else
            std::cout << "Connected to " << inet_ntoa(servaddr.sin_addr) << ":" << port << std::endl;
    } else
        data_next->filename = filename;

    data_next->record_mode = record_mode;
    releasePool.add(data_next);
    std::atomic_store(&currentParam, data_next);
    releasePool.release();
}

void file_record::get_params(int &record_mode) {
    std::shared_ptr<record_param_t> data_current = std::atomic_load(&currentParam);
    record_mode = data_current->record_mode;
}

void file_record::rthma_record(float **in, float **out, const int buf_size) {
    assert(buf_size == 48);
    static float buf[48 * 2];

    std::shared_ptr<record_param_t> data_current = std::atomic_load(&currentParam);
    if (data_current->record_mode == 0)
        return;

    float *ptr = buf;
    float *left = out[0];
    float *right = out[1];

    // write data, alternate left then right
    for (int i = 0; i < buf_size; i++) {
        *ptr++ = *left++;
        *ptr++ = *right++;
    }

    if (sock >= 0) {
        // std::cout << "streaming " << buf_size * sizeof(float) * 2 << " bytes\n";
        write(sock, buf, buf_size * sizeof(float) * 2);
    } else {
        // std::cout << "writing " << buf_size * sizeof(float) * 2 << " bytes\n";
        astream->write((const char *)buf, buf_size * sizeof(float) * 2);
    }
}

file_record::~file_record() {
    if (sock >= 0)
        close(sock);

    if (abuf) {
        delete astream;
        delete abuf;
    }
    releasePool.release();
}
