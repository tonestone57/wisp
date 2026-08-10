#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#endif

#include "wisp/utils/ipc.h"

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#ifndef _WIN32
ssize_t get_self_path(char *out_path, size_t out_len) {
    ssize_t len = readlink("/proc/self/exe", out_path, out_len - 1);
    if (len != -1) {
        out_path[len] = '\0';
    }
    return len;
}
#endif

int main(int argc, char **argv) {
    printf("Running IPC Spawn Test...\n");

    const char *ipc_name = "test_ipc_spawn_name";

    char target_path[MAX_PATH];

#ifdef _WIN32
    if (GetModuleFileNameA(NULL, target_path, sizeof(target_path)) > 0) {
        char *last_backslash = strrchr(target_path, '\\');
        if (last_backslash) {
            *last_backslash = '\0';
            strncat(target_path, "\\dummy_ipc_target.exe", sizeof(target_path) - strlen(target_path) - 1);
        }
    } else {
        strcpy(target_path, "dummy_ipc_target.exe");
    }
#else
    if (get_self_path(target_path, sizeof(target_path)) > 0) {
        char *last_slash = strrchr(target_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            strncat(target_path, "/dummy_ipc_target", sizeof(target_path) - strlen(target_path) - 1);
        }
    } else {
        strcpy(target_path, "./dummy_ipc_target");
    }
#endif

    int pid = wisp_ipc_spawn(target_path, ipc_name);
    assert(pid > 0);

#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
    assert(hProcess != NULL);

    WaitForSingleObject(hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(hProcess, &exit_code);
    assert(exit_code == 42);
    CloseHandle(hProcess);
#else
    int status;
    waitpid(pid, &status, 0);

    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 42);
#endif

    printf("IPC Spawn Test: ALL PASSED\n");
    return 0;
}
