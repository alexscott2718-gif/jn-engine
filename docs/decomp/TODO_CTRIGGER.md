# TODO — CTrigger generic proximity-volume primitive

> Standalone flag note. Fold into `docs/decomp/_next_session.md`'s task queue
> when picking this up; not yet wired into the parsed task sources.

**Status:** unported. Per `docs/linkage_certificates.csv`
(`linkage:ctrigger:enter-exit-latch`), CTrigger's watched-list sphere latch
with debounced enter/exit dispatch (vtable slots 21/22) has **no native file
implementing that mechanism at all** — not a simplification, an absence.

**Why it matters:** this is likely the connective-tissue linchpin blocking
campaign-playthrough hardening (Track E, `docs/continuation_options.md`).
Checkpoint progression, level-load triggers, and cutscene gating all plausibly
want to sit on top of a generic trigger-volume primitive rather than each
reinventing proximity detection independently. Recovering/porting CTrigger
first may unblock several of the other linked-blocked rows (`C3DCheckPoint`,
`CLoadLevel`) as a byproduct, or at least clarify how much of their current
divergence from the original is downstream of this gap.

**Entry points:** `docs/decomp/evidence/triggertype_trigger_target5.md` (RTTI
identity + mechanism already resolved: TRIG factory `FUN_0047dcf0`, base+0x5e8
`trigger_radius`, base+0x4b4 `LightRange`); no static caller of `UpdateTrigger`
or `RegisterTarget` currently exists in the recovered binary evidence, so the
native port needs its own registration/dispatch loop, not just a ported method.
