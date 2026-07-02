/* Headless dumper for the CJimmyGame initgame-seed linkage oracle
   (docs/linked_parity_plan.md L3). Links the real game_flow.c (unmodified) +
   task_loader.c (its only non-libc dependency) and exercises the public
   game_flow_init_game()/game_flow_lives()/game_flow_mission_value() API
   directly -- these are ordinary (non-static) functions, so no #include
   trick is needed here (unlike the cutscene camera oracles). */
#include <stdio.h>
#include "../../src/game/game_flow.h"

int main(void) {
    /* Before any seed call: globals are zero-initialized (BSS), not
       coincidentally matching the decompiled constants -- proves the values
       below genuinely come from calling InitGame, not static init. */
    printf("PRE|%d|%d\n", game_flow_lives(), game_flow_mission_value());

    game_flow_init_game();
    printf("SEED1|%d|%d\n", game_flow_lives(), game_flow_mission_value());

    /* Re-seed must be idempotent (deterministic reset, not accumulation) --
       matches InitGame unconditionally overwriting the fields rather than
       incrementing them. */
    game_flow_init_game();
    printf("SEED2|%d|%d\n", game_flow_lives(), game_flow_mission_value());

    return 0;
}
