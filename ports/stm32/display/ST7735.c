#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "spi.h"

#if MICROPY_HW_HAS_ST7735

#include "pin.h"
#include "bufhelper.h"
#include "spi.h"
#include "font_petme128_8x8.h"
#include "ST7735.h"
#include "extmod/modframebuf.h"

#define LCD_INSTR (0)
//#define LCD_CHAR_BUF_W (16)
//#define LCD_CHAR_BUF_H (4)

// color are converted to RGB565 using the COL macro
const uint16_t default_palette16[] = {
    COL(0x000000), // 0
    COL(0xffffff), // 1
    COL(0xff2121), // 2
    COL(0xff93c4), // 3
    COL(0xff8135), // 4
    COL(0xfff609), // 5
    COL(0x249ca3), // 6
    COL(0x78dc52), // 7
    COL(0x003fad), // 8
    COL(0x87f2ff), // 9
    COL(0x8e2ec4), // 10
    COL(0xa4839f), // 11
    COL(0x5c406c), // 12
    COL(0xe5cdc4), // 13
    COL(0x91463d), // 14
    COL(0x000000), // 15
};

const uint16_t c64_palette16[] = {
    COL(0x000000), // 0
    COL(0xffffff), // 1
    COL(0x883932), // 2
    COL(0x67b6bd), // 3
    COL(0x8b3f96), // 4
    COL(0x55a049), // 5
    COL(0x40318d), // 6
    COL(0xbfce72), // 7
    COL(0x8b5429), // 8
    COL(0x574200), // 9
    COL(0xb86962), // 10
    COL(0x505050), // 11
    COL(0x787878), // 12
    COL(0x94e089), // 13
    COL(0x7869c4), // 14
    COL(0x9f9f9f), // 15
};

const uint16_t apple2_palette16[] = {
	COL(0x000000), // 0
	COL(0x6C2940), // 1
	COL(0x403578), // 2
	COL(0xD93CF0), // 3
	COL(0x135740), // 4
	COL(0x808080), // 5
	COL(0x2697F0), // 6
	COL(0xBFB4F8), // 7
	COL(0x404B07), // 8
	COL(0xD9680F), // 9
	COL(0x808080), // 10
	COL(0xECA8BF), // 11
	COL(0x26C30F), // 12
	COL(0xBFCA87), // 13
	COL(0x93D6BF), // 14
	COL(0xFFFFFF), // 15
};

const uint16_t cga_palette16[] = {
	COL(0x000000), // 0
	COL(0x0000AA), // 1
	COL(0x00AA00), // 2
	COL(0x00AAAA), // 3
	COL(0xAA0000), // 4
	COL(0xAA00AA), // 5
	COL(0xAA5500), // 6
	COL(0xAAAAAA), // 7
	COL(0x555555), // 8
	COL(0x5555FF), // 9
	COL(0x55FF55), // 10
	COL(0x55FFFF), // 11
	COL(0xFF5555), // 12
	COL(0xFF55FF), // 13
	COL(0xFFFF55), // 14
	COL(0xFFFFFF), // 15
};

const uint16_t msx_palette16[] = {
	COL(0x000000), // 0
	COL(0x000000), // 1
	COL(0x3EB849), // 2
	COL(0x74D07D), // 3
	COL(0x5955E0), // 4
	COL(0x8076F1), // 5
	COL(0xB95E51), // 6
	COL(0x65DBEF), // 7
	COL(0xDB6559), // 8
	COL(0xFF897D), // 9
	COL(0xCCC35E), // 10
	COL(0xDED087), // 11
	COL(0x3AA241), // 12
	COL(0xB766B5), // 13
	COL(0xCCCCCC), // 14
	COL(0xFFFFFF), // 15
};

const uint16_t spectrum_palette16[] = {
	COL(0x000000), // 0
	COL(0x0000C0), // 1
	COL(0xC00000), // 2
	COL(0xC000C0), // 3
	COL(0x00C000), // 4
	COL(0x00C0C0), // 5
	COL(0xC0C000), // 6
	COL(0xC0C0C0), // 7
	COL(0x000000), // 8
	COL(0x0000FF), // 9
	COL(0xFF0000), // 10
	COL(0xFF00FF), // 11
	COL(0x00FF00), // 12
	COL(0x00FFFF), // 13
	COL(0xFFFF00), // 14
	COL(0xFFFFFF), // 15
};

const uint16_t adaptive_gs16_palette16[] = {
	COL(0x040204), // 0
	COL(0x888888), // 1
	COL(0xC8C7C8), // 2
	COL(0x484848), // 3
	COL(0xA8A8A8), // 4
	COL(0xE8E8E8), // 5
	COL(0x686868), // 6
	COL(0x282728), // 7
	COL(0x989898), // 8
	COL(0x171917), // 9
	COL(0xD8D7D8), // 10
	COL(0x575857), // 11
	COL(0xB8B8B8), // 12
	COL(0xFCFEFC), // 13
	COL(0x787878), // 14
	COL(0x393839), // 15
};

