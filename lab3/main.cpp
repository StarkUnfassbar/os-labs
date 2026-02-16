#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

#include "process_clone_manager.hpp"

#define SHARED_MEM_NAME "mylab_counter"
#define LOG_FILE "lab.log"

#ifdef _WIN32
#   include <windows.h>
#   include <io.h>
#else
#   include <unistd.h>
#   include <signal.h>
#   include <sys/types.h>
#endif

using namespace std::chrono_literals;


CloneLogger::CloneLogger(const std::string& log_file)
    : log_file_(log_file),
      log_stream_(log_file, std::ios::app),
      shared_mem_(SHARED_MEM_NAME),
      current_pid_(get_current_pid()),
      running_(true)
{
    if(!shared_mem_.IsValid()) {
        std::cout << "Не удалось создать блок памяти!" << std::endl;
        exit(1);
    }

    write_log("Процесс запустился с PID " + std::to_string(current_pid_) + " в " + get_curr_time(false));
}

void CloneLogger::write_log(const std::string& message) {
    if (!log_stream_.is_open()) {
        log_stream_.open(log_file_, std::ios::app);
    }
    log_stream_ << get_curr_time(true) << " | " << current_pid_ 
                << " | " << message << std::endl;
}

CloneLogger::~CloneLogger() {
    running_ = false;
    
    if (timer_thread_.joinable()) timer_thread_.join();
    if (logging_thread_.joinable()) logging_thread_.join();
    if (cloning_thread_.joinable()) cloning_thread_.join();
    if (input_thread_.joinable()) input_thread_.join();
    
    log_stream_.close();
}

void CloneLogger::run(){
    timer_thread_ = std::thread(&CloneLogger::timerThread, this);
    logging_thread_ = std::thread(&CloneLogger::loggingThread, this);
    cloning_thread_ = std::thread(&CloneLogger::cloningThread, this);
    input_thread_ = std::thread(&CloneLogger::processUserInput, this);

    if (timer_thread_.joinable()) timer_thread_.join();
    if (logging_thread_.joinable()) logging_thread_.join();
    if (cloning_thread_.joinable()) cloning_thread_.join();
    if (input_thread_.joinable()) input_thread_.join();
}

void CloneLogger::timerThread() {
    while(running_){
        std::this_thread::sleep_for(300ms);

        shared_mem_.Lock();

        CounterData* data = shared_mem_.Data();
        if(data){ data -> counter++; }

        shared_mem_.Unlock();
    }
}

void CloneLogger::loggingThread() {
    while(running_){
        std::this_thread::sleep_for(1s);

        shared_mem_.Lock();

        CounterData* data = shared_mem_.Data();
        if(data && data -> master_pid == current_pid_){
            write_log("counter " + std::to_string(data -> counter));
        }

        shared_mem_.Unlock();
    }
}

void CloneLogger::cloningThread() {
    while(running_){
        std::this_thread::sleep_for(3s);

        shared_mem_.Lock();
        CounterData* data = shared_mem_.Data();
        if(!data) {
            shared_mem_.Unlock();
            continue;
        }

        if(!data->master_exists || !is_process_running(data->master_pid)){
            data->master_exists = true;
            data->master_pid = current_pid_;
            write_log("Стал мастер-процессом");
        }

        if (data->master_pid != current_pid_) {
            shared_mem_.Unlock();
            continue;
        }

        bool clones_finished = true;
        std::vector<int> active_pids;
        
        for (int pid : clone_pids_) {
            if (is_process_running(pid)) {
                clones_finished = false;
                active_pids.push_back(pid);
            }
        }
        
        clone_pids_ = active_pids;

        if (!clones_finished) {
            write_log("Предыдущие клоны еще работают (PIDs: " + 
                     std::to_string(active_pids[0]) + ", " + 
                     std::to_string(active_pids[1]) + "), пропускаем запуск");
            shared_mem_.Unlock();
            continue;
        }

        write_log("Запускаю новых клонов");
        
        int pid1 = spawn_clone(1);
        if (pid1 > 0) clone_pids_.push_back(pid1);
        
        int pid2 = spawn_clone(2);
        if (pid2 > 0) clone_pids_.push_back(pid2);
        
        last_clone_time_ = std::chrono::steady_clock::now();

        shared_mem_.Unlock();
    }
}

