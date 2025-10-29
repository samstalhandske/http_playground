#ifndef WORKER_H
#define WORKER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef MAX_WORKER_TASKS
#define MAX_WORKER_TASKS 64
#endif

typedef uint32_t Worker_UID;
typedef void Worker_Context;
typedef bool (*Worker_Task_Callback)(Worker_Context *context, const uint32_t lifetime);

typedef struct {
    Worker_UID uid;

    Worker_Context *context;
    Worker_Task_Callback callback;

    uint32_t lifetime;
    bool done;
} Worker_Task;

typedef struct {
    const char *name;
    Worker_UID next_uid;

    Worker_Task tasks[MAX_WORKER_TASKS];
    uint32_t task_count;
} Worker;

bool worker_add_task(Worker *worker, Worker_Context *context, uint32_t context_size, const Worker_Task_Callback callback);
uint32_t worker_work(Worker *worker);

#endif