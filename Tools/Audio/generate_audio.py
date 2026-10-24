"""YJJJump BGM/효과음 합성기 (외부 라이브러리 없음, 직접 만든 소리라 상업적 사용 자유).

엔진 Python으로 실행:
    Engine/Binaries/ThirdParty/Python3/Win64/python.exe -I Tools/Audio/generate_audio.py
결과는 Tools/Audio/Generated/*.wav (44.1kHz 16bit 모노). 임포트는 Tools/Art/import_assets.py.
"""

import math
import os
import random
import struct
import sys
import wave

RATE = 44100
random.seed(1234)


# ---------------------------------------------------------------- 기본 파형

def square(phase, duty=0.5):
    return 1.0 if (phase % 1.0) < duty else -1.0


def triangle(phase):
    p = phase % 1.0
    return 4.0 * p - 1.0 if p < 0.5 else 3.0 - 4.0 * p


def noise():
    return random.uniform(-1.0, 1.0)


def note_hz(name):
    """'C5', 'F#4' 같은 음 이름 -> 주파수."""
    names = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}
    pitch, octave = name[:-1], int(name[-1])
    semitone = names[pitch] + (octave - 4) * 12 - 9
    return 440.0 * 2.0 ** (semitone / 12.0)


def tone(length, freq_start, freq_end=None, wave_fn=square, volume=0.5, attack=0.005, decay=None, duty=0.5, vibrato=0.0):
    """음 하나. 주파수는 시작->끝으로 지수 보간, 볼륨은 짧은 어택 + 감쇠."""
    freq_end = freq_end or freq_start
    count = int(length * RATE)
    out = []
    phase = 0.0
    for i in range(count):
        t = i / RATE
        alpha = i / max(count - 1, 1)
        freq = freq_start * (freq_end / freq_start) ** alpha
        if vibrato:
            freq *= 1.0 + 0.03 * math.sin(2 * math.pi * vibrato * t)
        phase += freq / RATE
        sample = wave_fn(phase, duty) if wave_fn is square else wave_fn(phase)
        env = min(t / attack, 1.0) if attack > 0 else 1.0
        if decay:
            env *= math.exp(-t / decay)
        else:
            env *= 1.0 - alpha
        out.append(sample * env * volume)
    return out


def noise_burst(length, volume=0.5, decay=0.05, lowpass=0.0):
    out = []
    previous = 0.0
    for i in range(int(length * RATE)):
        t = i / RATE
        sample = noise()
        if lowpass:
            previous = previous + (sample - previous) * (1.0 - lowpass)
            sample = previous
        out.append(sample * math.exp(-t / decay) * volume)
    return out


def mix(*tracks):
    length = max(len(t) for t in tracks)
    out = [0.0] * length
    for track in tracks:
        for i, s in enumerate(track):
            out[i] += s
    return out


def concat(*parts):
    out = []
    for part in parts:
        out.extend(part)
    return out


def add_at(buffer, samples, start_seconds):
    start = int(start_seconds * RATE)
    if len(buffer) < start + len(samples):
        buffer.extend([0.0] * (start + len(samples) - len(buffer)))
    for i, s in enumerate(samples):
        buffer[start + i] += s