const uint16_t rgb676_palette256[] = {
	COL(0x000000), // 0
	COL(0x000033), // 1
	COL(0x000066), // 2
	COL(0x000099), // 3
	COL(0x0000CC), // 4
	COL(0x0000FF), // 5
	COL(0x002B00), // 6
	COL(0x002B33), // 7
	COL(0x002B66), // 8
	COL(0x002B99), // 9
	COL(0x002BCC), // 10
	COL(0x002BFF), // 11
	COL(0x005500), // 12
	COL(0x005533), // 13
	COL(0x005566), // 14
	COL(0x005599), // 15
	COL(0x0055CC), // 16
	COL(0x0055FF), // 17
	COL(0x008000), // 18
	COL(0x008033), // 19
	COL(0x008066), // 20
	COL(0x008099), // 21
	COL(0x0080CC), // 22
	COL(0x0080FF), // 23
	COL(0x00AA00), // 24
	COL(0x00AA33), // 25
	COL(0x00AA66), // 26
	COL(0x00AA99), // 27
	COL(0x00AACC), // 28
	COL(0x00AAFF), // 29
	COL(0x00D500), // 30
	COL(0x00D533), // 31
	COL(0x00D566), // 32
	COL(0x00D599), // 33
	COL(0x00D5CC), // 34
	COL(0x00D5FF), // 35
	COL(0x00FF00), // 36
	COL(0x00FF33), // 37
	COL(0x00FF66), // 38
	COL(0x00FF99), // 39
	COL(0x00FFCC), // 40
	COL(0x00FFFF), // 41
	COL(0x330000), // 42
	COL(0x330033), // 43
	COL(0x330066), // 44
	COL(0x330099), // 45
	COL(0x3300CC), // 46
	COL(0x3300FF), // 47
	COL(0x332B00), // 48
	COL(0x332B33), // 49
	COL(0x332B66), // 50
	COL(0x332B99), // 51
	COL(0x332BCC), // 52
	COL(0x332BFF), // 53
	COL(0x335500), // 54
	COL(0x335533), // 55
	COL(0x335566), // 56
	COL(0x335599), // 57
	COL(0x3355CC), // 58
	COL(0x3355FF), // 59
	COL(0x338000), // 60
	COL(0x338033), // 61
	COL(0x338066), // 62
	COL(0x338099), // 63
	COL(0x3380CC), // 64
	COL(0x3380FF), // 65
	COL(0x33AA00), // 66
	COL(0x33AA33), // 67
	COL(0x33AA66), // 68
	COL(0x33AA99), // 69
	COL(0x33AACC), // 70
	COL(0x33AAFF), // 71
	COL(0x33D500), // 72
	COL(0x33D533), // 73
	COL(0x33D566), // 74
	COL(0x33D599), // 75
	COL(0x33D5CC), // 76
	COL(0x33D5FF), // 77
	COL(0x33FF00), // 78
	COL(0x33FF33), // 79
	COL(0x33FF66), // 80
	COL(0x33FF99), // 81
	COL(0x33FFCC), // 82
	COL(0x33FFFF), // 83
	COL(0x660000), // 84
	COL(0x660033), // 85
	COL(0x660066), // 86
	COL(0x660099), // 87
	COL(0x6600CC), // 88
	COL(0x6600FF), // 89
	COL(0x662B00), // 90
	COL(0x662B33), // 91
	COL(0x662B66), // 92
	COL(0x662B99), // 93
	COL(0x662BCC), // 94
	COL(0x662BFF), // 95
	COL(0x665500), // 96
	COL(0x665533), // 97
	COL(0x665566), // 98
	COL(0x665599), // 99
	COL(0x6655CC), // 100
	COL(0x6655FF), // 101
	COL(0x668000), // 102
	COL(0x668033), // 103
	COL(0x668066), // 104
	COL(0x668099), // 105
	COL(0x6680CC), // 106
	COL(0x6680FF), // 107
	COL(0x66AA00), // 108
	COL(0x66AA33), // 109
	COL(0x66AA66), // 110
	COL(0x66AA99), // 111
	COL(0x66AACC), // 112
	COL(0x66AAFF), // 113
	COL(0x66D500), // 114
	COL(0x66D533), // 115
	COL(0x66D566), // 116
	COL(0x66D599), // 117
	COL(0x66D5CC), // 118
	COL(0x66D5FF), // 119
	COL(0x66FF00), // 120
	COL(0x66FF33), // 121
	COL(0x66FF66), // 122
	COL(0x66FF99), // 123
	COL(0x66FFCC), // 124
	COL(0x66FFFF), // 125
	COL(0x990000), // 126
	COL(0x990033), // 127
	COL(0x990066), // 128
	COL(0x990099), // 129
	COL(0x9900CC), // 130
	COL(0x9900FF), // 131
	COL(0x992B00), // 132
	COL(0x992B33), // 133
	COL(0x992B66), // 134
	COL(0x992B99), // 135
	COL(0x992BCC), // 136
	COL(0x992BFF), // 137
	COL(0x995500), // 138
	COL(0x995533), // 139
	COL(0x995566), // 140
	COL(0x995599), // 141
	COL(0x9955CC), // 142
	COL(0x9955FF), // 143
	COL(0x998000), // 144
	COL(0x998033), // 145
	COL(0x998066), // 146
	COL(0x998099), // 147
	COL(0x9980CC), // 148
	COL(0x9980FF), // 149
	COL(0x99AA00), // 150
	COL(0x99AA33), // 151
	COL(0x99AA66), // 152
	COL(0x99AA99), // 153
	COL(0x99AACC), // 154
	COL(0x99AAFF), // 155
	COL(0x99D500), // 156
	COL(0x99D533), // 157
	COL(0x99D566), // 158
	COL(0x99D599), // 159
	COL(0x99D5CC), // 160
	COL(0x99D5FF), // 161
	COL(0x99FF00), // 162
	COL(0x99FF33), // 163
	COL(0x99FF66), // 164
	COL(0x99FF99), // 165
	COL(0x99FFCC), // 166
	COL(0x99FFFF), // 167
	COL(0xCC0000), // 168
	COL(0xCC0033), // 169
	COL(0xCC0066), // 170
	COL(0xCC0099), // 171
	COL(0xCC00CC), // 172
	COL(0xCC00FF), // 173
	COL(0xCC2B00), // 174
	COL(0xCC2B33), // 175
	COL(0xCC2B66), // 176
	COL(0xCC2B99), // 177
	COL(0xCC2BCC), // 178
	COL(0xCC2BFF), // 179
	COL(0xCC5500), // 180
	COL(0xCC5533), // 181
	COL(0xCC5566), // 182
	COL(0xCC5599), // 183
	COL(0xCC55CC), // 184
	COL(0xCC55FF), // 185
	COL(0xCC8000), // 186
	COL(0xCC8033), // 187
	COL(0xCC8066), // 188
	COL(0xCC8099), // 189
	COL(0xCC80CC), // 190
	COL(0xCC80FF), // 191
	COL(0xCCAA00), // 192
	COL(0xCCAA33), // 193
	COL(0xCCAA66), // 194
	COL(0xCCAA99), // 195
	COL(0xCCAACC), // 196
	COL(0xCCAAFF), // 197
	COL(0xCCD500), // 198
	COL(0xCCD533), // 199
	COL(0xCCD566), // 200
	COL(0xCCD599), // 201
	COL(0xCCD5CC), // 202
	COL(0xCCD5FF), // 203
	COL(0xCCFF00), // 204
	COL(0xCCFF33), // 205
	COL(0xCCFF66), // 206
	COL(0xCCFF99), // 207
	COL(0xCCFFCC), // 208
	COL(0xCCFFFF), // 209
	COL(0xFF0000), // 210
	COL(0xFF0033), // 211
	COL(0xFF0066), // 212
	COL(0xFF0099), // 213
	COL(0xFF00CC), // 214
	COL(0xFF00FF), // 215
	COL(0xFF2B00), // 216
	COL(0xFF2B33), // 217
	COL(0xFF2B66), // 218
	COL(0xFF2B99), // 219
	COL(0xFF2BCC), // 220
	COL(0xFF2BFF), // 221
	COL(0xFF5500), // 222
	COL(0xFF5533), // 223
	COL(0xFF5566), // 224
	COL(0xFF5599), // 225
	COL(0xFF55CC), // 226
	COL(0xFF55FF), // 227
	COL(0xFF8000), // 228
	COL(0xFF8033), // 229
	COL(0xFF8066), // 230
	COL(0xFF8099), // 231
	COL(0xFF80CC), // 232
	COL(0xFF80FF), // 233
	COL(0xFFAA00), // 234
	COL(0xFFAA33), // 235
	COL(0xFFAA66), // 236
	COL(0xFFAA99), // 237
	COL(0xFFAACC), // 238
	COL(0xFFAAFF), // 239
	COL(0xFFD500), // 240
	COL(0xFFD533), // 241
	COL(0xFFD566), // 242
	COL(0xFFD599), // 243
	COL(0xFFD5CC), // 244
	COL(0xFFD5FF), // 245
	COL(0xFFFF00), // 246
	COL(0xFFFF33), // 247
	COL(0xFFFF66), // 248
	COL(0xFFFF99), // 249
	COL(0xFFFFCC), // 250
	COL(0xFFFFFF), // 251
	COL(0x000000), // 252
	COL(0x000000), // 253
	COL(0x000000), // 254
	COL(0x000000), // 255
};

