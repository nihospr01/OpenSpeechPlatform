#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include  <vector>
#include <stdlib.h>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <chrono>
#include <unistd.h>
#include <thread>
#include "sema.hpp"

/*
This is a test program to measure the time for a thread to signal another thread
in another cpu and for a reply to be received.  It is critical that this be minimized,
particularly for the PCD where the only way to get decent performance is to split
the workload between multiple threads in different cpus.  This is a simplified version
of the code in OspProcess::process_channels()
*/

/**
 * @brief Set thread name, affinity and priority
 *
 * Only works for Linux.
 * 
 * @param thread_name If not NULL, will set the name of the current thread.
 * @param cpu_num If > 0, will set the cpu affinity. Should be in range of 0 to 3.
 * @param policy If > 0 will set the scheduling policy
 * @param prio priority
 * 
 * @note If there are at least 6 cpus, the affinity will be
 * set to 2*cpu_num, to avoid scheduling processes in the same logical core.
 */
 
void set_cpu_pri(const char *thread_name, int cpu_num, int policy, int prio) {
#ifdef __linux__
    int rc;

    if (thread_name != NULL) {
        rc = pthread_setname_np(pthread_self(), thread_name);
        if (rc != 0)
            std::cerr << "pthread_setname_np failed" << std::endl;
    }

    if (cpu_num >= 0) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);

        unsigned num_cpus = std::thread::hardware_concurrency();
        if (num_cpus < 6)
            CPU_SET(cpu_num, &cpuset);
        else
            CPU_SET(2 * cpu_num, &cpuset);  // skip odd because hyperthreading

        rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << std::endl;
        }
    }

    if (policy >= 0) {
        struct sched_param param;

        if (policy == SCHED_OTHER && prio != 0)
            std::cerr << "SCHED_OTHER only supports priority = 0" << std::endl;

        param.sched_priority = prio;
        if (pthread_setschedparam(pthread_self(), policy, &param) != 0)
            std::cerr << __func__ << " : pthread_setschedparam failed" << std::endl;
    }
#endif  // __linux__
}

class OspProcess {
   public:
    OspProcess();
    ~OspProcess();
    void process_channels(int channel);
    void process();

   private:
    rk_sema *thread_mutex[2], *finish_sema[2];
    std::thread *proc_chan_thread[2];
};

void OspProcess::process_channels(int channel) {
    do {
        rk_sema_wait(thread_mutex[channel]);
        // do something
        usleep(500);
        rk_sema_post(finish_sema[channel]);
    } while (1);
};

OspProcess::OspProcess() {
    for (int ch = 0; ch < 2; ch++) {
        thread_mutex[ch] = new rk_sema;
        rk_sema_init(thread_mutex[ch], 0);
        finish_sema[ch] = new rk_sema;
        rk_sema_init(finish_sema[ch], 0);
    }

    for (int channel = 0; channel < 2; channel++) {
        proc_chan_thread[channel] = new std::thread(&OspProcess::process_channels, this, channel);
        struct sched_param param;
        param.sched_priority = 90;
        if (pthread_setschedparam(proc_chan_thread[channel]->native_handle(), SCHED_FIFO, &param))
            std::cerr << __func__ << "pthread_setschedparam failed" << std::endl;
#ifdef __linux__
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
#ifdef __ARM_ARCH
        CPU_SET(channel + 1, &cpuset);
#else
        unsigned num_cpus = std::thread::hardware_concurrency();
        if (num_cpus < 6)
            CPU_SET(channel + 1, &cpuset);
        else
            CPU_SET(2 * channel + 2, &cpuset);  // skip odd because hyperthreading
#endif
        int rc = pthread_setaffinity_np(proc_chan_thread[channel]->native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
        char name[16];
        sprintf(name, "OSP: Chan %d", channel);
        rc = pthread_setname_np(proc_chan_thread[channel]->native_handle(), name);
        if (rc != 0)
            std::cerr << "pthread_setname_np failed" << std::endl;
#endif
    }
}

OspProcess::~OspProcess() {}

void OspProcess::process() {
    for (int channel = 0; channel < 2; channel++) {
        rk_sema_post(thread_mutex[channel]);
    }
    for (int channel = 0; channel < 2; channel++) {
        rk_sema_wait(finish_sema[channel]);
    }
}

int main()
{
    set_cpu_pri("OSP: AudioCB", 3, SCHED_FIFO, 3);
    OspProcess *osp = new OspProcess();
    
    set_cpu_pri("OSP", 0, SCHED_OTHER, 0);
    
    auto start = std::chrono::steady_clock::now();
    for (int i=0; i < 10000; i++) {
        osp->process();
        usleep(1000);
    }
    auto end = std::chrono::steady_clock::now();
    auto ttime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto expected_time = 10000 * 1500;
    ttime -= expected_time;
    ttime /= 10000;
    std::cout << "Latency in us: " << ttime << std::endl;

    delete osp;
}
