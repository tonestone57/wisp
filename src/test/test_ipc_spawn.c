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
    if (argc > 1 && strcmp(argv[1], "--child") == 0) {
        if (argc > 2) {
            printf("Child running with IPC name: %s\n", argv[2]);
            // If the IPC name matches what we expect, exit with 42
            if (strcmp(argv[2], "test_ipc_spawn_name") == 0) {
                return 42;
            }
            return 1; // Wrong IPC name
        }
        return 1; // No IPC name provided
    }

    printf("Running IPC Spawn Test...\n");

    const char *executable = argv[0];
    const char *ipc_name = "test_ipc_spawn_name";

#ifdef _WIN32
    // Windows testing involves creating a wrapper bat because wisp_ipc_spawn doesn't support arg arrays.
    // However, CreateProcess doesn't execute .bat files directly.
    // Let's use cmd.exe directly to run our own executable with arguments.
    // Since wisp_ipc_spawn takes (executable, ipc_name) and formats it as `%s %s`, we can trick it
    // by passing cmd.exe as executable, and `/c "path/to/self --child test_ipc_spawn_name"` as ipc_name.

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "/c \"%s --child %s\"", executable, ipc_name);

    int pid = wisp_ipc_spawn("cmd.exe", cmd);
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

    unlink(script_path);

    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 42);
#endif

    printf("IPC Spawn Test: ALL PASSED\n");
    return 0;
}