const uint16_t rgb685_palette256[] = {
	COL(0x000000), // 0
	COL(0x000040), // 1
	COL(0x000080), // 2
	COL(0x0000C0), // 3
	COL(0x0000FF), // 4
	COL(0x002400), // 5
	COL(0x002440), // 6
	COL(0x002480), // 7
	COL(0x0024C0), // 8
	COL(0x0024FF), // 9
	COL(0x004900), // 10
	COL(0x004940), // 11
	COL(0x004980), // 12
	COL(0x0049C0), // 13
	COL(0x0049FF), // 14
	COL(0x006D00), // 15
	COL(0x006D40), // 16
	COL(0x006D80), // 17
	COL(0x006DC0), // 18
	COL(0x006DFF), // 19
	COL(0x009200), // 20
	COL(0x009240), // 21
	COL(0x009280), // 22
	COL(0x0092C0), // 23
	COL(0x0092FF), // 24
	COL(0x00B600), // 25
	COL(0x00B640), // 26
	COL(0x00B680), // 27
	COL(0x00B6C0), // 28
	COL(0x00B6FF), // 29
	COL(0x00DB00), // 30
	COL(0x00DB40), // 31
	COL(0x00DB80), // 32
	COL(0x00DBC0), // 33
	COL(0x00DBFF), // 34
	COL(0x00FF00), // 35
	COL(0x00FF40), // 36
	COL(0x00FF80), // 37
	COL(0x00FFC0), // 38
	COL(0x00FFFF), // 39
	COL(0x330000), // 40
	COL(0x330040), // 41
	COL(0x330080), // 42
	COL(0x3300C0), // 43
	COL(0x3300FF), // 44
	COL(0x332400), // 45
	COL(0x332440), // 46
	COL(0x332480), // 47
	COL(0x3324C0), // 48
	COL(0x3324FF), // 49
	COL(0x334900), // 50
	COL(0x334940), // 51
	COL(0x334980), // 52
	COL(0x3349C0), // 53
	COL(0x3349FF), // 54
	COL(0x336D00), // 55
	COL(0x336D40), // 56
	COL(0x336D80), // 57
	COL(0x336DC0), // 58
	COL(0x336DFF), // 59
	COL(0x339200), // 60
	COL(0x339240), // 61
	COL(0x339280), // 62
	COL(0x3392C0), // 63
	COL(0x3392FF), // 64
	COL(0x33B600), // 65
	COL(0x33B640), // 66
	COL(0x33B680), // 67
	COL(0x33B6C0), // 68
	COL(0x33B6FF), // 69
	COL(0x33DB00), // 70
	COL(0x33DB40), // 71
	COL(0x33DB80), // 72
	COL(0x33DBC0), // 73
	COL(0x33DBFF), // 74
	COL(0x33FF00), // 75
	COL(0x33FF40), // 76
	COL(0x33FF80), // 77
	COL(0x33FFC0), // 78
	COL(0x33FFFF), // 79
	COL(0x660000), // 80
	COL(0x660040), // 81
	COL(0x660080), // 82
	COL(0x6600C0), // 83
	COL(0x6600FF), // 84
	COL(0x662400), // 85
	COL(0x662440), // 86
	COL(0x662480), // 87
	COL(0x6624C0), // 88
	COL(0x6624FF), // 89
	COL(0x664900), // 90
	COL(0x664940), // 91
	COL(0x664980), // 92
	COL(0x6649C0), // 93
	COL(0x6649FF), // 94
	COL(0x666D00), // 95
	COL(0x666D40), // 96
	COL(0x666D80), // 97
	COL(0x666DC0), // 98
	COL(0x666DFF), // 99
	COL(0x669200), // 100
	COL(0x669240), // 101
	COL(0x669280), // 102
	COL(0x6692C0), // 103
	COL(0x6692FF), // 104
	COL(0x66B600), // 105
	COL(0x66B640), // 106
	COL(0x66B680), // 107
	COL(0x66B6C0), // 108
	COL(0x66B6FF), // 109
	COL(0x66DB00), // 110
	COL(0x66DB40), // 111
	COL(0x66DB80), // 112
	COL(0x66DBC0), // 113
	COL(0x66DBFF), // 114
	COL(0x66FF00), // 115
	COL(0x66FF40), // 116
	COL(0x66FF80), // 117
	COL(0x66FFC0), // 118
	COL(0x66FFFF), // 119
	COL(0x990000), // 120
	COL(0x990040), // 121
	COL(0x990080), // 122
	COL(0x9900C0), // 123
	COL(0x9900FF), // 124
	COL(0x992400), // 125
	COL(0x992440), // 126
	COL(0x992480), // 127
	COL(0x9924C0), // 128
	COL(0x9924FF), // 129
	COL(0x994900), // 130
	COL(0x994940), // 131
	COL(0x994980), // 132
	COL(0x9949C0), // 133
	COL(0x9949FF), // 134
	COL(0x996D00), // 135
	COL(0x996D40), // 136
	COL(0x996D80), // 137
	COL(0x996DC0), // 138
	COL(0x996DFF), // 139
	COL(0x999200), // 140
	COL(0x999240), // 141
	COL(0x999280), // 142
	COL(0x9992C0), // 143
	COL(0x9992FF), // 144
	COL(0x99B600), // 145
	COL(0x99B640), // 146
	COL(0x99B680), // 147
	COL(0x99B6C0), // 148
	COL(0x99B6FF), // 149
	COL(0x99DB00), // 150
	COL(0x99DB40), // 151
	COL(0x99DB80), // 152
	COL(0x99DBC0), // 153
	COL(0x99DBFF), // 154
	COL(0x99FF00), // 155
	COL(0x99FF40), // 156
	COL(0x99FF80), // 157
	COL(0x99FFC0), // 158
	COL(0x99FFFF), // 159
	COL(0xCC0000), // 160
	COL(0xCC0040), // 161
	COL(0xCC0080), // 162
	COL(0xCC00C0), // 163
	COL(0xCC00FF), // 164
	COL(0xCC2400), // 165
	COL(0xCC2440), // 166
	COL(0xCC2480), // 167
	COL(0xCC24C0), // 168
	COL(0xCC24FF), // 169
	COL(0xCC4900), // 170
	COL(0xCC4940), // 171
	COL(0xCC4980), // 172
	COL(0xCC49C0), // 173
	COL(0xCC49FF), // 174
	COL(0xCC6D00), // 175
	COL(0xCC6D40), // 176
	COL(0xCC6D80), // 177
	COL(0xCC6DC0), // 178
	COL(0xCC6DFF), // 179
	COL(0xCC9200), // 180
	COL(0xCC9240), // 181
	COL(0xCC9280), // 182
	COL(0xCC92C0), // 183
	COL(0xCC92FF), // 184
	COL(0xCCB600), // 185
	COL(0xCCB640), // 186
	COL(0xCCB680), // 187
	COL(0xCCB6C0), // 188
	COL(0xCCB6FF), // 189
	COL(0xCCDB00), // 190
	COL(0xCCDB40), // 191
	COL(0xCCDB80), // 192
	COL(0xCCDBC0), // 193
	COL(0xCCDBFF), // 194
	COL(0xCCFF00), // 195
	COL(0xCCFF40), // 196
	COL(0xCCFF80), // 197
	COL(0xCCFFC0), // 198
	COL(0xCCFFFF), // 199
	COL(0xFF0000), // 200
	COL(0xFF0040), // 201
	COL(0xFF0080), // 202
	COL(0xFF00C0), // 203
	COL(0xFF00FF), // 204
	COL(0xFF2400), // 205
	COL(0xFF2440), // 206
	COL(0xFF2480), // 207
	COL(0xFF24C0), // 208
	COL(0xFF24FF), // 209
	COL(0xFF4900), // 210
	COL(0xFF4940), // 211
	COL(0xFF4980), // 212
	COL(0xFF49C0), // 213
	COL(0xFF49FF), // 214
	COL(0xFF6D00), // 215
	COL(0xFF6D40), // 216
	COL(0xFF6D80), // 217
	COL(0xFF6DC0), // 218
	COL(0xFF6DFF), // 219
	COL(0xFF9200), // 220
	COL(0xFF9240), // 221
	COL(0xFF9280), // 222
	COL(0xFF92C0), // 223
	COL(0xFF92FF), // 224
	COL(0xFFB600), // 225
	COL(0xFFB640), // 226
	COL(0xFFB680), // 227
	COL(0xFFB6C0), // 228
	COL(0xFFB6FF), // 229
	COL(0xFFDB00), // 230
	COL(0xFFDB40), // 231
	COL(0xFFDB80), // 232
	COL(0xFFDBC0), // 233
	COL(0xFFDBFF), // 234
	COL(0xFFFF00), // 235
	COL(0xFFFF40), // 236
	COL(0xFFFF80), // 237
	COL(0xFFFFC0), // 238
	COL(0xFFFFFF), // 239
	COL(0x000000), // 240
	COL(0x000000), // 241
	COL(0x000000), // 242
	COL(0x000000), // 243
	COL(0x000000), // 244
	COL(0x000000), // 245
	COL(0x000000), // 246
	COL(0x000000), // 247
	COL(0x000000), // 248
	COL(0x000000), // 249
	COL(0x000000), // 250
	COL(0x000000), // 251
	COL(0x000000), // 252
	COL(0x000000), // 253
	COL(0x000000), // 254
	COL(0x000000), // 255
};

