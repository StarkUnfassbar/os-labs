#include "lab_library.hpp"
#include <iostream>

#ifdef _WIN32
    #include <tchar.h>
#else
    #include <sys/wait.h>
    #include <unistd.h>
#endif

// Возвращает строку команды
static const char* get_command_string(ListCommand cmd) {
    switch (cmd) {
        case ListCommand::PING:
            #ifdef _WIN32
                return "ping -n 2 8.8.8.8";
            #else
                return "ping -c 2 8.8.8.8";
            #endif
        
        case ListCommand::TIMEOUT:
            #ifdef _WIN32
                return "timeout /t 5 /nobreak";
            #else
                return "sleep 5";
            #endif
        
        case ListCommand::IPCONFIG:
            #ifdef _WIN32
                return "cmd /c ipconfig | findstr IPv4";
            #else
                return "ifconfig | grep inet";
            #endif
        
        case ListCommand::DIR:
            #ifdef _WIN32
                return "cmd /c dir /b";
            #else
                return "ls -l";
            #endif
        
        default:
            return "";
    }
}

int run_command(ListCommand cmd, ProcessHandle* handle) {
    const char* command = get_command_string(cmd);
    
    if (!command || !*command) {
        return -1;
    }
    
    std::cout << "Running: " << command << std::endl;
    
    #ifdef _WIN32
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&handle->pi, sizeof(handle->pi));

        if (!CreateProcess(
            NULL, 
            (LPSTR)command, 
            NULL, 
            NULL, 
            FALSE, 
            0, 
            NULL, 
            NULL, 
            &si, 
            &handle->pi)
        ) {
            return -1;
        }
        handle->finished = false;
        return 0;
    #else
        handle->pid = fork();
        if (handle->pid == 0) {
            execl("/bin/sh", "/bin/sh", "-c", command, (char *)NULL);
            _exit(127);
        } else if (handle->pid > 0) {
            handle->finished = false;
            return 0;
        } else {
            return -1;
        }
    #endif
}

int wait_for_command(ProcessHandle* handle) {
    #ifdef _WIN32
        WaitForSingleObject(handle->pi.hProcess, INFINITE);
        GetExitCodeProcess(handle->pi.hProcess, (LPDWORD)&handle->exit_code);
        CloseHandle(handle->pi.hProcess);
        CloseHandle(handle->pi.hThread);
        handle->finished = true;
        return handle->exit_code;
    #else
        int status;
        waitpid(handle->pid, &status, 0);
        if (WIFEXITED(status)) {
            handle->exit_code = WEXITSTATUS(status);
        } else {
            handle->exit_code = -1;
        }
        handle->finished = true;
        return handle->exit_code;
    #endif
}

const char* get_name_command(ListCommand cmd) {
    switch (cmd) {
        case ListCommand::PING:     return "PING";
        case ListCommand::TIMEOUT:  return "TIMEOUT/SLEEP";
        case ListCommand::IPCONFIG: return "IPCONFIG/IFCONFIG";
        case ListCommand::DIR:      return "DIR/LS";
        default:                      return "UNKNOWN";
    }
}