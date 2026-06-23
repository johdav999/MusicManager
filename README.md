# MusicManager

Developed with Unreal Engine 5

## Simulation Time

`UGameTimeSubsystem` owns deterministic weekly advancement. Existing monthly systems remain behind a month-boundary compatibility pass while subsystems migrate to native weekly logic.
Use `AdvanceOneWeek()` for normal progression, `AdvanceWeeks(NumWeeks)` for fast-forward, and `AdvanceMonth()` only as a legacy compatibility entry point.

## Simulation Time

`UGameTimeSubsystem` owns deterministic weekly advancement. Existing monthly systems remain behind a month-boundary compatibility pass while subsystems migrate to native weekly logic.
Use `AdvanceOneWeek()` for normal progression, `AdvanceWeeks(NumWeeks)` for fast-forward, and `AdvanceMonth()` only as a legacy compatibility entry point.

## Simulation Time

`UGameTimeSubsystem` owns weekly advancement. Use `AdvanceOneWeek()` for normal single-step time progression and `AdvanceWeeks(NumWeeks)` for fast-forward. Multi-week advancement still runs every deterministic weekly simulation phase, but UI refreshes are coalesced and flushed once after the batch completes.