const uint16_t rgb884_palette256[] = {
	COL(0x000000), // 0
	COL(0x000055), // 1
	COL(0x0000AA), // 2
	COL(0x0000FF), // 3
	COL(0x002400), // 4
	COL(0x002455), // 5
	COL(0x0024AA), // 6
	COL(0x0024FF), // 7
	COL(0x004900), // 8
	COL(0x004955), // 9
	COL(0x0049AA), // 10
	COL(0x0049FF), // 11
	COL(0x006D00), // 12
	COL(0x006D55), // 13
	COL(0x006DAA), // 14
	COL(0x006DFF), // 15
	COL(0x009200), // 16
	COL(0x009255), // 17
	COL(0x0092AA), // 18
	COL(0x0092FF), // 19
	COL(0x00B600), // 20
	COL(0x00B655), // 21
	COL(0x00B6AA), // 22
	COL(0x00B6FF), // 23
	COL(0x00DB00), // 24
	COL(0x00DB55), // 25
	COL(0x00DBAA), // 26
	COL(0x00DBFF), // 27
	COL(0x00FF00), // 28
	COL(0x00FF55), // 29
	COL(0x00FFAA), // 30
	COL(0x00FFFF), // 31
	COL(0x240000), // 32
	COL(0x240055), // 33
	COL(0x2400AA), // 34
	COL(0x2400FF), // 35
	COL(0x242400), // 36
	COL(0x242455), // 37
	COL(0x2424AA), // 38
	COL(0x2424FF), // 39
	COL(0x244900), // 40
	COL(0x244955), // 41
	COL(0x2449AA), // 42
	COL(0x2449FF), // 43
	COL(0x246D00), // 44
	COL(0x246D55), // 45
	COL(0x246DAA), // 46
	COL(0x246DFF), // 47
	COL(0x249200), // 48
	COL(0x249255), // 49
	COL(0x2492AA), // 50
	COL(0x2492FF), // 51
	COL(0x24B600), // 52
	COL(0x24B655), // 53
	COL(0x24B6AA), // 54
	COL(0x24B6FF), // 55
	COL(0x24DB00), // 56
	COL(0x24DB55), // 57
	COL(0x24DBAA), // 58
	COL(0x24DBFF), // 59
	COL(0x24FF00), // 60
	COL(0x24FF55), // 61
	COL(0x24FFAA), // 62
	COL(0x24FFFF), // 63
	COL(0x490000), // 64
	COL(0x490055), // 65
	COL(0x4900AA), // 66
	COL(0x4900FF), // 67
	COL(0x492400), // 68
	COL(0x492455), // 69
	COL(0x4924AA), // 70
	COL(0x4924FF), // 71
	COL(0x494900), // 72
	COL(0x494955), // 73
	COL(0x4949AA), // 74
	COL(0x4949FF), // 75
	COL(0x496D00), // 76
	COL(0x496D55), // 77
	COL(0x496DAA), // 78
	COL(0x496DFF), // 79
	COL(0x499200), // 80
	COL(0x499255), // 81
	COL(0x4992AA), // 82 //71 [57, 138, 188] is mapped to index 82 which is [73, 146, 170] [49, 92, AA]
	COL(0x4992FF), // 83
	COL(0x49B600), // 84
	COL(0x49B655), // 85
	COL(0x49B6AA), // 86
	COL(0x49B6FF), // 87
	COL(0x49DB00), // 88
	COL(0x49DB55), // 89
	COL(0x49DBAA), // 90
	COL(0x49DBFF), // 91
	COL(0x49FF00), // 92
	COL(0x49FF55), // 93
	COL(0x49FFAA), // 94
	COL(0x49FFFF), // 95
	COL(0x6D0000), // 96
	COL(0x6D0055), // 97
	COL(0x6D00AA), // 98
	COL(0x6D00FF), // 99
	COL(0x6D2400), // 100
	COL(0x6D2455), // 101
	COL(0x6D24AA), // 102
	COL(0x6D24FF), // 103
	COL(0x6D4900), // 104
	COL(0x6D4955), // 105
	COL(0x6D49AA), // 106
	COL(0x6D49FF), // 107
	COL(0x6D6D00), // 108
	COL(0x6D6D55), // 109
	COL(0x6D6DAA), // 110
	COL(0x6D6DFF), // 111
	COL(0x6D9200), // 112
	COL(0x6D9255), // 113
	COL(0x6D92AA), // 114
	COL(0x6D92FF), // 115
	COL(0x6DB600), // 116
	COL(0x6DB655), // 117
	COL(0x6DB6AA), // 118
	COL(0x6DB6FF), // 119
	COL(0x6DDB00), // 120
	COL(0x6DDB55), // 121
	COL(0x6DDBAA), // 122
	COL(0x6DDBFF), // 123
	COL(0x6DFF00), // 124
	COL(0x6DFF55), // 125
	COL(0x6DFFAA), // 126
	COL(0x6DFFFF), // 127
	COL(0x920000), // 128
	COL(0x920055), // 129
	COL(0x9200AA), // 130
	COL(0x9200FF), // 131
	COL(0x922400), // 132
	COL(0x922455), // 133
	COL(0x9224AA), // 134
	COL(0x9224FF), // 135
	COL(0x924900), // 136
	COL(0x924955), // 137
	COL(0x9249AA), // 138
	COL(0x9249FF), // 139
	COL(0x926D00), // 140
	COL(0x926D55), // 141
	COL(0x926DAA), // 142
	COL(0x926DFF), // 143
	COL(0x929200), // 144
	COL(0x929255), // 145
	COL(0x9292AA), // 146
	COL(0x9292FF), // 147
	COL(0x92B600), // 148
	COL(0x92B655), // 149
	COL(0x92B6AA), // 150
	COL(0x92B6FF), // 151
	COL(0x92DB00), // 152
	COL(0x92DB55), // 153
	COL(0x92DBAA), // 154
	COL(0x92DBFF), // 155
	COL(0x92FF00), // 156
	COL(0x92FF55), // 157
	COL(0x92FFAA), // 158
	COL(0x92FFFF), // 159
	COL(0xB60000), // 160
	COL(0xB60055), // 161
	COL(0xB600AA), // 162
	COL(0xB600FF), // 163
	COL(0xB62400), // 164
	COL(0xB62455), // 165
	COL(0xB624AA), // 166
	COL(0xB624FF), // 167
	COL(0xB64900), // 168
	COL(0xB64955), // 169
	COL(0xB649AA), // 170
	COL(0xB649FF), // 171
	COL(0xB66D00), // 172
	COL(0xB66D55), // 173
	COL(0xB66DAA), // 174
	COL(0xB66DFF), // 175
	COL(0xB69200), // 176
	COL(0xB69255), // 177
	COL(0xB692AA), // 178
	COL(0xB692FF), // 179
	COL(0xB6B600), // 180
	COL(0xB6B655), // 181
	COL(0xB6B6AA), // 182
	COL(0xB6B6FF), // 183
	COL(0xB6DB00), // 184
	COL(0xB6DB55), // 185
	COL(0xB6DBAA), // 186
	COL(0xB6DBFF), // 187
	COL(0xB6FF00), // 188
	COL(0xB6FF55), // 189
	COL(0xB6FFAA), // 190
	COL(0xB6FFFF), // 191
	COL(0xDB0000), // 192
	COL(0xDB0055), // 193
	COL(0xDB00AA), // 194
	COL(0xDB00FF), // 195
	COL(0xDB2400), // 196
	COL(0xDB2455), // 197
	COL(0xDB24AA), // 198
	COL(0xDB24FF), // 199
	COL(0xDB4900), // 200
	COL(0xDB4955), // 201
	COL(0xDB49AA), // 202
	COL(0xDB49FF), // 203
	COL(0xDB6D00), // 204
	COL(0xDB6D55), // 205
	COL(0xDB6DAA), // 206
	COL(0xDB6DFF), // 207
	COL(0xDB9200), // 208
	COL(0xDB9255), // 209
	COL(0xDB92AA), // 210
	COL(0xDB92FF), // 211
	COL(0xDBB600), // 212
	COL(0xDBB655), // 213
	COL(0xDBB6AA), // 214
	COL(0xDBB6FF), // 215
	COL(0xDBDB00), // 216
	COL(0xDBDB55), // 217
	COL(0xDBDBAA), // 218
	COL(0xDBDBFF), // 219
	COL(0xDBFF00), // 220
	COL(0xDBFF55), // 221
	COL(0xDBFFAA), // 222
	COL(0xDBFFFF), // 223
	COL(0xFF0000), // 224
	COL(0xFF0055), // 225
	COL(0xFF00AA), // 226
	COL(0xFF00FF), // 227
	COL(0xFF2400), // 228
	COL(0xFF2455), // 229
	COL(0xFF24AA), // 230
	COL(0xFF24FF), // 231
	COL(0xFF4900), // 232
	COL(0xFF4955), // 233
	COL(0xFF49AA), // 234
	COL(0xFF49FF), // 235
	COL(0xFF6D00), // 236
	COL(0xFF6D55), // 237
	COL(0xFF6DAA), // 238
	COL(0xFF6DFF), // 239
	COL(0xFF9200), // 240
	COL(0xFF9255), // 241
	COL(0xFF92AA), // 242
	COL(0xFF92FF), // 243
	COL(0xFFB600), // 244
	COL(0xFFB655), // 245
	COL(0xFFB6AA), // 246
	COL(0xFFB6FF), // 247
	COL(0xFFDB00), // 248
	COL(0xFFDB55), // 249
	COL(0xFFDBAA), // 250
	COL(0xFFDBFF), // 251
	COL(0xFFFF00), // 252
	COL(0xFFFF55), // 253
	COL(0xFFFFAA), // 254
	COL(0xFFFFFF), // 255
};

