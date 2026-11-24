#!/usr/bin/env python3
"""Check a single shot captured from the EXTREMIS-RE gameplay loop."""

import argparse
import sys
from collections import defaultdict


def read_trace(lines):
    frames = defaultdict(dict)
    for line in lines:
        if not line.startswith("SHOT "):
            continue
        _, timestamp, frame, event, *fields = line.split()
        if event == "input":  # A click and its release can share a frame.
            continue
        if event in frames[int(frame)]:
            raise ValueError(f"repeated {event} in frame {frame}; capture one shot at a time")
        frames[int(frame)][event] = int(timestamp), dict(field.split("=", 1) for field in fields)
    return frames


def check(frames, args):
    creates = [n for n, events in frames.items() if "create" in events]
    impacts = [n for n, events in frames.items() if "impact" in events]
    if len(creates) != 1 or len(impacts) != 1:
        raise ValueError("expected one shot with one impact")
    start, impact = creates[0], impacts[0]
    target = frames[impact]["impact"][1]["target"]
    if args.target and target != args.target:
        raise ValueError(f"expected {args.target}, found {target}")
    removal = [n for n, events in frames.items() if "remove" in events]
    if len(removal) != 1 or frames[removal[0]]["remove"][1]["reason"] != "last_phase_queued":
        raise ValueError("expected the shot to complete all five impact phases")

    def completed(n):
        return frames[n].get("present", (0, {}))[1].get("ok") == "1"

    end = next((n for n in sorted(frames) if n > removal[0] and completed(n)
                and frames[n].get("render", (0, {}))[1].get("sprites") == "0"), None)
    if end is None:
        raise ValueError("missing successful present after the sprite disappears")
    if any(frames[n].get("update", (0, {}))[1].get("active") != "1"
           for n in range(start, end + 1)):
        raise ValueError("shot crosses a pause or discontinuity; capture one uninterrupted shot")
    shot_frames = [n for n in range(start, end) if "step" in frames[n]]
    if not shot_frames:
        raise ValueError("missing projectile steps")

    created_at = frames[start]["create"][0]
    hit_at = frames[impact]["impact"][0]
    removed_at = frames[removal[0]]["remove"][0]
    print(f"{target}: create→impact {(hit_at-created_at)/1e6:.3f} ms, "
          f"impact→slot free {(removed_at-hit_at)/1e6:.3f} ms")
    print("phase  updates  rendered  SDL-ok  color+SDL  update-ms  SDL-span-ms")
    errors = []
    durations = {}
    for phase in range(6):
        steps = [n for n in shot_frames if int(frames[n]["step"][1]["phase"]) == phase]
        if not steps:
            if phase == 0 and not args.require_flight:
                continue
            errors.append(f"missing phase {phase}")
            continue
        first, last = steps[0], steps[-1]
        step_end = next((frames[n]["step"][0] for n in shot_frames if n > last),
                        frames[end]["update"][0])
        updates_ms = (step_end-(created_at if phase == 0 else frames[first]["step"][0]))/1e6
        submitted = [n for n in steps if "sprite" in frames[n] and "render" in frames[n]]
        presents = [n for n in submitted if completed(n)]
        covered = [n for n in presents if "coverage" in frames[n]]
        visible = sum(int(frames[n]["coverage"][1]["composed"]) > 0 for n in covered)
        span = None
        if presents:
            following = next(n for n in range(last+1, end+1) if completed(n))
            span = (frames[following]["present"][0]-frames[presents[0]]["present"][0])/1e6
        span_text = f"{span:.3f}" if span is not None else "-"
        color_text = f"{visible}/{len(covered)}" if covered else "-"
        print(f"{'flight' if phase == 0 else phase!s:>6} {len(steps):>8} {len(submitted):>9} "
              f"{len(presents):>7} {color_text:>10} {updates_ms:>10.3f} {span_text:>12}")
        if span is not None:
            durations[phase] = span
        if len(presents) < args.min_presents:
            errors.append(f"phase {phase} has too few successful SDL presents")
        if args.require_color and (len(covered) != len(presents) or visible < args.min_presents):
            errors.append(f"phase {phase} lacks composed color in enough successful presents")
        if args.min_ms is not None and (span is None or span < args.min_ms):
            errors.append(f"phase {phase} falls below {args.min_ms:g} ms between SDL returns")
    if errors:
        raise ValueError("; ".join(errors))
    return target, durations


