#include "worker.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

bool worker_add_task(Worker *worker, Worker_Context *context, uint32_t context_size, const Worker_Task_Callback callback) {
    assert(worker != NULL);
    assert(callback != NULL);

    if(worker->task_count >= MAX_WORKER_TASKS) {
        return false;
    }

    Worker_Task *task = &worker->tasks[worker->task_count];
    worker->task_count += 1;
    
    task->uid = worker->next_uid;
    worker->next_uid += 1;
    task->callback = callback;
    task->lifetime = 0;

    task->context = malloc(context_size);
    memcpy(task->context, context, context_size);

    return true;
}

uint32_t worker_work(Worker *worker) {
    assert(worker != NULL);
    if(worker->task_count == 0) {
        return 0;
    }

    // printf("'%s' working ...\n", worker->name);
    
    // Do the work.
    for(uint32_t i = 0; i < worker->task_count; i++) {
        Worker_Task *task = &worker->tasks[i];
        assert(task != NULL);

        task->done = task->callback(task->context, task->lifetime);
        task->lifetime += 1;
    }

    // Remove tasks that are done.
    int32_t task_count = worker->task_count;
    for(int32_t i = task_count - 1; i >= 0; i--) {
        Worker_Task *task = &worker->tasks[i];
        assert(task != NULL);

        if(!task->done) {
            continue;
        }

        free(task->context);

        if(i == (int32_t)worker->task_count - 1) {
            memset(task, 0, sizeof(Worker_Task));
        }
        else {
            Worker_Task *last_task = &worker->tasks[worker->task_count - 1];
            *task = *last_task;
            memset(last_task, 0, sizeof(Worker_Task));
        }

        worker->task_count -= 1;
    }

    // printf("'%s' done working.\n", worker->name);
    
    return worker->task_count;
}