const uint16_t * palettes16[] = {
    default_palette16,
    c64_palette16,
    apple2_palette16,
    cga_palette16,
    msx_palette16,
    spectrum_palette16,
	adaptive_gs16_palette16
};

const uint16_t * palettes256[] = {
    rgb676_palette256,
    rgb685_palette256,
    rgb884_palette256
};

// uint8_t fb[DISPLAY_WIDTH * DISPLAY_HEIGHT]; // only for palette

static uint8_t cmdBuf[20];

typedef struct _pyb_screen_obj_t {
    mp_obj_base_t base;

    // hardware control for the LCD
    const spi_t *spi;
    const pin_obj_t *pin_cs1;
    const pin_obj_t *pin_rst;
    const pin_obj_t *pin_dc;
    const pin_obj_t *pin_bl;

    // character buffer for stdout-like output
    //char char_buffer[LCD_CHAR_BUF_W * LCD_CHAR_BUF_H];
    //int line;
    //int column;
    //int next_line;

} pyb_screen_obj_t;

#define DELAY 0x80

static const uint8_t initCmds[] = {
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      120,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      120,                    //     500 ms delay
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_INVCTR , 1      ,  // inverse, riven
      0x03,
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05,                  //     16-bit color 565
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
    0x02, 0x1c, 0x07, 0x12,
    0x37, 0x32, 0x29, 0x2d,
    0x29, 0x25, 0x2B, 0x39,
    0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
    0x03, 0x1d, 0x07, 0x06,
    0x2E, 0x2C, 0x29, 0x2D,
    0x2E, 0x2E, 0x37, 0x3F,
    0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      10,
    0, 0 // END
};

