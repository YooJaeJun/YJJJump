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


def line(c, x0, y0, x1, y1, color, width=1.0):
    """두께 있는 선분."""
    def inside(x, y):
        dx, dy = x1 - x0, y1 - y0
        length_sq = dx * dx + dy * dy or 1.0
        t = max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / length_sq))
        px, py = x0 + t * dx, y0 + t * dy
        return (x - px) ** 2 + (y - py) ** 2 <= (width / 2.0) ** 2

    c.fill(inside, color)


def bald_head(c, cx, cy, r, skin="#F4B4A0"):
    """번들번들 대머리 + 반짝이 + 머리카락 세 가닥."""
    c.circle(cx, cy, r, hex_color(skin))
    c.ellipse(cx - r * 0.35, cy - r * 0.5, r * 0.3, r * 0.15, hex_color("#FFFFFF"))
    for i, dx in enumerate((-2, 0, 2)):
        line(c, cx + dx, cy - r + 1, cx + dx + (i - 1) * 2, cy - r - 4, OUTLINE, 1.0)


# 이하 아트는 일부러 병맛 / 초딩 취향. 귀여우면 실패.


def make_beetle():
    # 주인공: Lv 999Gp였던 장수풍뎅이. 두 발로 서는 열혈 꼬마 스타일.
    # 징그럽지 않게 다리는 둘, 머리는 크게(2등신), 멍청한 짝짝이 눈 + 열혈 눈썹 + 빨간 머리띠.
    # 나중에 투구(머리 위)와 검(앞으로 내민 오른손 주먹)을 붙일 수 있게 비워 둔다.
    c = Canvas(72, 88)
    skin = hex_color("#7A4524")        # 번들번들 갈색 피부(갑각)
    skin_light = hex_color("#B06A3A")
    shell = hex_color("#2B1A12")       # 등껍질 = 학생복 같은 검은 갑옷
    shell_light = hex_color("#5A3A26")

    # 신발 (빨간 운동화) + 다리
    c.rect(24, 70, 30, 80, skin)
    c.rect(38, 70, 44, 80, skin)
    c.ellipse(26, 82, 8, 4.5, hex_color("#E5363C"))
    c.ellipse(42, 82, 8, 4.5, hex_color("#E5363C"))
    c.rect(19, 83, 34, 86, hex_color("#FFFFFF"))
    c.rect(35, 83, 50, 86, hex_color("#FFFFFF"))
    # 몸통: 검은 등껍질 갑옷 + 배
    c.ellipse(34, 60, 16, 14, shell)
    c.ellipse(28, 55, 6, 3, shell_light)
    c.ellipse(36, 64, 9, 8, hex_color("#D9A86C"))
    c.rect(20, 66, 49, 69, hex_color("#1A0F0A"))
    c.rect(32, 66, 37, 69, hex_color("#FFD23F"))
    # 왼팔: 허리에 손 (뒤쪽)
    line(c, 20, 54, 14, 62, skin, 5)
    c.circle(16, 64, 3.5, skin)
    # 오른팔: 앞으로 쭉 내민 주먹 (나중에 검 손잡이 자리)
    line(c, 46, 54, 58, 56, skin, 5)
    c.circle(61, 56, 5, skin)
    c.set(59, 54, skin_light)
    # 머리 (크게)
    c.circle(34, 30, 20, skin)
    c.ellipse(26, 18, 6, 4, skin_light)
    # 뿔: 이마에서 위로 크게 휜 장수풍뎅이 뿔 (투구 쓰면 뿔이 뚫고 나옴)
    c.polygon([(36, 12), (42, 12), (48, 0), (52, 2), (47, 6), (44, 4)], hex_color("#3A2216"))
    c.polygon([(34, 14), (40, 14), (46, 2), (42, 4)], hex_color("#3A2216"))
    line(c, 40, 10, 45, 2, hex_color("#6E4A30"), 1)
    # 더듬이 두 가닥 (귀엽게 동그란 끝)
    line(c, 24, 14, 18, 4, OUTLINE, 1)
    c.circle(18, 4, 2, skin_light)
    line(c, 28, 12, 26, 2, OUTLINE, 1)
    c.circle(26, 2, 2, skin_light)
    # 열혈 빨간 머리띠 + 휘날리는 끈
    c.fill(lambda x, y: 20 <= y < 25 and (x - 34) ** 2 + (y - 30) ** 2 <= 20 ** 2, hex_color("#E5363C"))
    c.polygon([(14, 22), (4, 18), (6, 24)], hex_color("#E5363C"))
    c.polygon([(14, 24), (2, 28), (6, 30), (15, 26)], hex_color("#C42A31"))
    # 열혈 눈썹 (화남) + 멍청한 짝짝이 눈 (한쪽 눈동자 딴 데 봄)
    line(c, 25, 27, 33, 30, OUTLINE, 2.5)
    line(c, 37, 30, 46, 27, OUTLINE, 2.5)
    c.ellipse(30, 35, 4.5, 5, hex_color("#FFFFFF"))
    c.circle(31, 36, 2, hex_color("#000000"))
    c.ellipse(42, 35, 3.5, 4, hex_color("#FFFFFF"))
    c.circle(40, 34, 1.5, hex_color("#000000"))
    # 볼터치 + 소리 지르는 입 (이빨 하나)
    c.ellipse(23, 41, 3, 2, hex_color("#FF8A8A"))
    c.ellipse(47, 41, 3, 2, hex_color("#FF8A8A"))
    c.ellipse(37, 45, 5, 4, hex_color("#5A0E14"))
    c.ellipse(37, 47, 3, 1.5, hex_color("#E8506A"))
    c.rect(35, 41, 38, 44, hex_color("#FFFFF0"))
    # 땀방울 (멍충미)
    c.ellipse(53, 26, 1.5, 2.5, hex_color("#8FD8FF"))
    c.outline()
    return c