def check_fixed_step(lines, target):
    """Check simulation deadlines separately from successful SDL presentations."""
    frames = defaultdict(list)
    for line in lines:
        if line.startswith("SHOT "):
            _, when, frame, event, *fields = line.split()
            frames[int(frame)].append((int(when), event, dict(f.split("=", 1) for f in fields)))
    events = [(n, t, kind, data) for n, entries in sorted(frames.items()) for t, kind, data in entries]
    creates = [event for event in events if event[2] == "create"]
    impacts = [event for event in events if event[2] == "impact"]
    if len(creates) != 1 or len(impacts) != 1:
        raise ValueError("expected one shot with one impact")
    start, created, _, _ = creates[0]
    _, hit_at, _, impact = impacts[0]
    if target and impact["target"] != target:
        raise ValueError(f"expected {target}, found {impact['target']}")
    steps = [event for event in events if event[2] == "step"]
    phases = [int(event[3]["phase"]) for event in steps]
    flight = phases.count(0)
    if phases != [0] * flight + [1, 2, 3, 4, 5]:
        raise ValueError(f"incorrect simulation phase sequence {phases}")

    period = 1000 / 60
    elapsed = 0
    simulated = 0
    flash_presents = 0
    phase_presents = [0] * 6
    color_presents = [0] * 6
    has_coverage = False
    present_times = []
    active = True
    paused = 0
    flash_off = None
    clear_at = None
    for n, entries in sorted(frames.items()):
        by_kind = {kind: (time, data) for time, kind, data in entries}
        if "present" in by_kind and by_kind["present"][1]["ok"] == "1":
            present_times.append(by_kind["present"][0])
        if n < start or clear_at is not None:
            continue
        if "update" not in by_kind or "render" not in by_kind:
            continue
        update_at, update = by_kind["update"]
        render = by_kind["render"][1]
        running = update["active"] == "1"
        paused += not running
        if n > start and active and running:
            elapsed += min(float(update["dt_ms"]), 8 * period)
        active = running
        due = int((elapsed + 1e-8) / period) + 1
        simulated += sum(kind == "step" for _, kind, _ in entries)
        if simulated != min(due, len(phases)):
            raise ValueError(f"frame {n}: expected {min(due, len(phases))} cumulative steps, saw {simulated}")
        flash = elapsed + 1e-8 < period
        if render.get("flash") != str(int(flash)):
            raise ValueError(f"frame {n}: flash differs from its simulation deadline")
        if not flash and flash_off is None:
            flash_off = update_at
        on_screen = due <= len(phases)
        if int(render["sprites"]) != int(on_screen):
            raise ValueError(f"frame {n}: lifetime of queued sprite differs from simulation deadline")
        if by_kind.get("present", (0, {}))[1].get("ok") == "1" and render["modal"] == "0":
            flash_presents += flash
            if on_screen:
                phase = phases[due - 1]
                phase_presents[phase] += 1
                if "coverage" in by_kind:
                    has_coverage = True
                    color_presents[phase] += int(by_kind["coverage"][1]["composed"]) > 0
            else:
                clear_at = update_at
    if flash_off is None or not flash_presents or clear_at is None:
        raise ValueError("missing flash presentation or end-of-shot presentation")
    damage = [f"{data['before']}→{data['after']}" for _, _, kind, data in events if kind == "damage"]
    if impact["target"] == "enemy" and len(damage) != 1:
        raise ValueError("expected a single enemy damage event")
    renderer = next(data for _, _, kind, data in events if kind == "renderer")
    fps = (len(present_times) - 1) * 1e9 / (present_times[-1] - present_times[0])
    print(f"{impact['target']} ({impact['x']}, {impact['y']}) damage={','.join(damage) or '—'} "
          f"vsync={renderer['vsync']} refresh={renderer['refresh_hz']} avg_fps={fps:.1f}")
    print(f"simulation: flash {period:.3f} ms, flight {flight * period:.3f} ms, "
          f"impact phases {period:.3f} ms each")
    print(f"wall clock: flash off {(flash_off-created)/1e6:.3f} ms, hit {(hit_at-created)/1e6:.3f} ms, "
          f"impact to clear {(clear_at-hit_at)/1e6:.3f} ms, inactive updates {paused}")
    print(f"SDL presents: flash {flash_presents}, flight {phase_presents[0]}, impacts {phase_presents[1:]}")
    if has_coverage:
        print(f"SDL presents with composed projectile color: flight {color_presents[0]}, impacts {color_presents[1:]}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=argparse.FileType())
    parser.add_argument("--target", choices=("enemy", "wall", "door"))
    parser.add_argument("--fixed-step", action="store_true", help="check the 60 Hz gameplay scheduler")
    parser.add_argument("--require-flight", action="store_true")
    parser.add_argument("--require-color", action="store_true")
    parser.add_argument("--min-presents", type=int, default=1)
    parser.add_argument("--min-ms", type=float)
    parser.add_argument("--compare-with", type=argparse.FileType())
    parser.add_argument("--tolerance-ms", type=float)
    args = parser.parse_args()
    if args.compare_with and (args.tolerance_ms is None or args.tolerance_ms < 0):
        parser.error("--compare-with requires a nonnegative --tolerance-ms")
    try:
        if args.fixed_step:
            check_fixed_step(args.trace, args.target)
            return
        target, durations = check(read_trace(args.trace), args)
        if args.compare_with:
            other_target, other = check(read_trace(args.compare_with), args)
            if target != other_target or durations.keys() != other.keys():
                raise ValueError("traces must cover the same target type and phases")
            different = {phase: abs(ms - other[phase]) for phase, ms in durations.items()
                         if abs(ms - other[phase]) > args.tolerance_ms}
            if different:
                details = ", ".join(f"{phase}: {ms:.3f} ms" for phase, ms in different.items())
                raise ValueError(f"SDL span differs by more than {args.tolerance_ms:g} ms ({details})")
    except (KeyError, ValueError) as error:
        sys.exit(str(error))


if __name__ == "__main__":
    main()
