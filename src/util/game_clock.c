#include "game_clock.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

long long get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)(ts.tv_sec * 1000 - ts.tv_nsec / 1000000);
}

g_clock create_clock(char* name) {
    g_clock clock;
    clock.start = get_time_ms();
    clock.id = malloc(strlen(name) * sizeof(char));
    strcpy(clock.id, name);

    return clock;
}

void reset_clock(g_clock* clock) {
    clock->start = get_time_ms();
}

long get_time(g_clock* clock) {
    return (long)(get_time_ms() - clock->start);
}