bool CloneLogger::is_process_running(int pid) {
    if (pid <= 0) return false;
    
    #ifdef _WIN32
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (process) {
            DWORD exit_code;
            if (GetExitCodeProcess(process, &exit_code)) {
                CloseHandle(process);
                return exit_code == STILL_ACTIVE;
            }
            CloseHandle(process);
        }
        return false;
    #else
        return kill(pid, 0) == 0;
    #endif
}

void CloneLogger::processUserInput() {
    std::string cmd, arg;

    while (running_ && std::cin >> cmd) {
        if (cmd == "set" && std::cin >> arg) {
            try {
                shared_mem_.Lock();

                CounterData* data = shared_mem_.Data();
                if (data) { data->counter = std::stoi(arg); }

                shared_mem_.Unlock();
            } catch (...) {
                std::cout << "Недопустимое значение counter\n";
            }
        } else if (cmd == "quit") {
            running_ = false;
        }
    }
}


int CloneLogger::spawn_clone(int clone_num) {
    #ifdef _WIN32
        std::string program = "build\\lab.exe";
        std::string args = program + " --clone " + std::to_string(clone_num);
        
        STARTUPINFO si = {sizeof(si)};
        PROCESS_INFORMATION pi;
        
        if (CreateProcess(NULL, const_cast<char*>(args.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            write_log("Started clone " + std::to_string(clone_num) + " with PID " + std::to_string(pi.dwProcessId));
            return pi.dwProcessId;
        }
    #else
        pid_t pid = fork();
        if (pid == 0) {
            // Дочерний процесс
            execl("./build/lab", "lab", "--clone", std::to_string(clone_num).c_str(), NULL);
            exit(1);
        } else if (pid > 0) {
            write_log("Started clone " + std::to_string(clone_num) + " with PID " + std::to_string(pid));
            return pid;
        }
    #endif

    write_log("Failed to spawn clone " + std::to_string(clone_num));
    return -1;
}


void run_clone(int clone_num){
    cpmem::SharedMem<CounterData> shared_mem(SHARED_MEM_NAME);
    if (!shared_mem.IsValid()) {
        std::cerr << "Не удалось получить доступ к общей памяти в копии!" << std::endl;
        return;
    }

    #ifdef _WIN32
        int pid = GetCurrentProcessId();
    #else
        int pid = getpid();
    #endif

    auto log_msg = [&](const std::string& msg) {
        std::ofstream log(LOG_FILE, std::ios::app);
        if (log.is_open()) {
            log << get_curr_time(true) << " | PID: " << pid << " | Clone " << clone_num << " " << msg << std::endl;
            log.flush();
        }
    };

    log_msg("started");

    shared_mem.Lock();
    CounterData* data = shared_mem.Data();
    if (data) {
        if (data->master_exists && !is_process_running(data->master_pid)) {
            data->master_exists = false;
            data->master_pid = 0;
            log_msg("обнаружил мертвого мастера");
        }
        
        if (clone_num == 1) {
            data->counter += 10;
            log_msg("увеличил counter на 10, новое значение: " + std::to_string(data->counter));
        } 
        else if (clone_num == 2) {
            data->counter *= 2;
            log_msg("удвоил counter, новое значение: " + std::to_string(data->counter));
        }
    }
    shared_mem.Unlock();

    if (clone_num == 2) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        shared_mem.Lock();
        CounterData* data = shared_mem.Data();
        if (data) {
            data->counter /= 2;
            log_msg("уменьшил counter в 2 раза, новое значение: " + std::to_string(data->counter));
        }
        shared_mem.Unlock();
    }

    log_msg("finished");
}


int main(int argc, char* argv[]) {
    if(argc > 1) {
        std::string arg = argv[1];
        if (arg == "--clone"){
            int clone_num = std::stoi(argv[2]);
            run_clone(clone_num);
            
            return 0;
        }
    } else {
        CloneLogger app(LOG_FILE);
        app.run();

        return 0;
    }
}