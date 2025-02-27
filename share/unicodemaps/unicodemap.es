#region: Spain (Espagna)
#format: <UNICODE>, <ROW><COL>, <MODIFIERS>
# <UNICODE>: hexadecimal unicode number or DEADKEY<N>
# <ROW>: row in keyboard matrix (hexadecimal: 0-B)
# <COL>: column in keyboard matrix (0-7)
# <MODIFIERS>: space separated list of modifiers:
#              SHIFT CTRL GRAPH CODE
#
# Example characters in comments are encoded in UTF-8
#
DEADKEY1, 25,
0000, 02, CTRL SHIFT  # ^@
0001, 26, CTRL        # ^A
0002, 27, CTRL        # ^B
0003, 30, CTRL        # ^C
0004, 31, CTRL        # ^D
0005, 32, CTRL        # ^E
0006, 33, CTRL        # ^F
0007, 34, CTRL        # ^G
0008, 75,             # Backspace
0009, 73,             # Tab
000a, 37, CTRL        # ^J
000b, 81,             # Home (is Home a unicode character?)
000c, 41, CTRL        # ^L
000d, 77,             # Enter/CR
000e, 43, CTRL        # ^N
000f, 44, CTRL        # ^O
0010, 45, CTRL        # ^P
0011, 46, CTRL        # ^Q
0012, 82,             # Insert (is Insert a unicode character?)
0013, 50, CTRL        # ^S
0014, 51, CTRL        # ^T
0015, 52, CTRL        # ^U
0016, 53, CTRL        # ^V
0017, 54, CTRL        # ^W
0018, 76,             # Select (is Select a unicode character?)
0019, 56, CTRL        # ^Y
001a, 57, CTRL        # ^Z
001b, 72,             # Escape(SDL maps ESC and ^[ to this code)
001c, 87,             # Right (SDL maps ^\ to this code)
001d, 84,             # Left  (SDL maps ^] to this code)
001e, 85,             # Up    (SDL maps ^6 to this code)
001f, 86,             # Down  (SDL maps ^/ to this code)
0020, 80,
0021, 01, SHIFT
0022, 20, SHIFT
0023, 03, SHIFT
0024, 04, SHIFT
0025, 05, SHIFT
0026, 07, SHIFT
0027, 20,
0028, 11, SHIFT
0029, 00, SHIFT
002a, 10, SHIFT
002b, 13, SHIFT
002c, 22,
002d, 12,
002e, 23,
002f, 24,
0030, 00,
0031, 01,
0032, 02,
0033, 03,
0034, 04,
0035, 05,
0036, 06,
0037, 07,
0038, 10,
0039, 11,
003a, 21, SHIFT
003b, 21,
003c, 22, SHIFT
003d, 13,
003e, 23, SHIFT
003f, 24, SHIFT
0040, 02, SHIFT
0041, 26, SHIFT
0042, 27, SHIFT
0043, 30, SHIFT
0044, 31, SHIFT
0045, 32, SHIFT
0046, 33, SHIFT
0047, 34, SHIFT
0048, 35, SHIFT
0049, 36, SHIFT
004a, 37, SHIFT
004b, 40, SHIFT
004c, 41, SHIFT
004d, 42, SHIFT
004e, 43, SHIFT
004f, 44, SHIFT
0050, 45, SHIFT
0051, 46, SHIFT
0052, 47, SHIFT
0053, 50, SHIFT
0054, 51, SHIFT
0055, 52, SHIFT
0056, 53, SHIFT
0057, 54, SHIFT
0058, 55, SHIFT
0059, 56, SHIFT
005a, 57, SHIFT
005b, 15,
005c, 14,
005d, 16,
005e, 06, SHIFT
005f, 12, SHIFT
#0060, 21,             # `
0061, 26,
0062, 27,
0063, 30,
0064, 31,
0065, 32,
0066, 33,
0067, 34,
0068, 35,
0069, 36,
006a, 37,
006b, 40,
006c, 41,
006d, 42,
006e, 43,
006f, 44,
0070, 45,
0071, 46,
0072, 47,
0073, 50,
0074, 51,
0075, 52,
0076, 53,
0077, 54,
0078, 55,
0079, 56,
007a, 57,
007b, 15, SHIFT
007c, 14, SHIFT       # |
007d, 16, SHIFT
007f, 83,             # Delete
00a0, 80,
00a1, 01, SHIFT CODE
00a2, 04, CODE
00a3, 04, SHIFT CODE
00a5, 05, SHIFT CODE
00a7, 03, CODE
00aa, 23, CODE
00ab, 22, SHIFT GRAPH
00ac, 56, SHIFT GRAPH
00b0, 57, SHIFT GRAPH
00b1, 13, GRAPH
00b2, 02, SHIFT GRAPH
00b5, 42, CODE
00b6, 03, SHIFT CODE  # ¶
00b7, 30, SHIFT GRAPH
00ba, 24, CODE
00bb, 23, SHIFT GRAPH
00bc, 01, GRAPH
00bd, 02, GRAPH
00bf, 24, SHIFT CODE
00c3, 35, SHIFT CODE
00c4, 26, SHIFT CODE
00c5, 22, SHIFT CODE
00c6, 37, SHIFT CODE
00c7, 11, SHIFT CODE
00c9, 52, SHIFT CODE
00d1, 17, SHIFT
00d5, 41, SHIFT CODE
00d6, 33, SHIFT CODE
00dc, 34, SHIFT CODE
00df, 07, CODE
00e0, 57, CODE
00e1, 56, CODE
00e2, 46, CODE
00e3, 35, CODE
00e4, 26, CODE
00e5, 22, CODE
00e6, 37, CODE
00e7, 11, CODE
00e8, 55, CODE
00e9, 52, CODE
00ea, 54, CODE
00eb, 50, CODE
00ec, 30, CODE
00ed, 36, CODE
00ee, 32, CODE
00ef, 31, CODE
00f1, 17,
00f2, 53, CODE
00f3, 44, CODE
00f4, 47, CODE
00f5, 41, CODE
00f6, 33, CODE
00f7, 24, SHIFT GRAPH
00f9, 27, CODE
00fa, 45, CODE
00fb, 51, CODE
00fc, 34, CODE
00ff, 05, CODE
0128, 40, SHIFT CODE
0129, 40, CODE
0132, 20, SHIFT CODE  # Ĳ
0133, 20, CODE        # ĳ
0168, 17, SHIFT CODE
0169, 17, CODE
0192, 01, CODE
0393, 10, SHIFT CODE
0394, 00, SHIFT CODE  # Δ
03a3, 21, SHIFT CODE
03a6, 15, SHIFT CODE
03a9, 16, SHIFT CODE
03b1, 06, CODE
03b4, 00, CODE
03b5, 12, CODE
03b8, 13, CODE        # θ
03c0, 45, SHIFT CODE
03c3, 21, CODE
03c4, 10, CODE
03c6, 15, CODE
03c9, 16, CODE        # ω
2022, 55, SHIFT GRAPH # •
2027, 11, GRAPH       # ‧
2030, 05, GRAPH       # ‰
207f, 03, SHIFT GRAPH
20a7, 02, SHIFT CODE
221a, 07, GRAPH
221e, 10, GRAPH
2229, 04, GRAPH
223d, 21, GRAPH       # ∽
2248, 21, SHIFT GRAPH
2260, 02, CODE        # ≠
2261, 13, SHIFT GRAPH
2264, 22, GRAPH
2265, 23, GRAPH
2310, 47, SHIFT GRAPH
2320, 06, GRAPH
2321, 06, SHIFT GRAPH
2500, 12, GRAPH       # ─
2502, 14, SHIFT GRAPH # │
250c, 47, GRAPH       # ┌
2510, 56, GRAPH       # ┐
2514, 53, GRAPH       # └
2518, 43, GRAPH       # ┘
251c, 33, GRAPH       # ├
2524, 35, GRAPH       # ┤
252c, 51, GRAPH       # ┬
2534, 27, GRAPH       # ┴
253c, 34, GRAPH       # ┼
2571, 24, GRAPH       # ╱
2572, 14, GRAPH       # ╲
2573, 55, GRAPH       # ╳
2580, 36, SHIFT GRAPH # ▀
2582, 52, GRAPH       # ▂
2584, 36, GRAPH       # ▄
2586, 44, GRAPH       # ▆
2588, 45, GRAPH       # █
258a, 41, GRAPH       # ▊
258c, 40, GRAPH       # ▌
258e, 37, GRAPH       # ▎
2590, 40, SHIFT GRAPH # ▐
2594, 44, SHIFT GRAPH # ▔
2595, 41, SHIFT GRAPH # ▕
2596, 35, SHIFT GRAPH # ▖
2597, 33, SHIFT GRAPH # ▗
2598, 43, SHIFT GRAPH # ▘
259a, 31, SHIFT GRAPH # ▚
259d, 53, SHIFT GRAPH # ▝
259e, 31, GRAPH       # ▞
25a7, 46, GRAPH       # ▧
25a8, 46, SHIFT GRAPH # ▨
25a9, 45, SHIFT GRAPH # ▩
25b2, 32, SHIFT GRAPH # ▲
25b6, 54, GRAPH       # ▶
25bc, 32, GRAPH       # ▼
25c0, 54, SHIFT GRAPH # ◀
25c7, 30, GRAPH       # ◇
25cb, 00, GRAPH       # ○
25d8, 11, SHIFT GRAPH
25d9, 00, SHIFT GRAPH
25fc, 26, SHIFT GRAPH # ◼
25fe, 26, GRAPH       # ◾
263a, 15, GRAPH
263b, 15, SHIFT GRAPH
263c, 57, GRAPH
2640, 42, SHIFT GRAPH
2642, 42, GRAPH
2660, 17, GRAPH
2663, 20, GRAPH
2665, 20, SHIFT GRAPH
2666, 17, SHIFT GRAPH
266a, 16, GRAPH
266b, 16, SHIFT GRAPH
29d3, 50, GRAPH       # ⧓
29d7, 50, SHIFT GRAPH # ⧗
