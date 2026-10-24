"""YJJJump 프로토타입 픽셀아트 생성기.

외부 라이브러리 없이 PNG를 만든다. 엔진에 들어 있는 Python으로 실행:
    Engine/Binaries/ThirdParty/Python3/Win64/python.exe -I Tools/Art/generate_art.py
결과는 Tools/Art/Generated/*.png. 임포트는 Tools/Art/import_assets.py.
"""

import math
import os
import struct
import sys
import zlib

OUTLINE = (43, 27, 46, 255)
CLEAR = (0, 0, 0, 0)


def hex_color(value, alpha=255):
    value = value.lstrip("#")
    return (int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16), alpha)


class Canvas:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = [[CLEAR for _ in range(width)] for _ in range(height)]

    def set(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y][x] = color

    def get(self, x, y):
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.pixels[y][x]
        return CLEAR

    def fill(self, inside, color):
        for y in range(self.height):
            for x in range(self.width):
                if inside(x + 0.5, y + 0.5):
                    self.pixels[y][x] = color

    def ellipse(self, cx, cy, rx, ry, color):
        self.fill(lambda x, y: ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0, color)

    def circle(self, cx, cy, r, color):
        self.ellipse(cx, cy, r, r, color)

    def rect(self, x0, y0, x1, y1, color):
        self.fill(lambda x, y: x0 <= x < x1 and y0 <= y < y1, color)

    def polygon(self, points, color):
        def inside(x, y):
            result = False
            j = len(points) - 1
            for i in range(len(points)):
                xi, yi = points[i]
                xj, yj = points[j]
                if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
                    result = not result
                j = i
            return result

        self.fill(inside, color)

    def outline(self, color=OUTLINE):
        """불투명 픽셀 바깥쪽에 1px 테두리."""
        edge = []
        for y in range(self.height):
            for x in range(self.width):
                if self.pixels[y][x][3] == 0:
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        if self.get(x + dx, y + dy)[3] > 0 and self.get(x + dx, y + dy) != color:
                            edge.append((x, y))
                            break
        for x, y in edge:
            self.pixels[y][x] = color

    def save(self, path):
        raw = bytearray()
        for row in self.pixels:
            raw.append(0)
            for pixel in row:
                raw.extend(pixel)

        def chunk(kind, data):
            body = kind + data
            return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

        png = b"\x89PNG\r\n\x1a\n"
        png += chunk(b"IHDR", struct.pack(">IIBBBBB", self.width, self.height, 8, 6, 0, 0, 0))
        png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        png += chunk(b"IEND", b"")
        with open(path, "wb") as file:
            file.write(png)


def eyes(c, left, right, y, look=(1, -1), size=4):
    """흰자 + 눈동자. look은 눈동자 방향."""
    for ex in (left, right):
        c.ellipse(ex, y, size * 0.75, size, hex_color("#FFFFFF"))
        c.circle(ex + look[0] * 1.2, y + look[1] * 1.2, size * 0.45, hex_color("#1E1428"))
        c.set(int(ex + look[0] * 1.2 - 1), int(y + look[1] * 1.2 - 1), hex_color("#FFFFFF"))


def make_player():
    # 주인공 YJJ: 주황 동글이 + 빨간 머리띠
    c = Canvas(48, 48)
    c.ellipse(17, 43, 6, 3.5, hex_color("#C8641E"))
    c.ellipse(31, 43, 6, 3.5, hex_color("#C8641E"))
    c.circle(24, 26, 18, hex_color("#FF9A35"))
    c.ellipse(24, 32, 11, 9, hex_color("#FFC27A"))
    c.ellipse(16, 18, 4, 3, hex_color("#FFC88A"))
    # 머리띠와 휘날리는 끈
    c.fill(lambda x, y: 11 <= y < 15 and ((x - 24) / 18) ** 2 + ((y - 26) / 18) ** 2 <= 1.0, hex_color("#E5363C"))
    c.polygon([(6, 12), (1, 8), (2, 15), (8, 15)], hex_color("#E5363C"))
    c.polygon([(6, 15), (0, 18), (3, 21), (9, 16)], hex_color("#C42A31"))
    eyes(c, 20, 30, 22, look=(1, -1), size=4)
    c.ellipse(15, 29, 3, 2, hex_color("#FF6F6F"))
    c.ellipse(35, 29, 3, 2, hex_color("#FF6F6F"))
    c.polygon([(22, 31), (28, 31), (25, 35)], hex_color("#7A2A1A"))
    c.outline()
    return c


