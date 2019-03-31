/** @file libscheduler.h
 */

#ifndef LIBSCHEDULER_H_
#define LIBSCHEDULER_H_
#include "../libpriqueue/libpriqueue.h"

/**
  Constants which represent the different scheduling algorithms
*/
typedef enum {FCFS = 0, SJF, PSJF, PRI, PPRI, RR} scheme_t;

scheme_t current_scheme;
priqueue_t job_queue;
int jobs_count;
int accumulated_wait;
int accumulated_response;
int accumulated_turnaround;

int core_count;
int core_active;
int *core_statuses;
int *core_jobs;

void  scheduler_start_up               (int cores, scheme_t scheme);
int   scheduler_new_job                (int job_number, int time, int running_time, int priority);
int   scheduler_job_finished           (int core_id, int job_number, int time);
int   scheduler_quantum_expired        (int core_id, int time);
float scheduler_average_turnaround_time();
float scheduler_average_waiting_time   ();
float scheduler_average_response_time  ();
void  scheduler_clean_up               ();
int   first_available_core             ();

void  scheduler_show_queue             ();

#endif /* LIBSCHEDULER_H_ */
