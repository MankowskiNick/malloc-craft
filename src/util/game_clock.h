#ifndef GAME_CLOCK_H
#define GAME_CLOCK_H

typedef struct {
	long long start;
	char* id;
} g_clock;

g_clock create_clock(char* name);
void reset_clock(g_clock* clock);
long get_time(g_clock* clock);

#endif 
