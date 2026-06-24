#ifndef TASK_LOADER_H
#define TASK_LOADER_H

/* Native port of CTaskList (docs/decomp/CTaskList.md).

   CTaskList is the task / game-state object loaded at level start: it
   deserializes a `.tsk` file carrying the level's start record (which `.gam`
   to load + spawn point) and an initial entity-state override table over the
   mission/actor tag namespace (NPCs, the SCENE sequencer, the REACTOR
   objective). CJimmyGame owns a CTaskList and consults it during InitGame.

   The on-disk `.tsk` format (LV1B) is measured in docs/decomp/CTaskList.md and
   round-tripped by tools/tsk_parser.py; this is the C port of that parser. Only
   two task files ship in the original: NewGame.tsk (fresh game -> Level 1) and
   RestartLevel.tsk (respawn). The proprietary binaries are not committed, so
   task_load() falls back to a baked NewGame default derived from the committed
   docs/tsk_data/NewGame.tsk.json. */

#define TASK_MAX_ENTITIES 64
#define TASK_TAG_MAX      32

typedef struct {
    char         tag[TASK_TAG_MAX];
    unsigned int state;          /* 0 = default/inactive; non-zero = custom */
} TaskEntityState;

typedef struct {
    char  task_name[32];         /* typically 'STARTEXP' */
    char  gam_file[64];          /* level .gam this task loads, e.g. 'level1b.gam' */
    float spawn[3];              /* start X, Y, Z */
    int   has_start;             /* 1 when a STARTEXP record was found */
    TaskEntityState entities[TASK_MAX_ENTITIES];
    int   entity_count;
    int   valid;                 /* 1 when parsed or baked successfully */
} TaskList;

/* Parse a `.tsk` binary (LV1B magic). Faithful to tools/tsk_parser.py:
   STARTEXP record (4 pstrings + 3 BE floats) then a u8-counted entity-state
   table of (pstring tag, u32 BE state) that runs to EOF. Returns 1 on success. */
int task_parse_file(const char *path, TaskList *out);

/* Load a named task ('NewGame' / 'RestartLevel'). Searches JN_TSK_ROOT, the
   gam root, and xp-jnbg-original for "<name>.tsk"; if none is found and name is
   'NewGame', fills the baked default (level1b.gam + the 12-entity table).
   Returns 1 if any data (parsed or baked) is available. */
int task_load(const char *name, TaskList *out);

/* Current state for a mission tag (case-insensitive). Returns -1 if absent.
   The TaskList is the live runtime store, so this reflects any task_set writes. */
long task_entity_state(const TaskList *t, const char *tag);

/* Write a mission tag's runtime state (case-insensitive). Faithful to
   set_task_state (FUN_0045f990): writes EXISTING entries only — the original
   never appends a new tag. Returns 1 if the tag was found and written, else 0. */
int task_set_entity_state(TaskList *t, const char *tag, long value);

#endif
