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

    def outline(self, color=OUTLINE, thickness=1):
        """불투명 픽셀 바깥쪽에 테두리. thickness 2 이상이면 냥코 대전쟁처럼 굵게."""
        for step in range(thickness):
            edge = []
            for y in range(self.height):
                for x in range(self.width):
                    if self.pixels[y][x][3] == 0:
                        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                            neighbor = self.get(x + dx, y + dy)
                            # 첫 줄은 그림 바깥에만, 이후 줄은 이미 그은 테두리 바깥에도
                            if neighbor[3] > 0 and (step > 0 or neighbor != color):
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


INK = hex_color("#141018")


def stick(c, x0, y0, x1, y1, width=3.0):
    """졸라맨 팔다리: 굵은 검은 막대."""
    line(c, x0, y0, x1, y1, INK, width)


def make_beetle():
    # 주인공 장수풍뎅이. 레퍼런스: 냥코 대전쟁(흰 얼굴, 점 눈, 굵은 외곽선) + 졸라맨(막대 팔다리) + 열혈 머리띠.
    # 머리 위(투구)와 앞으로 내민 주먹(검)은 나중에 장비를 붙일 자리.
    c = Canvas(76, 100)
    face = hex_color("#FFF7EA")
    shell = hex_color("#3B2416")
    shell_shine = hex_color("#8A5A3A")

    # 다리 (막대) + 빨간 운동화
    stick(c, 33, 74, 29, 90)
    stick(c, 41, 74, 46, 90)
    c.ellipse(27, 92, 6, 3.5, hex_color("#E5363C"))
    c.ellipse(48, 92, 6, 3.5, hex_color("#E5363C"))
    # 등껍질 (배낭처럼 등에 멘 장수풍뎅이 껍질)
    c.ellipse(28, 62, 11, 14, shell)
    line(c, 28, 50, 28, 75, hex_color("#1E120A"), 1)
    c.ellipse(24, 56, 3, 5, shell_shine)
    # 몸통 (작고 흰 냥코 몸)
    c.ellipse(38, 64, 10, 11, face)
    # 팔: 뒤 팔은 허리에, 앞 팔은 주먹을 쭉 (검 쥘 자리)
    stick(c, 32, 60, 24, 70)
    stick(c, 44, 60, 60, 58)
    c.circle(63, 58, 4, face)
    # 머리 (크고 흰, 살짝 찌그러진 냥코 얼굴)
    c.ellipse(38, 34, 23, 20, face)
    # 장수풍뎅이 뿔: 이마에서 크게 휘어 올라감 (투구 쓰면 뿔만 튀어나옴)
    c.polygon([(36, 17), (46, 16), (54, 6), (60, 0), (64, 3), (56, 8), (50, 18)], shell)
    c.polygon([(52, 9), (56, 2), (58, 3), (55, 10)], shell_shine)
    # 열혈 빨간 머리띠 + 뒤로 휘날리는 끈
    c.fill(lambda x, y: 21 <= y < 26 and ((x - 38) / 23) ** 2 + ((y - 34) / 20) ** 2 <= 1.0, hex_color("#E5363C"))
    c.polygon([(16, 23), (6, 17), (5, 22)], hex_color("#E5363C"))
    c.polygon([(16, 25), (4, 28), (7, 31)], hex_color("#C42A31"))
    # 냥코식 무표정 점 눈 + 열혈 화난 눈썹
    c.ellipse(41, 35, 1.6, 2.6, INK)
    c.ellipse(52, 35, 1.6, 2.6, INK)
    line(c, 37, 29, 44, 31, INK, 2)
    line(c, 49, 31, 56, 29, INK, 2)
    # 소리 지르는 작은 입 + 볼
    c.polygon([(45, 42), (51, 42), (48, 47)], hex_color("#7A1420"))
    c.ellipse(36, 41, 2.5, 1.5, hex_color("#FFB0A8"))
    c.ellipse(58, 40, 2, 1.5, hex_color("#FFB0A8"))
    c.outline(INK, thickness=2)
    return c


