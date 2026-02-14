#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/wait.h>
    #include <unistd.h>
    #include <signal.h>
#endif

enum class ListCommand {
    PING,
    TIMEOUT,
    IPCONFIG,
    DIR
};

struct ProcessHandle {
    #ifdef _WIN32
        PROCESS_INFORMATION pi;
    #else
        pid_t pid;
    #endif

    bool finished;
    int exit_code;
};

int run_command(ListCommand cmd, ProcessHandle* handle);
int wait_for_command(ProcessHandle* handle);
const char* get_name_command(ListCommand cmd);