STATIC void screen_delay(void) {
    __asm volatile ("nop\nnop");
}

STATIC void send_cmd(pyb_screen_obj_t *screen, uint8_t * buf, uint8_t len) {
    screen_delay();
    mp_hal_pin_low(screen->pin_cs1); // CS=0; enable
    mp_hal_pin_low(screen->pin_dc); // DC=0 for instruction
    screen_delay();
    HAL_SPI_Transmit(screen->spi->spi, buf, 1, 1000);
    //printf("cmd 0x%x\n", buf[0]);
    mp_hal_pin_high(screen->pin_dc);
    screen_delay();
    len--;
    buf++;
    if (len > 0){
        HAL_SPI_Transmit(screen->spi->spi, buf, len, 1000);
        //printf("v 0x%x len %d\n", buf[0], len);
    }
    mp_hal_pin_high(screen->pin_cs1); // CS=1; disable
}

static void sendCmdSeq(pyb_screen_obj_t *screen, const uint8_t *buf) {
    while (*buf) {
        cmdBuf[0] = *buf++;
        int v = *buf++;
        int len = v & ~DELAY;
        // note that we have to copy to RAM
        memcpy(cmdBuf + 1, buf, len);
        send_cmd(screen, cmdBuf, len + 1);
        buf += len;
        if (v & DELAY) {
            mp_hal_delay_ms(*buf++);
        }
    }
}