def make_bald_eagle():
    # 대머리 독수리 (레벨 도둑). 냥코 대전쟁의 근육다리 냥처럼 쓸데없이 긴 다리 + 선글라스.
    c = Canvas(84, 108)
    skin = hex_color("#FFE0D2")
    feather = hex_color("#5A3820")
    # 쓸데없이 늠름한 다리
    line(c, 34, 66, 30, 84, skin, 7)
    line(c, 30, 84, 34, 100, skin, 6)
    line(c, 48, 66, 52, 84, skin, 7)
    line(c, 52, 84, 48, 100, skin, 6)
    for fx in (34, 48):
        stick(c, fx, 101, fx - 6, 105, 2)
        stick(c, fx, 101, fx + 6, 105, 2)
        stick(c, fx, 101, fx, 106, 2)
    # 몸통 (갈색 깃털) + 흰 목털
    c.ellipse(41, 58, 18, 14, feather)
    c.ellipse(41, 44, 13, 6, hex_color("#FFFFFF"))
    # 날개 팔짱 (막대 팔 대신 날개)
    c.polygon([(24, 50), (12, 62), (30, 64)], hex_color("#3E2614"))
    c.polygon([(58, 50), (70, 62), (52, 64)], hex_color("#3E2614"))
    line(c, 26, 60, 56, 60, hex_color("#2A190C"), 2)
    # 대머리 (번들) + 머리카락 세 가닥
    c.ellipse(42, 26, 17, 17, skin)
    c.ellipse(35, 15, 5, 2.5, hex_color("#FFFFFF"))
    for i, dx in enumerate((-3, 0, 3)):
        line(c, 42 + dx, 9, 42 + dx + (i - 1) * 2, 4, INK, 1)
    # 선글라스 + 반짝
    c.rect(29, 22, 56, 28, INK)
    c.set(32, 23, hex_color("#FFFFFF"))
    c.set(48, 23, hex_color("#FFFFFF"))
    # 거대 갈고리 부리 + 비웃는 입꼬리
    c.polygon([(54, 29), (72, 30), (76, 36), (72, 41), (70, 36), (56, 36)], hex_color("#F2B21C"))
    line(c, 56, 36, 66, 35, hex_color("#8A5A10"), 1)
    c.outline(INK, thickness=2)
    return c


def make_ui_panel():
    # UI 9-슬라이스 패널: 둥근 사각형, 굵은 검은 테두리. 색은 HUD에서 틴트
    c = Canvas(24, 24)
    def inside(x, y):
        cx = min(max(x, 6), 18)
        cy = min(max(y, 6), 18)
        return (x - cx) ** 2 + (y - cy) ** 2 <= 36
    c.fill(inside, hex_color("#FFFFFF"))
    c.fill(lambda x, y: inside(x, y) and y > 15, hex_color("#D8D8D8"))
    c.outline(INK, thickness=2)
    return c


def make_ui_coin():
    c = Canvas(16, 16)
    c.circle(8, 8, 6.5, hex_color("#FFD23F"))
    c.circle(8, 8, 4.5, hex_color("#FFB000"))
    c.rect(7, 5, 9, 11, hex_color("#FFF3A0"))
    c.outline(INK)
    return c


def make_ui_skull():
    c = Canvas(16, 16)
    c.circle(8, 7, 6, hex_color("#FFFFFF"))
    c.rect(5, 10, 11, 14, hex_color("#FFFFFF"))
    c.circle(6, 7, 1.6, INK)
    c.circle(10, 7, 1.6, INK)
    c.set(8, 10, INK)
    c.outline(INK)
    return c


def make_ui_arrow():
    c = Canvas(16, 16)
    c.polygon([(8, 1), (15, 8), (11, 8), (11, 15), (5, 15), (5, 8), (1, 8)], hex_color("#7CF07C"))
    c.outline(INK)
    return c


def make_ui_flame():
    c = Canvas(16, 16)
    c.polygon([(8, 0), (13, 7), (14, 11), (11, 15), (5, 15), (2, 11), (4, 6), (6, 9)], hex_color("#FF5A1F"))
    c.polygon([(8, 6), (11, 11), (9, 14), (6, 14), (5, 11)], hex_color("#FFD23F"))
    c.outline(INK)
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
    "T_UI_Panel": make_ui_panel,
    "T_UI_Coin": make_ui_coin,
    "T_UI_Skull": make_ui_skull,
    "T_UI_Arrow": make_ui_arrow,
    "T_UI_Flame": make_ui_flame,
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
