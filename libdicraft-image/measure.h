#ifndef MEASURE_H
#define MEASURE_H

#define m_id const char *, const char *
void start_measure(m_id);
void continue_measure(m_id);
void stop_measure(m_id);
void show_measure(m_id);
void stop_show_measure(m_id);
#undef m_id

#define measure_accum(id1,id2,...) do { \
	continue_measure(id1,id2); \
	__VA_ARGS__ \
	stop_measure(id1,id2); \
} while (0)

#define measure_once(id1,id2,...) do { \
	start_measure(id1,id2); \
	__VA_ARGS__ \
	stop_show_measure(id1,id2); \
} while (0)

void measure_clear();

#endif

