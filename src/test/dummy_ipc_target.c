#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "test_ipc_spawn_name") == 0) {
        return 42;
    }
    return 1;
}