static void setAddrWindow(pyb_screen_obj_t *screen, int x, int y, int w, int h) {
    uint8_t cmd0[] = {ST7735_RASET, 0, (uint8_t)x, 0, (uint8_t)(x + w - 1)};
    uint8_t cmd1[] = {ST7735_CASET, 0, (uint8_t)y, 0, (uint8_t)(y + h - 1)};
    send_cmd(screen, cmd1, sizeof(cmd1));
    send_cmd(screen, cmd0, sizeof(cmd0));
}

static void configure(pyb_screen_obj_t *screen, uint8_t madctl) {
    uint8_t cmd0[] = {ST7735_MADCTL, madctl};
    // 0x00 0x06 0x03: blue tab
    uint8_t cmd1[] = {ST7735_FRMCTR1, 0x00, 0x06, 0x03};
    send_cmd(screen, cmd0, sizeof(cmd0));
    send_cmd(screen, cmd1, sizeof(cmd1));
}

/// \classmethod \constructor(skin_position)
///
/// Construct an LCD object in the given skin position.  `skin_position` can be 'X' or 'Y', and
/// should match the position where the LCD pyskin is plugged in.
STATIC mp_obj_t pyb_screen_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    int madctl = 0x60; // riven, adapt to new screen
    // inverse for framebuffer define
    int offX = 0x0;
    int offY = 0x0;
    int width = DISPLAY_HEIGHT;
    int height = DISPLAY_WIDTH;
    mp_arg_check_num(n_args, n_kw, 0, 5, false);
    if (n_args >= 1) {
        madctl = mp_obj_get_int(args[0]);
        if (n_args == 5) {
            offX = mp_obj_get_int(args[1]);
            offY = mp_obj_get_int(args[2]);
            width = mp_obj_get_int(args[3]);
            height = mp_obj_get_int(args[4]);
        }
    }

    // create screen object
    pyb_screen_obj_t *screen = m_new_obj(pyb_screen_obj_t);
    screen->base.type = &pyb_screen_type;

    // configure pins, tft bind to spi2 on f4
    screen->spi = &spi_obj[1];
    screen->pin_cs1 = pyb_pin_PB12;
    screen->pin_rst = pyb_pin_PB10;
    screen->pin_dc = pyb_pin_PA8;
    screen->pin_bl = pyb_pin_PB3;

    // init the SPI bus
    SPI_InitTypeDef *init = &screen->spi->spi->Init;
    init->Mode = SPI_MODE_MASTER;

    // compute the baudrate prescaler from the desired baudrate
    // uint spi_clock;
    // SPI2 and SPI3 are on APB1
    // spi_clock = HAL_RCC_GetPCLK1Freq();

    // data is sent bigendian, latches on rising clock
    init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    init->CLKPolarity = SPI_POLARITY_HIGH;
    init->CLKPhase = SPI_PHASE_2EDGE;
    init->Direction = SPI_DIRECTION_2LINES;
    init->DataSize = SPI_DATASIZE_8BIT;
    init->NSS = SPI_NSS_SOFT;
    init->FirstBit = SPI_FIRSTBIT_MSB;
    init->TIMode = SPI_TIMODE_DISABLED;
    init->CRCCalculation = SPI_CRCCALCULATION_DISABLED;
    init->CRCPolynomial = 0;

    // init the SPI bus
    spi_init(screen->spi, false);
    // set the pins to default values
    mp_hal_pin_high(screen->pin_cs1);
    mp_hal_pin_high(screen->pin_rst);
    mp_hal_pin_high(screen->pin_dc);
    mp_hal_pin_low(screen->pin_bl);

    // init the pins to be push/pull outputs
    mp_hal_pin_output(screen->pin_cs1);
    mp_hal_pin_output(screen->pin_rst);
    mp_hal_pin_output(screen->pin_dc);
    mp_hal_pin_output(screen->pin_bl);
    // init the LCD
    mp_hal_delay_ms(1); // wait a bit
    mp_hal_pin_low(screen->pin_rst); // RST=0; reset
    mp_hal_delay_ms(1); // wait for reset; 2us min
    mp_hal_pin_high(screen->pin_rst); // RST=1; enable
    mp_hal_delay_ms(1); // wait for reset; 2us min
    // set backlight
    mp_hal_pin_high(screen->pin_bl);

    sendCmdSeq(screen, initCmds);

    madctl = madctl & 0xff;
    configure(screen, madctl);
    setAddrWindow(screen, offX, offY, width, height);

    //memset(fb, 10, sizeof(fb));
    //draw_screen(screen);

    return MP_OBJ_FROM_PTR(screen);
}

