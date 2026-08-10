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

int main(int argc, char **argv) {
    printf("Running IPC Spawn Test...\n");

    const char *ipc_name = "test_ipc_spawn_name";

#ifdef _WIN32
    // Windows testing involves executing the dummy IPC target instead of creating a .bat file,
    // to work correctly with CreateProcess logic in wisp_ipc_spawn.
    char target_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, target_path, sizeof(target_path)) > 0) {
        char *last_backslash = strrchr(target_path, '\\');
        if (last_backslash) {
            *last_backslash = '\0';
            strncat(target_path, "\\dummy_ipc_target.exe", sizeof(target_path) - strlen(target_path) - 1);
        }
    } else {
        strcpy(target_path, "dummy_ipc_target.exe");
    }

    int pid = wisp_ipc_spawn(target_path, ipc_name);
    assert(pid > 0);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
    assert(hProcess != NULL);

    WaitForSingleObject(hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(hProcess, &exit_code);
    assert(exit_code == 42);
    CloseHandle(hProcess);
#else
    char script_path[] = "/tmp/wisp_spawn_test_XXXXXX";
    int fd = mkstemp(script_path);
    assert(fd != -1);

    const char *script_content = "#!/bin/sh\n"
                                 "if [ \"$1\" = \"test_ipc_spawn_name\" ]; then\n"
                                 "  exit 42\n"
                                 "else\n"
                                 "  exit 1\n"
                                 "fi\n";
    ssize_t written = write(fd, script_content, strlen(script_content));
    assert(written == (ssize_t)strlen(script_content));
    close(fd);

    chmod(script_path, 0755);

    int pid = wisp_ipc_spawn(script_path, ipc_name);
    assert(pid > 0);

    int status;
    waitpid(pid, &status, 0);

    unlink(script_path); // Cleanup immediately

    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 42);
#endif

    printf("IPC Spawn Test: ALL PASSED\n");
    return 0;
}
