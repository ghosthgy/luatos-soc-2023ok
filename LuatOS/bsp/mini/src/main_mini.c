
#include <stdio.h>

#include "luat_base.h"
#include "luat_malloc.h"
#include "luat_msgbus.h"
#include "luat_fs.h"
#include <stdlib.h>

#include "bget.h"

#define LUAT_LOG_TAG "main"
#include "luat_log.h"

#define LUAT_HEAP_SIZE (1024*1024)
uint8_t luavm_heap[LUAT_HEAP_SIZE] = {0};

int cmdline_argc;
char** cmdline_argv;

int lua_main (int argc, char **argv);

void luat_log_init_win32(void);
void luat_uart_initial_win32(void);

// boot
int main(int argc, char** argv) {   
    luat_log_init_win32();
    bpool(luavm_heap, LUAT_HEAP_SIZE);
    luat_fs_init();
#ifdef LUAT_USE_LUAC
    extern int luac_main(int argc, char* argv[]);
    luac_main(argc, argv);
#else
    cmdline_argc = argc;
    cmdline_argv = argv;
    if (cmdline_argc > 1) {
        size_t len = strlen(cmdline_argv[1]);
        if (cmdline_argv[1][0] != '-') {
            if (cmdline_argv[1][len - 1] == '/' || cmdline_argv[1][len - 1] == '\\') {
                printf("chdir %s %d\n", cmdline_argv[1], chdir(cmdline_argv[1]));
                cmdline_argc = 1;
            }
        }
    }
    lua_main(cmdline_argc, cmdline_argv);
#endif
    return 0;
}