/// \method show()
///
/// Show the hidden buffer on the screen.
/// args: fb, palette number if palette mode
STATIC mp_obj_t pyb_screen_show(size_t n_args, const mp_obj_t *args) {
    (void)n_args;

    // get framebuffer, first argument
    pyb_screen_obj_t *screen = MP_OBJ_TO_PTR(args[0]);

    mp_obj_framebuf_t *fb = MP_OBJ_TO_PTR(args[1]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);
    byte *p = bufinfo.buf;

    // get format
    uint8_t format = fb->format;   
    uint8_t palette = fb->palette; 

    uint8_t cmdBuf[] = {ST7735_RAMWR};
    send_cmd(screen, cmdBuf, 1);

    mp_hal_pin_low(screen->pin_cs1); // CS=0; enable
    mp_hal_pin_high(screen->pin_dc); // DC=1
    
    //printf("Show buffer in format %d of size %d and pal %d\n", format, bufinfo.len, palette);

    const uint16_t * pal;
    switch (format) {
        case FRAMEBUF_RGB565:
            // Will use DMA if available
            spi_transfer(screen->spi, bufinfo.len, p, NULL, 500);  
            //HAL_SPI_Transmit(screen->spi->spi, p, bufinfo.len, 1000);
            break;            
        case FRAMEBUF_PAL16:
            // after some tests, sending by packets of 8 pixels is the most efficient 
            // and not too greedy in terms of memory
            pal = palettes16[palette];
            uint8_t pixels_per_tansfer = 8;
            for (int i=0;i<bufinfo.len;i+=pixels_per_tansfer){
                uint8_t cc[pixels_per_tansfer*4];
                uint8_t counter = 0;
                for(int j=0; j<pixels_per_tansfer; j++){                
                    cc[counter++] = pal[(p[i+j] >> 4) & 0xf] >> 8;
                    cc[counter++] = pal[(p[i+j] >> 4) & 0xf] & 0xff;					
                    cc[counter++] = pal[p[i+j] & 0xf] >> 8;
                    cc[counter++] = pal[p[i+j] & 0xf] & 0xff;					
                }
                HAL_SPI_Transmit(screen->spi->spi, (uint8_t*)&cc, pixels_per_tansfer*4, 100);
                //spi_transfer(screen->spi, pixels_per_tansfer*4, (uint8_t*)&cc, NULL, 100);  
            }        
            break;
        case FRAMEBUF_PAL256:
            pal = palettes256[palette];
            uint8_t pixels8_per_tansfer = 16;
            // after some tests, sending by packets of 16 pixels is the most efficient 
            // and not too greedy in terms of memory			
            for (int i=0;i<bufinfo.len;i+=pixels8_per_tansfer){
                uint8_t cc[pixels8_per_tansfer*2];
                uint8_t counter = 0;
                for(int j=0; j<pixels8_per_tansfer; j++){                
                    cc[counter++] = pal[p[i+j]] >> 8;
                    cc[counter++] = pal[p[i+j] & 0xff];
                }
                HAL_SPI_Transmit(screen->spi->spi, (uint8_t*)&cc, pixels8_per_tansfer*2, 100);	
			}		          
            break;                
        case FRAMEBUF_MVLSB:
        case FRAMEBUF_MHLSB:
        case FRAMEBUF_MHMSB:
        case FRAMEBUF_GS2_HMSB:
        case FRAMEBUF_GS4_HMSB:
            HAL_SPI_Transmit(screen->spi->spi, p, bufinfo.len, 1000);
            break;
        default:
            mp_raise_ValueError("invalid format");        
    }

    mp_hal_pin_high(screen->pin_cs1); // CS=1; disable

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_screen_show_obj, 2, 2, pyb_screen_show);

STATIC mp_obj_t pyb_screen_set_window(size_t n_args, const mp_obj_t *args) {
    (void)n_args;

    pyb_screen_obj_t *screen = MP_OBJ_TO_PTR(args[0]);

    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t w = mp_obj_get_int(args[3]);
    mp_int_t h = mp_obj_get_int(args[4]);

    printf("x %d y %d w %d h %d", x,y,w,h);

    uint8_t cmd0[] = {ST7735_RASET, 0, (uint8_t)x, 0, (uint8_t)(x + w - 1)};
    uint8_t cmd1[] = {ST7735_CASET, 0, (uint8_t)y, 0, (uint8_t)(y + h - 1)};
    send_cmd(screen, cmd1, sizeof(cmd1));
    send_cmd(screen, cmd0, sizeof(cmd0));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_screen_set_window_obj, 5, 5, pyb_screen_set_window);

STATIC const mp_rom_map_elem_t pyb_screen_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&pyb_screen_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_window), MP_ROM_PTR(&pyb_screen_set_window_obj) }
};

STATIC MP_DEFINE_CONST_DICT(pyb_screen_locals_dict, pyb_screen_locals_dict_table);

const mp_obj_type_t pyb_screen_type = {
    { &mp_type_type },
    .name = MP_QSTR_SCREEN,
    .make_new = pyb_screen_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_screen_locals_dict,
};


#endif // MICROPY_HW_HAS_ST7735