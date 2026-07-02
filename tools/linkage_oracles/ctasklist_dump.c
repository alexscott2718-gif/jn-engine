/* Headless dumper for the CTaskList .tsk parser (task_parse_file), used by the
   CTaskList linkage oracle to diff the native C port against the reference
   parser tools/tsk_parser.py. No engine deps -- task_loader.c is pure libc.

   Build: cc -I src/game tools/linkage_oracles/ctasklist_dump.c \
              src/game/task_loader.c -o <out>
   Run:   <out> <path.tsk>   -> a canonical, byte-exact table dump on stdout. */
#include "task_loader.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.tsk>\n", argv[0]);
        return 2;
    }
    TaskList t;
    if (!task_parse_file(argv[1], &t)) {
        printf("parse|fail\n");
        return 0;
    }
    printf("name|%s\n", t.task_name);
    printf("gam|%s\n", t.gam_file);
    printf("start|%d\n", t.has_start);
    unsigned int b;
    memcpy(&b, &t.spawn[0], 4); printf("sx|%08x\n", b);
    memcpy(&b, &t.spawn[1], 4); printf("sy|%08x\n", b);
    memcpy(&b, &t.spawn[2], 4); printf("sz|%08x\n", b);
    printf("n|%d\n", t.entity_count);
    for (int i = 0; i < t.entity_count; i++)
        printf("e|%s|%u\n", t.entities[i].tag, t.entities[i].state);
    printf("valid|%d\n", t.valid);
    return 0;
}
