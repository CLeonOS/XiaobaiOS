#include <clks/types.h>
#include <clks/userland.h>

clks_bool clks_userland_init(void) {
    return CLKS_TRUE;
}

void clks_userland_tick(u64 tick) {
    (void)tick;
}

clks_bool clks_userland_shell_ready(void) {
    return CLKS_FALSE;
}

clks_bool clks_userland_shell_exec_requested(void) {
    return CLKS_FALSE;
}

clks_bool clks_userland_shell_auto_exec_enabled(void) {
    return CLKS_FALSE;
}

u64 clks_userland_launch_attempts(void) {
    return 0ULL;
}

u64 clks_userland_launch_success(void) {
    return 0ULL;
}

u64 clks_userland_launch_failures(void) {
    return 0ULL;
}