def make_cloud_slime():
    # 구름 슬라임: 가장 약한 몹
    c = Canvas(48, 40)
    body = hex_color("#F4FAFF")
    c.circle(16, 22, 10, body)
    c.circle(26, 16, 12, body)
    c.circle(34, 23, 9, body)
    c.rect(10, 22, 40, 33, body)
    c.ellipse(24, 32, 15, 4, hex_color("#CFE4F5"))
    c.ellipse(20, 12, 4, 2.5, hex_color("#FFFFFF"))
    eyes(c, 20, 30, 23, look=(0, 0), size=3)
    c.ellipse(15, 28, 2.5, 1.5, hex_color("#FF9DB0"))
    c.ellipse(35, 28, 2.5, 1.5, hex_color("#FF9DB0"))
    c.outline()
    return c


def make_bird():
    # 하늘 새: 좌우로 순찰. 오른쪽을 본다
    c = Canvas(48, 40)
    c.polygon([(4, 18), (12, 14), (12, 26), (2, 25)], hex_color("#2C6FBF"))
    c.ellipse(24, 22, 16, 12, hex_color("#3F8FE8"))
    c.ellipse(26, 27, 10, 6, hex_color("#BFE3FF"))
    c.polygon([(14, 16), (24, 4), (30, 6), (26, 18)], hex_color("#2C6FBF"))
    c.polygon([(38, 19), (47, 22), (38, 25)], hex_color("#FFC233"))
    eyes(c, 33, 33, 17, look=(1, 0), size=3)
    c.rect(20, 33, 22, 38, hex_color("#FFC233"))
    c.rect(27, 33, 29, 38, hex_color("#FFC233"))
    c.outline()
    return c


def make_thunder_cloud():
    # 번개 구름: 튼튼한 몹. 위아래로 떠다닌다
    c = Canvas(56, 48)
    body = hex_color("#5C5F78")
    c.circle(17, 19, 11, body)
    c.circle(29, 14, 13, body)
    c.circle(40, 20, 10, body)
    c.rect(9, 19, 48, 30, body)
    c.ellipse(28, 29, 18, 4, hex_color("#44465C"))
    c.ellipse(24, 9, 5, 2.5, hex_color("#7C809C"))
    c.polygon([(27, 30), (22, 41), (27, 40), (24, 48), (34, 36), (29, 37), (32, 30)], hex_color("#FFE14D"))
    eyes(c, 22, 35, 20, look=(0, 1), size=3.5)
    # 화난 눈썹
    for i in range(6):
        c.set(18 + i, 14 + i // 2, OUTLINE)
        c.set(39 - i, 14 + i // 2, OUTLINE)
    c.outline()
    return c


def make_impact():
    # 타격 이펙트: 별 모양 폭발
    c = Canvas(48, 48)
    points = []
    for i in range(16):
        angle = math.pi * 2 * i / 16 - math.pi / 2
        radius = 23 if i % 2 == 0 else 10
        points.append((24 + math.cos(angle) * radius, 24 + math.sin(angle) * radius))
    c.polygon(points, hex_color("#FFD23F"))
    c.circle(24, 24, 8, hex_color("#FFF6C8"))
    c.outline(hex_color("#E0701E"))
    return c


def make_rope():
    # 꼬인 밧줄. 길이 방향으로 늘려 쓴다
    c = Canvas(32, 8)
    for y in range(1, 7):
        for x in range(32):
            stripe = (x + y * 2) % 8 < 4
            c.set(x, y, hex_color("#B9824A") if stripe else hex_color("#8E5B2E"))
    for x in range(32):
        c.set(x, 0, OUTLINE)
        c.set(x, 7, OUTLINE)
    return c


def make_rope_peg():
    c = Canvas(16, 16)
    c.circle(8, 8, 6.5, hex_color("#6E4A2B"))
    c.circle(8, 8, 4, hex_color("#93683F"))
    c.set(6, 5, hex_color("#C99B66"))
    c.set(7, 5, hex_color("#C99B66"))
    c.outline()
    return c


def make_sky():
    # 세로 그라데이션 (위: 진한 파랑, 아래: 연한 하늘)
    c = Canvas(8, 256)
    top = hex_color("#3D6FD6")
    bottom = hex_color("#BDE7FF")
    for y in range(256):
        t = y / 255.0
        color = tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3)) + (255,)
        for x in range(8):
            c.set(x, y, color)
    return c


ART = {
    "T_Player": make_player,
    "T_MobCloudSlime": make_cloud_slime,
    "T_MobBird": make_bird,
    "T_MobThunderCloud": make_thunder_cloud,
    "T_Impact": make_impact,
    "T_Rope": make_rope,
    "T_RopePeg": make_rope_peg,
    "T_SkyBackground": make_sky,
}


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "Generated")
    os.makedirs(out_dir, exist_ok=True)
    for name, make in ART.items():
        path = os.path.join(out_dir, name + ".png")
        make().save(path)
        print("wrote", path)


if __name__ == "__main__":
    main()
