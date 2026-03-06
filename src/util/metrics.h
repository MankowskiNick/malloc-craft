#ifndef METRICS_H
#define METRICS_H

#define FPS_BUFFER_SIZE 60

void init_metrics(void);
float get_fps(void);
void update_fps(void);
int get_delta_ms(void);

#endif