#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define id_proto const char *name1, const char *name2
#define id_pass name1, name2

struct measure_record {
	const char *name1, *name2;
	uint64_t start;
	uint64_t accum;
};

static uint64_t getus() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static struct measure_record **records = NULL;
static int record_len = 0;

/* find index of measurement record by given name.
 * if it doesn't exist, -1
 */
static int find_record(id_proto) {
	for (int i = 0; i < record_len; i++) {
		if (records[i] != NULL
			&& !strcmp(name1, records[i]->name1)
			&& !strcmp(name2, records[i]->name2)
		) {
			return i;
		}
	}
	return -1;
}

/* find a measurement record by name,
 * creating it if necessary.
 */
static struct measure_record *give_record(id_proto) {
	/* try and find it in list */
	int i_found = find_record(id_pass);
	if (i_found != -1) {
		return records[i_found];
	}

	/* not found, create it. just append it to the list.
	 * by just enlarging the list, it also creates the list, if necessary.
	 */
	record_len++;
	records = realloc(records, record_len * sizeof(struct measure_record *));

	/* create the entry itself */
	struct measure_record *r = malloc(sizeof(struct measure_record));
	r->name1 = name1; r->name2 = name2;
	r->accum = 0;

	records[record_len-1] = r;
	return r;
}

void start_measure(id_proto) {
	struct measure_record *r = give_record(id_pass);
	r->accum = 0;
	r->start = getus();
}

void continue_measure(id_proto) {
	struct measure_record *r = give_record(id_pass);
	r->start = getus();
}

void stop_measure(id_proto) {
	uint64_t st = getus();
	struct measure_record *r = give_record(id_pass);
	r->accum += (st - r->start);
}

void show_measure(id_proto) {
	struct measure_record *r = give_record(id_pass);
	fprintf(stderr, "[%s,%s] took %8.5fs\n", id_pass, r->accum / 1000000.);
}

void stop_show_measure(id_proto) {
	stop_measure(id_pass);
	show_measure(id_pass);
}

void measure_clear() {
	for (int i = 0; i < record_len; i++) {
		if (records[i] != NULL) {
			free(records[i]);
		}
	}
	free(records);
	record_len = 0;
	records = NULL;
}