def save(path, samples, peak=0.9):
    top = max(max(abs(s) for s in samples), 1e-6)
    scale = peak / top
    with wave.open(path, "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s * scale)) * 32767)) for s in samples))


# ---------------------------------------------------------------- 효과음

def sfx_launch():
    # 로프 "띠요옹": 아래에서 위로 휘는 삼각파 + 비브라토
    return mix(tone(0.35, 140, 620, triangle, 0.8, vibrato=18, decay=0.2),
               tone(0.12, 90, 60, triangle, 0.5, decay=0.05))


def sfx_land():
    # 로프에 "퉁"
    return mix(tone(0.16, 130, 55, triangle, 0.9, decay=0.06), noise_burst(0.05, 0.25, 0.01, 0.7))


def sfx_perfect():
    # "띠링!" 높은 두 음 + 반짝
    return concat(tone(0.07, note_hz("E6"), wave_fn=square, volume=0.45, duty=0.25, decay=0.06),
                  mix(tone(0.25, note_hz("B6"), wave_fn=square, volume=0.45, duty=0.25, decay=0.12),
                      tone(0.25, note_hz("E7"), wave_fn=triangle, volume=0.25, decay=0.1)))


def sfx_good():
    return tone(0.18, note_hz("G5"), wave_fn=square, volume=0.45, duty=0.25, decay=0.08)


def sfx_miss():
    # "뿌우" 내려가는 버즈
    return tone(0.3, 320, 140, square, 0.4, duty=0.5, decay=0.15)


def sfx_dash():
    # "슈웅": 거친 바람 + 올라가는 음
    whoosh = []
    previous = 0.0
    length = 0.22
    for i in range(int(length * RATE)):
        t = i / RATE
        previous += (noise() - previous) * 0.35
        env = math.sin(math.pi * t / length)
        whoosh.append(previous * env * 0.8)
    return mix(whoosh, tone(0.18, 300, 900, square, 0.18, duty=0.125, decay=0.1))


def sfx_hit():
    # "퍽"
    return mix(noise_burst(0.12, 0.8, 0.03, 0.5), tone(0.12, 160, 70, triangle, 0.9, decay=0.04))


def sfx_kill():
    # "빠악! 뿅": 타격 + 위로 튀는 팝
    smash = mix(noise_burst(0.15, 0.9, 0.04, 0.3), tone(0.15, 220, 60, square, 0.5, decay=0.05))
    pop = tone(0.18, 500, 1600, square, 0.35, duty=0.25, decay=0.08)
    out = list(smash)
    add_at(out, pop, 0.09)
    return out


def sfx_levelup():
    # 팡파레 아르페지오
    notes = ["C5", "E5", "G5", "C6", "E6"]
    out = []
    for i, name in enumerate(notes):
        add_at(out, tone(0.12 if i < 4 else 0.45, note_hz(name), wave_fn=square, volume=0.4, duty=0.25, decay=0.08 if i < 4 else 0.25), i * 0.07)
    add_at(out, tone(0.45, note_hz("C6"), wave_fn=triangle, volume=0.3, decay=0.25), 0.28)
    return out


# ---------------------------------------------------------------- BGM

def bgm_sky():
    """150BPM 칩튠 루프 16마디 (C 장조, 병맛 경쾌). 루프 지점에서 끊김 없게 마디 길이로 정확히 자른다."""
    bpm = 150
    eighth = 60.0 / bpm / 2
    bar = eighth * 8
    bars = 16
    total = int(bar * bars * RATE)
    out = [0.0] * total

    # 코드 진행: A(C G Am F) x2, B(F G Em Am), A'
    chords = ["C", "G", "Am", "F", "C", "G", "Am", "F", "F", "G", "Em", "Am", "C", "G", "F", "G"]
    roots = {"C": "C3", "G": "G2", "Am": "A2", "F": "F2", "Em": "E2"}
    triads = {"C": ["C5", "E5", "G5"], "G": ["B4", "D5", "G5"], "Am": ["C5", "E5", "A5"], "F": ["C5", "F5", "A5"], "Em": ["B4", "E5", "G5"]}

    # 멜로디 (음, 8분음표 길이). r = 쉼표
    A = [
        [("E5", 1), ("G5", 1), ("C6", 2), ("G5", 1), ("E5", 1), ("G5", 2)],
        [("D5", 1), ("G5", 1), ("B5", 2), ("A5", 1), ("G5", 1), ("D5", 2)],
        [("C5", 1), ("E5", 1), ("A5", 2), ("G5", 1), ("E5", 1), ("C5", 2)],
        [("A4", 1), ("C5", 1), ("F5", 2), ("E5", 1), ("D5", 1), ("C5", 2)],
    ]
    A2 = A[:3] + [[("A4", 1), ("C5", 1), ("F5", 1), ("A5", 1), ("G5", 2), ("r", 2)]]
    B = [
        [("A5", 2), ("G5", 1), ("F5", 1), ("A5", 2), ("C6", 2)],
        [("B5", 2), ("A5", 1), ("G5", 1), ("D6", 3), ("r", 1)],
        [("G5", 1), ("B5", 1), ("E6", 2), ("D6", 1), ("B5", 1), ("G5", 2)],
        [("A5", 1), ("E5", 1), ("A5", 1), ("C6", 1), ("E6", 2), ("r", 2)],
    ]
    ending = A[:2] + [[("A4", 1), ("C5", 1), ("F5", 2), ("A5", 2), ("C6", 2)],
                      [("B5", 2), ("D6", 2), ("G6", 3), ("r", 1)]]
    melody = A + A2 + B + ending

    for index in range(bars):
        start = index * bar
        chord = chords[index]

        # 리드 (25% 듀티 사각파)
        cursor = start
        for name, length in melody[index]:
            if name != "r":
                add_at(out, tone(length * eighth * 0.92, note_hz(name), wave_fn=square, volume=0.18, duty=0.25, decay=0.35, vibrato=6), cursor)
            cursor += length * eighth

        # 베이스 (삼각파, 8분 옥타브 통통)
        for step in range(8):
            name = roots[chord]
            octave_up = name[:-1] + str(int(name[-1]) + 1)
            add_at(out, tone(eighth * 0.85, note_hz(octave_up if step % 2 else name), wave_fn=triangle, volume=0.32, decay=0.2), start + step * eighth)

        # 아르페지오 (12.5% 듀티, 16분)
        for step in range(16):
            name = triads[chord][step % 3]
            add_at(out, tone(eighth * 0.45, note_hz(name), wave_fn=square, volume=0.05, duty=0.125, decay=0.05), start + step * eighth / 2)

        # 드럼: 킥 1,3박 / 스네어 2,4박 / 하이햇 8분
        for beat in range(4):
            t = start + beat * eighth * 2
            if beat % 2 == 0:
                add_at(out, tone(0.12, 150, 45, triangle, 0.6, decay=0.05), t)
            else:
                add_at(out, noise_burst(0.12, 0.35, 0.05, 0.2), t)
        for step in range(8):
            add_at(out, noise_burst(0.03, 0.08 if step % 2 else 0.12, 0.01), start + step * eighth)

    return out[:total]


SOUNDS = {
    "SFX_Launch": sfx_launch,
    "SFX_Land": sfx_land,
    "SFX_Perfect": sfx_perfect,
    "SFX_Good": sfx_good,
    "SFX_Miss": sfx_miss,
    "SFX_Dash": sfx_dash,
    "SFX_Hit": sfx_hit,
    "SFX_Kill": sfx_kill,
    "SFX_LevelUp": sfx_levelup,
    "BGM_Sky": bgm_sky,
}


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "Generated")
    os.makedirs(out_dir, exist_ok=True)
    for name, make in SOUNDS.items():
        path = os.path.join(out_dir, name + ".wav")
        save(path, make(), 0.8 if name.startswith("BGM") else 0.9)
        print("wrote", path)


if __name__ == "__main__":
    main()