def make_bald_sparrow():
    # 대머리 참새: 머리카락 세 가닥, 눈 밑 다크서클, 콧수염
    c = Canvas(56, 48)
    c.ellipse(24, 30, 16, 12, hex_color("#8A5A34"))
    c.ellipse(26, 34, 10, 7, hex_color("#E6CFA8"))
    c.polygon([(6, 26), (0, 20), (2, 30), (10, 32)], hex_color("#5E3A1E"))
    c.polygon([(16, 24), (26, 20), (24, 32)], hex_color("#6E4526"))
    bald_head(c, 38, 18, 11)
    # 퀭한 눈 + 다크서클
    c.ellipse(41, 17, 3, 2.5, hex_color("#FFFFFF"))
    c.set(42, 17, hex_color("#000000"))
    line(c, 38, 20, 44, 20, hex_color("#7A5A8A"), 1)
    line(c, 37, 13, 45, 15, OUTLINE, 1.5)
    # 부리 + 콧수염
    c.polygon([(48, 19), (55, 21), (48, 23)], hex_color("#F0A020"))
    line(c, 44, 24, 51, 25, hex_color("#3A2414"), 2)
    line(c, 51, 25, 53, 28, hex_color("#3A2414"), 1.5)
    # 다리
    line(c, 22, 41, 20, 47, hex_color("#F0A020"), 1.5)
    line(c, 28, 41, 29, 47, hex_color("#F0A020"), 1.5)
    c.outline()
    return c


def make_bald_eagle():
    # 대머리 독수리: 선글라스, 갈고리 부리, 금목걸이, 근육 날개. 레벨 도둑
    c = Canvas(80, 72)
    c.ellipse(34, 46, 24, 18, hex_color("#4A2E1A"))
    # 근육 날개 (알통)
    c.polygon([(14, 36), (2, 22), (6, 18), (20, 28)], hex_color("#3A2414"))
    c.circle(8, 18, 6, hex_color("#3A2414"))
    c.ellipse(26, 40, 9, 6, hex_color("#5E3C24"))
    # 흰 목털
    c.ellipse(50, 34, 12, 8, hex_color("#F4F0E6"))
    for x in range(40, 62, 4):
        c.polygon([(x, 38), (x + 2, 44), (x + 4, 38)], hex_color("#F4F0E6"))
    bald_head(c, 54, 22, 13)
    # 선글라스 + 반짝
    c.rect(46, 18, 66, 23, hex_color("#101018"))
    c.rect(55, 18, 57, 21, hex_color("#101018"))
    c.set(48, 19, hex_color("#FFFFFF"))
    c.set(60, 19, hex_color("#FFFFFF"))
    # 갈고리 부리 + 비웃는 입꼬리
    c.polygon([(64, 24), (78, 26), (79, 32), (74, 30), (66, 30)], hex_color("#F2B21C"))
    line(c, 66, 30, 72, 29, hex_color("#8A5A10"), 1)
    # 금목걸이
    for x in range(42, 62, 2):
        c.set(x, 43 + (x - 52) ** 2 // 40, hex_color("#FFD23F"))
    # 다리
    line(c, 28, 62, 26, 70, hex_color("#F2B21C"), 2)
    line(c, 38, 62, 40, 70, hex_color("#F2B21C"), 2)
    c.outline()
    return c


def make_impact():
    # 만화 효과 "쾅!": 삐죽삐죽 폭발 + 느낌표
    c = Canvas(56, 56)
    points = []
    for i in range(20):
        angle = math.pi * 2 * i / 20
        radius = 27 if i % 2 == 0 else 14 + (i % 3) * 2
        points.append((28 + math.cos(angle) * radius, 28 + math.sin(angle) * radius))
    c.polygon(points, hex_color("#FF3B2F"))
    inner = [(28 + (x - 28) * 0.65, 28 + (y - 28) * 0.65) for x, y in points]
    c.polygon(inner, hex_color("#FFE14D"))
    c.rect(25, 15, 31, 31, OUTLINE)
    c.rect(25, 34, 31, 40, OUTLINE)
    c.outline()
    return c


def make_cloud_butt():
    # 배경 장식: 엉덩이 구름
    c = Canvas(64, 40)
    c.circle(20, 22, 15, hex_color("#FFF4F6"))
    c.circle(44, 22, 15, hex_color("#FFF4F6"))
    c.ellipse(32, 30, 24, 9, hex_color("#FFF4F6"))
    line(c, 32, 10, 32, 30, hex_color("#E8B8C4"), 1.5)
    c.ellipse(16, 16, 4, 2.5, hex_color("#FFFFFF"))
    c.ellipse(13, 26, 3, 1.5, hex_color("#FFC2D0"))
    c.ellipse(51, 26, 3, 1.5, hex_color("#FFC2D0"))
    c.outline(hex_color("#C8A0B0"))
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
    "T_Beetle": make_beetle,
    "T_MobBaldSparrow": make_bald_sparrow,
    "T_MobBaldEagle": make_bald_eagle,
    "T_Impact": make_impact,
    "T_CloudButt": make_cloud_butt,
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
