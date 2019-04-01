/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements.
*/
typedef struct _job_t
{
  int priority;
  int arrival_time;
  int job_id;
  int running_time;
  int running;
  int start_time;
  int last_known_active_time;
} job_t;

int job_compare(const void* j1, const void* j2)
{
  return ((job_t*) j1)->priority - ((job_t*) j2)->priority;
}
int job_compare_sjf(const void* j1, const void* j2)
{
  job_t* job2 = (job_t*) j2;
  if(job2->running == 1)
    return 1;
  return ((job_t*) j1)->priority - ((job_t*) j2)->priority;
}

/**
  Initalizes the scheduler.

  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
  core_count = cores;
  core_statuses = malloc( core_count * sizeof(int));
  core_jobs = malloc( core_count * sizeof(int));
  for(int i=0; i<core_count; i++) {
    core_statuses[i] = 0;
    core_jobs[i] = -1;
  }

  current_scheme = scheme;
  if(scheme == SJF || scheme == PRI)
    priqueue_init(&job_queue, job_compare_sjf);
  else
    priqueue_init(&job_queue, job_compare);

  jobs_count = 0;
  accumulated_wait = 0;
  accumulated_turnaround = 0;
  accumulated_response = 0;
}


/**
  Called when a new job arrives.

  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made.

 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
  jobs_count++;
  //initializes new job
  job_t* new_job = (job_t*) malloc(sizeof(job_t));
  new_job->job_id = job_number;
  new_job->running_time = running_time;
  new_job->arrival_time = time;
  new_job->running = 0;
  new_job->start_time = -1;

  int job_index;
  //decides on priority of new job based on scheme
  switch (current_scheme)
  {
    case FCFS:
      new_job->priority = 1;     //everything has equal priority, so new elements enter at back of queue
      //add job to queue
      job_index = priqueue_offer(&job_queue, new_job);
      //if new_job is at front of queue, it should be running
      if(job_index < core_count)
      {
        new_job->running = 1;
        new_job->start_time = time;
        int starting_core = first_available_core();
        core_jobs[starting_core] = new_job->job_id;
        return starting_core;
      }
      break;

    case SJF:
      new_job->priority = new_job->running_time;
      //add job to queue
      job_index = priqueue_offer(&job_queue, new_job);
      //if new_job is at front of queue, it should be running
      if(job_index < core_count)
      {
        new_job->running = 1;
        new_job->start_time = time;
        int starting_core = first_available_core();
        core_jobs[starting_core] = new_job->job_id;
        return starting_core;
      }
      break;

    case PSJF:
      new_job->priority = new_job->running_time;     //job's priority is its runtime
      //update runtime of job currently running if queue is not empty
      for(int i=0; i<priqueue_size(&job_queue); i++) {
        if(i==core_count)
          break;

        job_t* running_job = (job_t*)priqueue_at(&job_queue, i);
        running_job->priority = running_job->priority - (time - running_job->last_known_active_time);
        running_job->last_known_active_time = time;
      }
      //add job to queue
      job_index = priqueue_offer(&job_queue, new_job);
      //if new_job is at front of queue, it should be running
      if(job_index < core_count)
      {
        new_job->start_time = time;
        new_job->last_known_active_time = time;
        new_job->running = 1;
        new_job->start_time = time;
        int starting_core;
        if(priqueue_size(&job_queue) > core_count)
        {
          job_t* old_job = (job_t*)priqueue_at(&job_queue, core_count);
          old_job->running = 0;
          if(old_job->start_time == time)
          {
            old_job->start_time = -1;
          }
          for(int i=0;i<core_count;i++)
          {
            if(core_jobs[i] == old_job->job_id)
            {
              starting_core = i;
              break;
            }
          }
        }
        else
          starting_core = first_available_core();
        core_jobs[starting_core] = new_job->job_id;
        return starting_core;
      }
      break;

    case PRI:
      new_job->priority = priority;
      //add job to queue
      job_index = priqueue_offer(&job_queue, new_job);
      //if new_job is at front of queue, it should be running
      if(job_index < core_count)
      {
        new_job->running = 1;
        new_job->start_time = time;
        int starting_core = first_available_core();
        core_jobs[starting_core] = new_job->job_id;
        return starting_core;
      }
      break;

    case PPRI:
      new_job->priority = priority;
      //add job to queue
      job_index = priqueue_offer(&job_queue, new_job);
      //if new_job is at front of queue, it should be running
      if(job_index < core_count)
      {
        new_job->running = 1;
        new_job->start_time = time;
        int starting_core;
        if(priqueue_size(&job_queue) > core_count)
        {
          job_t* old_job = (job_t*)priqueue_at(&job_queue, core_count);
          old_job->running = 0;
          if(old_job->start_time == time)
          {
            old_job->start_time = -1;
          }
          for(int i=0;i<core_count;i++)
          {
            if(core_jobs[i] == old_job->job_id)
            {
              starting_core = i;
              break;
            }
          }
        }
        else
          starting_core = first_available_core();
        core_jobs[starting_core] = new_job->job_id;
        return starting_core;
      }
      break;

    case RR:
      new_job->priority = 1;     //everything has equal priority, so new elements enter at back of queue
      //add job to queue
      job_index = priqueue_offer(&job_queue, new_job);
      //if new_job is at front of queue, it should be running
      if(job_index < core_count)
      {
        new_job->running = 1;
        new_job->start_time = time;
        int starting_core = first_available_core();
        core_jobs[starting_core] = new_job->job_id;
        return starting_core;
      }
      break;
  }
  return -1;
}

int first_available_core() {
  int core = -1;
  for(int i=0; i<core_count; i++) {
    if (core_statuses[i] == 0) {
      core = i;
      break;
    }
  }
  if (core > -1)
    core_statuses[core] = 1;
  return core;
}

/**
  Called when a job has completed execution.

  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
  if(current_scheme == PSJF){
    for(int i=0; i<priqueue_size(&job_queue); i++) {
      if(i==core_count)
        break;

      job_t* running_job = (job_t*)priqueue_at(&job_queue, i);
      running_job->priority = running_job->priority - (time - running_job->last_known_active_time);
      running_job->last_known_active_time = time;
    }
  }
  for(int i=0; i<priqueue_size(&job_queue); i++) {
    job_t * finished_job = (job_t*)priqueue_at(&job_queue, i);
    if (finished_job->job_id == job_number) {
      accumulated_wait += (time - finished_job->running_time - finished_job->arrival_time);
      accumulated_response += (finished_job->start_time - finished_job->arrival_time);
      accumulated_turnaround += (time - finished_job->arrival_time);
      priqueue_remove(&job_queue, finished_job);
    }

  }

  for(int i=0; i<priqueue_size(&job_queue); i++) {
    job_t * new_running_job = (job_t*)priqueue_at(&job_queue, i);
    // printf("JOB : %d", i);
    // printf("Running : %d", j->running);
    if (new_running_job->running == 0) {
      if(new_running_job->start_time == -1)
        new_running_job->start_time = time;
      new_running_job->last_known_active_time = time;
      new_running_job->running = 1;
      core_jobs[core_id] = new_running_job->job_id;
      return new_running_job->job_id;
      break;
    }
  }
  core_statuses[core_id] = 0;
  core_jobs[core_id] = -1;
  return -1;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.

  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
  for(int i=0; i<priqueue_size(&job_queue); i++) {
  job_t* front = (job_t*) priqueue_at(&job_queue, i);
    if (front->job_id == core_jobs[core_id]){
      priqueue_remove_at(&job_queue, i);
      front->running = 0;
      priqueue_offer(&job_queue, front);
    }
  }
  for(int i=0; i<priqueue_size(&job_queue); i++) {
    job_t * next_job = (job_t*)priqueue_at(&job_queue, i);
    if (next_job->running == 0) {
      next_job->running = 1;
      if(next_job->start_time == -1)
        next_job->start_time = time;
      core_jobs[core_id] = next_job->job_id;
    	return next_job->job_id;
    }
  }
  core_statuses[core_id] = 0;
  core_jobs[core_id] = -1;
  return -1;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	return (float)accumulated_wait/(float)jobs_count;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	return (float)accumulated_turnaround/(float)jobs_count;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	return (float)accumulated_response/(float)jobs_count;
}


/**
  Free any memory associated with your scheduler.

  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
  priqueue_destroy(&job_queue);
  free(core_statuses);
  free(core_jobs);
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)

  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{

}
