#pragma once

#include "cross_process_mem.hpp"
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>

#ifndef CLONGER_H
#define CLONGER_H

#ifdef _WIN32
#   include <windows.h>
#   include <process.h>
#   include <tlhelp32.h>
#else
#   include <sys/wait.h>
#   include <unistd.h>
#   include <signal.h>
#   include <sys/types.h>
#endif

struct CounterData {
    int counter;
    bool master_exists;
    int master_pid;
    
    CounterData() : counter(0), master_exists(false), master_pid(0) {}
};

inline int get_current_pid() {
    #ifdef _WIN32
        return GetCurrentProcessId();
    #else
        return getpid();
    #endif
}

inline std::string get_curr_time(bool with_ms = false) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream strstr;
    strstr << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    if (with_ms) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        strstr << "." << std::setfill('0') << std::setw(3) << ms.count();
    }
    
    return strstr.str();
}

class CloneLogger {
    public:
        CloneLogger(const std::string& log_file);
        ~CloneLogger();

        void run();

    private:
        void write_log(const std::string& message);
        void timerThread();
        void loggingThread();
        void cloningThread();
        void processUserInput();
        bool is_process_running(int pid);
        int spawn_clone(int clone_num);
        
        std::string log_file_;
        std::ofstream log_stream_;
        cpmem::SharedMem<CounterData> shared_mem_;
        std::atomic<bool> running_;
        std::thread timer_thread_;
        std::thread logging_thread_;
        std::thread cloning_thread_;
        std::thread input_thread_;
        const int current_pid_;
        
        std::vector<int> clone_pids_;
        std::chrono::steady_clock::time_point last_clone_time_;
};

#endif