#!/usr/bin/env python3
"""Author deterministic original synthesis studies; no external samples or models.

These are implementation cues and a quiet ambience/music study, not final recorded
foley, orchestral music, or a mastered soundtrack. Regenerate with Python 3.
"""
from array import array
import hashlib
import json
import math
from pathlib import Path
import random
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Art" / "Audio"
RATE = 24000
TAU = 2 * math.pi


def tone(t, frequency, decay=5):
    return math.exp(-t * decay) * (math.sin(TAU * frequency * t) + .27 * math.sin(TAU * frequency * 2.01 * t))


def save(name, duration, sample, purpose, loop=False):
    count = int(duration * RATE)
    samples = array("h")
    peak = 0.0
    square_sum = 0.0
    for i in range(count):
        t = i / RATE
        # Smooth endpoints prevent command cues from introducing audio clicks.
        edge = min(1, t / .015, (duration - t) / .04)
        value = max(-.92, min(.92, sample(t) * max(0, edge)))
        peak = max(peak, abs(value))
        square_sum += value * value
        samples.append(round(value * 32767))
    if sys.byteorder != "little":
        samples.byteswap()
    path = OUTPUT / (name + ".wav")
    with wave.open(str(path), "wb") as sound:
        sound.setnchannels(1)
        sound.setsampwidth(2)
        sound.setframerate(RATE)
        sound.writeframes(samples.tobytes())
    return {"id": name, "path": "Audio/" + path.name, "purpose": purpose,
            "loop": loop, "duration_seconds": duration, "sample_rate": RATE,
            "channels": 1, "peak_dbfs": round(20 * math.log10(max(peak, 1e-9)), 2),
            "rms_dbfs": round(20 * math.log10(max(math.sqrt(square_sum / count), 1e-9)), 2),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    randomizer = random.Random(57118)
    notes = [293.6648, 329.6276, 391.9954, 440.0, 493.8833, 587.3295]
    events = [(j * 2 + .35, notes[[0, 2, 1, 4, 3, 2, 5, 4][j % 8]]) for j in range(16)]

    def woodland(t):
        # Wind is a periodic, slow mixture of inharmonic oscillators, deliberately quiet.
        wind = sum(math.sin(TAU * (37 + j * 13) * t + j * .8) for j in range(9)) * .003
        wind *= .6 + .4 * math.sin(TAU * t / 32) ** 2
        pad = sum(math.sin(TAU * f * t) for f in (146.8324, 195.9977, 246.9417)) * .015
        pad *= math.sin(math.pi * t / 32) ** 2
        bells = sum(.065 * tone(t - start, frequency, 1.9) for start, frequency in events if 0 <= t - start < 5)
        return wind + pad + bells

    manifest = []
    manifest.append(save("A_WoodlandLoop", 32, woodland, "Quiet woodland and original pentatonic bell study", True))
    manifest.append(save("A_Command", .22, lambda t: .16 * tone(t, 740, 22), "Accepted order acknowledgement"))
    manifest.append(save("A_Build", .62, lambda t: .18 * tone(t, 132, 10) + (.12 * tone(t - .18, 196, 11) if t >= .18 else 0), "Construction placement cue"))
    manifest.append(save("A_Magic", 1.4, lambda t: .17 * tone(t, 587.3295, 2.6) + .08 * tone(t, 880, 3.3), "Spell confirmation and light-network cue"))
    manifest.append(save("A_Alert", .7, lambda t: .12 * tone(t, 392, 8) + (.12 * tone(t - .24, 523.251, 8) if t >= .24 else 0), "Notification cue"))
    manifest.append(save("A_Impact", .35, lambda t: .2 * tone(t, 72, 17) + randomizer.uniform(-.09, .09) * math.exp(-t * 26), "Restrained combat impact"))
    manifest.append(save("A_Victory", 2.8, lambda t: sum(.14 * tone(t - start, f, 3.3) for start, f in ((0, 392), (.35, 493.8833), (.7, 587.3295), (1.05, 783.9909)) if t >= start), "Victory cue"))
    manifest.append(save("A_Defeat", 2.2, lambda t: .16 * tone(t, 220, 2.8) + (.12 * tone(t - .4, 196, 3) if t >= .4 else 0), "Defeat cue"))
    (OUTPUT / "manifest.json").write_text(json.dumps({"format_version": 1,
        "authorship": "Original deterministic mathematical synthesis in Tools/author_audio.py; no external recordings",
        "status": "Prototype synthesis studies; final sound design, listening review and Unreal mix remain pending",
        "assets": manifest}, indent=2) + "\n", encoding="utf-8")
    print(f"Authored {len(manifest)} original prototype WAV cues in {OUTPUT}")


if __name__ == "__main__":
    main()
