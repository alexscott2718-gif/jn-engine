/* Headless dumper for the CTaskList set-task-state linkage oracle
   (docs/linked_parity_plan.md L3). Compiles the real task_loader.c into a CLI
   that seeds the baked NewGame table (task_load("NewGame", ...) -- no
   proprietary .tsk needed, see docs/decomp/CTaskList.md), applies a sequence
   of `tag=value` writes via the real task_set_entity_state() (unmodified),
   and dumps each write's return code plus the final table so the oracle can
   diff it against an independently-transcribed Python reference of
   FUN_0045f990's write loop. */
#include "../../src/game/task_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    TaskList t;
    if (!task_load("NewGame", &t)) {
        fprintf(stderr, "task_load(NewGame) failed\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (!eq) {
            fprintf(stderr, "bad arg (expected tag=value): %s\n", argv[i]);
            return 1;
        }
        *eq = '\0';
        long val = strtol(eq + 1, NULL, 10);
        int ok = task_set_entity_state(&t, argv[i], val);
        printf("W|%s|%ld|%d\n", argv[i], val, ok);
    }

    printf("N|%d\n", t.entity_count);
    for (int i = 0; i < t.entity_count; i++)
        printf("E|%s|%u\n", t.entities[i].tag, t.entities[i].state);
    return 0;
}
