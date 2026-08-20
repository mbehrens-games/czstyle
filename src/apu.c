/******************************************************************************/
/* apu.c (faux sound chip)                                                    */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "apu.h"

/* note: the tables are generated in gnu octave */
/*       see the octave directory for .m files  */

/**********/
/* CLOCKS */
/**********/

/* the clock rate is a multiple of 1000 so that   */
/* there are an integer number of samples per ms. */
#define APU_CLOCK_RATE        48000
#define APU_CLOCKS_PER_SAMPLE (APU_CLOCK_RATE / APU_OUT_SAMPLING_RATE)

#define APU_SEQ_DIVIDER  8  /* seq clock is 6000  */
#define APU_LFO_DIVIDER 32  /* lfo clock is 1500  */
#define APU_ENV_DIVIDER  3  /* env clock is 16000 */
#define APU_OSC_DIVIDER  1  /* osc clock is 48000 */
#define APU_PCM_DIVIDER  2  /* pcm clock is 24000 */

#define APU_TMR_DIVIDER 96  /* lcm of the other dividers */

static unsigned short S_apu_timer;

/*************/
/* SEQUENCER */
/*************/

/* phase tables */
static unsigned short S_apu_seq_phase_incs_table[224] = 
  {  5592,  5767,  5942,  6117,  6291,  6466,  6641,  6816,
     6991,  7165,  7340,  7515,  7690,  7864,  8039,  8214,
     8389,  8563,  8738,  8913,  9088,  9262,  9437,  9612,
     9787,  9961, 10136, 10311, 10486, 10661, 10835, 11010,
    11185, 11360, 11534, 11709, 11884, 12059, 12233, 12408,
    12583, 12758, 12932, 13107, 13282, 13457, 13631, 13806,
    13981, 14156, 14331, 14505, 14680, 14855, 15030, 15204,
    15379, 15554, 15729, 15903, 16078, 16253, 16428, 16602,
    16777, 16952, 17127, 17302, 17476, 17651, 17826, 18001,
    18175, 18350, 18525, 18700, 18874, 19049, 19224, 19399,
    19573, 19748, 19923, 20098, 20272, 20447, 20622, 20797,
    20972, 21146, 21321, 21496, 21671, 21845, 22020, 22195,
    22370, 22544, 22719, 22894, 23069, 23243, 23418, 23593,
    23768, 23942, 24117, 24292, 24467, 24642, 24816, 24991,
    25166, 25341, 25515, 25690, 25865, 26040, 26214, 26389,
    26564, 26739, 26913, 27088, 27263, 27438, 27613, 27787,
    27962, 28137, 28312, 28486, 28661, 28836, 29011, 29185,
    29360, 29535, 29710, 29884, 30059, 30234, 30409, 30583,
    30758, 30933, 31108, 31283, 31457, 31632, 31807, 31982,
    32156, 32331, 32506, 32681, 32855, 33030, 33205, 33380,
    33554, 33729, 33904, 34079, 34253, 34428, 34603, 34778,
    34953, 35127, 35302, 35477, 35652, 35826, 36001, 36176,
    36351, 36525, 36700, 36875, 37050, 37224, 37399, 37574,
    37749, 37923, 38098, 38273, 38448, 38623, 38797, 38972,
    39147, 39322, 39496, 39671, 39846, 40021, 40195, 40370,
    40545, 40720, 40894, 41069, 41244, 41419, 41594, 41768,
    41943, 42118, 42293, 42467, 42642, 42817, 42992, 43166,
    43341, 43516, 43691, 43865, 44040, 44215, 44390, 44564
  };

/* midi note tables */
static unsigned char S_apu_seq_midi_note_number_table[128] = 
  {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
    72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
    84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    96,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0
  };

static unsigned short S_apu_seq_midi_note_velocity_table[128] = 
  { 4095, 1008, 1000,  992,  984,  976,  968,  960,
     952,  944,  936,  928,  920,  912,  904,  896,
     888,  880,  872,  864,  856,  848,  840,  832,
     824,  816,  808,  800,  792,  784,  776,  768,
     760,  752,  744,  736,  728,  720,  712,  704,
     696,  688,  680,  672,  664,  656,  648,  640,
     632,  624,  616,  608,  600,  592,  584,  576,
     568,  560,  552,  544,  536,  528,  520,  512,
     504,  496,  488,  480,  472,  464,  456,  448,
     440,  432,  424,  416,  408,  400,  392,  384,
     376,  368,  360,  352,  344,  336,  328,  320,
     312,  304,  296,  288,  280,  272,  264,  256,
     248,  240,  232,  224,  216,  208,  200,  192,
     184,  176,  168,  160,  152,  144,  136,  128,
     120,  112,  104,   96,   88,   80,   72,   64,
      56,   48,   40,   32,   24,   16,    8,    0
  };

/* volume and panning */
static unsigned short S_apu_inst_vol_table[128] = 
  { 4095, 1512, 1500, 1488, 1476, 1464, 1452, 1440,
    1428, 1416, 1404, 1392, 1380, 1368, 1356, 1344,
    1332, 1320, 1308, 1296, 1284, 1272, 1260, 1248,
    1236, 1224, 1212, 1200, 1188, 1176, 1164, 1152,
    1140, 1128, 1116, 1104, 1092, 1080, 1068, 1056,
    1044, 1032, 1020, 1008,  996,  984,  972,  960,
     948,  936,  924,  912,  900,  888,  876,  864,
     852,  840,  828,  816,  804,  792,  780,  768,
     756,  744,  732,  720,  708,  696,  684,  672,
     660,  648,  636,  624,  612,  600,  588,  576,
     564,  552,  540,  528,  516,  504,  492,  480,
     468,  456,  444,  432,  420,  408,  396,  384,
     372,  360,  348,  336,  324,  312,  300,  288,
     276,  264,  252,  240,  228,  216,  204,  192,
     180,  168,  156,  144,  132,  120,  108,   96,
      84,   72,   60,   48,   36,   24,   12,    0
  };

static unsigned short S_apu_inst_pan_table[128] = 
  { 4095, 1625, 1369, 1220, 1113, 1031,  964,  907,
     858,  814,  776,  741,  709,  679,  652,  627,
     604,  582,  561,  541,  523,  505,  488,  472,
     457,  442,  428,  415,  402,  389,  377,  366,
     355,  344,  334,  324,  314,  304,  295,  286,
     278,  269,  261,  253,  246,  238,  231,  224,
     217,  210,  204,  198,  191,  185,  179,  174,
     168,  163,  157,  152,  147,  142,  137,  133,
     128,  124,  119,  115,  111,  107,  103,   99,
      95,   91,   88,   84,   81,   78,   74,   71,
      68,   65,   62,   59,   57,   54,   51,   49,
      46,   44,   42,   39,   37,   35,   33,   31,
      29,   27,   26,   24,   22,   21,   19,   18,
      16,   15,   14,   12,   11,   10,    9,    8,
       7,    6,    5,    5,    4,    3,    3,    2,
       2,    1,    1,    1,    0,    0,    0,    0
  };

/*******/
/* LFO */
/*******/

/* phase tables */
static unsigned short S_apu_lfo_phase_incs_table[32] = 
  {  1398,  2097,  2796,  3495,  4194,  4893,  5592,  6291,
     6991,  7690,  8389,  9088,  9787, 10486, 11185, 11884,
    12583, 13282, 13981, 14680, 15379, 16078, 16777, 17476,
    18175, 18874, 19573, 20272, 20972, 21671, 22370, 23069
  };

/* sensitivities */
static unsigned char S_apu_lfo_vib_shifts_table[4] = 
  { 5, 4, 2, 1 };

static unsigned char S_apu_lfo_trem_shifts_table[2] = 
  { 2, 0 };

/* step sizes */
static unsigned short S_apu_lfo_step_sizes_table[64] = 
  {  0,  2,  5,  7, 10, 12, 15, 17,
     2,  4,  6,  8, 11, 13, 15, 17,
     5,  7,  8, 10, 12, 14, 15, 17,
     7,  8, 10, 11, 13, 14, 16, 17,
    10, 11, 12, 13, 14, 15, 16, 17,
    12, 13, 13, 14, 15, 16, 16, 17,
    15, 15, 16, 16, 16, 16, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17
  };

/*******/
/* ENV */
/*******/

/* stages */
enum
{
  APU_ENV_STAGE_A = 0,
  APU_ENV_STAGE_D, 
  APU_ENV_STAGE_S, 
  APU_ENV_STAGE_R 
};

/* rate and level constants */
#define APU_ENV_RATE_NUM_BLOCKS         16
#define APU_ENV_RATE_PATTERNS_PER_BLOCK 8

#define APU_ENV_MAX_RATE  ((16 * 8) - 1)  /* 127  */
#define APU_ENV_MAX_LEVEL ((16 * 64) - 1) /* 1023 */

#define APU_ENV_RATE_BASE_BLOCK 11

/* step patterns */
static unsigned short S_apu_env_step_patterns[16] = 
  { 0x0000, 0x0080, 0x0808, 0x0888, 0x2222, 0x22A2, 0x2A2A, 0x2AAA,
    0x5555, 0x55D5, 0x5D5D, 0x5DDD, 0x7777, 0x77F7, 0x7F7F, 0x7FFF
  };

/* parameter mapping */
static unsigned short S_apu_env_rise_time_map[100] = 
  { 127, 126, 125, 124, 122, 121, 120, 119, 118, 117,
    116, 115, 113, 112, 111, 110, 109, 108, 107, 106,
    104, 103, 102, 101, 100,  99,  98,  97,  96,  94,
     93,  92,  91,  90,  89,  88,  87,  85,  84,  83,
     82,  81,  80,  79,  78,  76,  75,  74,  73,  72,
     71,  70,  69,  68,  66,  65,  64,  63,  62,  61,
     60,  59,  57,  56,  55,  54,  53,  52,  51,  50,
     48,  47,  46,  45,  44,  43,  42,  41,  40,  38,
     37,  36,  35,  34,  33,  32,  31,  29,  28,  27,
     26,  25,  24,  23,  22,  20,  19,  18,  17,  16
  };

static unsigned short S_apu_env_fall_time_map[100] = 
  { 111, 110, 109, 108, 106, 105, 104, 103, 102, 101,
    100,  99,  97,  96,  95,  94,  93,  92,  91,  90,
     88,  87,  86,  85,  84,  83,  82,  81,  80,  78,
     77,  76,  75,  74,  73,  72,  71,  69,  68,  67,
     66,  65,  64,  63,  62,  60,  59,  58,  57,  56,
     55,  54,  53,  52,  50,  49,  48,  47,  46,  45,
     44,  43,  41,  40,  39,  38,  37,  36,  35,  34,
     32,  31,  30,  29,  28,  27,  26,  25,  24,  22,
     21,  20,  19,  18,  17,  16,  15,  13,  12,  11,
     10,   9,   8,   7,   6,   4,   3,   2,   1,   0
  };

static unsigned short S_apu_env_sustain_level_map[100] = 
  { 1023,  512,  507,  502,  496,  491,  486,  481,  476,  471,
     465,  460,  455,  450,  445,  440,  434,  429,  424,  419,
     414,  409,  403,  398,  393,  388,  383,  378,  372,  367,
     362,  357,  352,  347,  341,  336,  331,  326,  321,  315,
     310,  305,  300,  295,  290,  284,  279,  274,  269,  264,
     259,  253,  248,  243,  238,  233,  228,  222,  217,  212,
     207,  202,  197,  191,  186,  181,  176,  171,  165,  160,
     155,  150,  145,  140,  134,  129,  124,  119,  114,  109,
     103,   98,   93,   88,   83,   78,   72,   67,   62,   57,
      52,   47,   41,   36,   31,   26,   21,   16,   10,    5
  };

/*******/
/* OSC */
/*******/

/* pitch table */
#define APU_OSC_PITCH_NUM_BLOCKS      9
#define APU_OSC_PITCH_STEPS_PER_BLOCK (12 * 64)

#define APU_OSC_MAX_PITCH ((9 * 12 * 64) - 1) /* 6911 */

#define APU_OSC_PITCH_BASE_BLOCK 2

static unsigned short S_apu_osc_pitch_table[48] = 
  { 1429, 1450, 1471, 1492,
    1514, 1536, 1558, 1581,
    1604, 1627, 1651, 1675,
    1699, 1724, 1749, 1774,
    1800, 1826, 1853, 1880,
    1907, 1935, 1963, 1992,
    2021, 2050, 2080, 2110,
    2141, 2172, 2204, 2236,
    2268, 2301, 2335, 2369,
    2403, 2438, 2473, 2509,
    2546, 2583, 2620, 2659,
    2697, 2736, 2776, 2817
  };

static unsigned short S_apu_osc_pitch_deltas[48] = 
  { 21, 21, 21, 22,
    22, 22, 23, 23,
    23, 24, 24, 24,
    25, 25, 25, 26,
    26, 27, 27, 27,
    28, 28, 29, 29,
    29, 30, 30, 31,
    31, 32, 32, 32,
    33, 34, 34, 34,
    35, 35, 36, 37,
    37, 37, 39, 38,
    39, 40, 41, 41
  };

/* sine wavetable (10 bit index, 1st quarter cycle stored) */
static unsigned short S_apu_osc_sine_table[256] = 
  {  2137,  1731,  1543,  1419,  1326,  1252,  1190,  1137,
     1091,  1050,  1013,   979,   949,   920,   894,   869,
      846,   825,   804,   785,   767,   749,   732,   717,
      701,   687,   672,   659,   646,   633,   621,   609,
      598,   587,   576,   566,   556,   546,   536,   527,
      518,   509,   501,   492,   484,   476,   468,   461,
      453,   446,   439,   432,   425,   418,   411,   405,
      399,   392,   386,   380,   375,   369,   363,   358,
      352,   347,   341,   336,   331,   326,   321,   316,
      311,   307,   302,   297,   293,   289,   284,   280,
      276,   271,   267,   263,   259,   255,   251,   248,
      244,   240,   236,   233,   229,   226,   222,   219,
      215,   212,   209,   205,   202,   199,   196,   193,
      190,   187,   184,   181,   178,   175,   172,   169,
      167,   164,   161,   159,   156,   153,   151,   148,
      146,   143,   141,   138,   136,   134,   131,   129,
      127,   125,   122,   120,   118,   116,   114,   112,
      110,   108,   106,   104,   102,   100,    98,    96,
       94,    92,    91,    89,    87,    85,    83,    82,
       80,    78,    77,    75,    74,    72,    70,    69,
       67,    66,    64,    63,    62,    60,    59,    57,
       56,    55,    53,    52,    51,    49,    48,    47,
       46,    45,    43,    42,    41,    40,    39,    38,
       37,    36,    35,    34,    33,    32,    31,    30,
       29,    28,    27,    26,    25,    24,    23,    23,
       22,    21,    20,    20,    19,    18,    17,    17,
       16,    15,    15,    14,    13,    13,    12,    12,
       11,    10,    10,     9,     9,     8,     8,     7,
        7,     7,     6,     6,     5,     5,     5,     4,
        4,     4,     3,     3,     3,     2,     2,     2,
        2,     1,     1,     1,     1,     1,     1,     1,
        0,     0,     0,     0,     0,     0,     0,     0
  };

/* level table */
#define APU_OSC_LEVEL_NUM_BLOCKS 16   /* blocks 0 to 15 */
#define APU_OSC_LEVEL_TABLE_SIZE 256

#define APU_OSC_MAX_LEVEL ((16 * 256) - 1) /* 4095 */

#define APU_OSC_LEVEL_ZERO_BLOCK 13   /* output is zeroed from here out */

/* converting from 12 bit db value to 13 bit linear value */
static unsigned short S_apu_osc_level_table[APU_OSC_LEVEL_TABLE_SIZE] = 
  { 8168, 8148, 8124, 8104, 8080, 8060, 8036, 8016,
    7992, 7972, 7952, 7928, 7908, 7884, 7864, 7844,
    7820, 7800, 7780, 7760, 7736, 7716, 7696, 7676,
    7656, 7632, 7612, 7592, 7572, 7552, 7532, 7512,
    7492, 7472, 7448, 7428, 7408, 7388, 7368, 7348,
    7328, 7308, 7292, 7272, 7252, 7232, 7212, 7192,
    7172, 7152, 7132, 7116, 7096, 7076, 7056, 7036,
    7020, 7000, 6980, 6964, 6944, 6924, 6904, 6888,
    6868, 6848, 6832, 6812, 6796, 6776, 6756, 6740,
    6720, 6704, 6684, 6668, 6648, 6632, 6612, 6596,
    6576, 6560, 6540, 6524, 6508, 6488, 6472, 6452,
    6436, 6420, 6400, 6384, 6368, 6348, 6332, 6316,
    6300, 6280, 6264, 6248, 6232, 6212, 6196, 6180,
    6164, 6148, 6132, 6112, 6096, 6080, 6064, 6048,
    6032, 6016, 6000, 5984, 5968, 5952, 5936, 5916,
    5900, 5884, 5872, 5856, 5840, 5824, 5808, 5792,
    5776, 5760, 5744, 5728, 5712, 5696, 5684, 5668,
    5652, 5636, 5620, 5604, 5592, 5576, 5560, 5544,
    5532, 5516, 5500, 5484, 5472, 5456, 5440, 5428,
    5412, 5396, 5384, 5368, 5352, 5340, 5324, 5312,
    5296, 5280, 5268, 5252, 5240, 5224, 5212, 5196,
    5184, 5168, 5156, 5140, 5128, 5112, 5100, 5084,
    5072, 5056, 5044, 5032, 5016, 5004, 4988, 4976,
    4964, 4948, 4936, 4924, 4908, 4896, 4884, 4868,
    4856, 4844, 4832, 4816, 4804, 4792, 4780, 4764,
    4752, 4740, 4728, 4712, 4700, 4688, 4676, 4664,
    4652, 4636, 4624, 4612, 4600, 4588, 4576, 4564,
    4552, 4540, 4528, 4512, 4500, 4488, 4476, 4464,
    4452, 4440, 4428, 4416, 4404, 4392, 4380, 4368,
    4356, 4344, 4336, 4324, 4312, 4300, 4288, 4276,
    4264, 4252, 4240, 4228, 4220, 4208, 4196, 4184,
    4172, 4160, 4152, 4140, 4128, 4116, 4104, 4096
  };

/*******/
/* PCM */
/*******/

/* phase tables */
static unsigned short S_apu_pcm_phase_incs[4] = 
  { 22629, 22837, 30106, 60211 };

/* value to db table (8 bits) */
static unsigned short S_apu_pcm_val_to_db_table[128] = 
  { 2047, 1641, 1452, 1328, 1235, 1161, 1099, 1046,
    1000,  959,  922,  889,  858,  829,  803,  778,
     755,  733,  713,  693,  675,  657,  641,  625,
     609,  594,  580,  567,  553,  541,  528,  516,
     505,  494,  483,  472,  462,  452,  442,  433,
     424,  415,  406,  397,  389,  381,  373,  365,
     357,  349,  342,  335,  328,  321,  314,  307,
     301,  294,  288,  281,  275,  269,  263,  257,
     252,  246,  240,  235,  229,  224,  219,  214,
     208,  203,  198,  194,  189,  184,  179,  174,
     170,  165,  161,  156,  152,  148,  143,  139,
     135,  131,  127,  123,  119,  115,  111,  107,
     103,   99,   95,   92,   88,   84,   81,   77,
      73,   70,   66,   63,   60,   56,   53,   50,
      46,   43,   40,   37,   33,   30,   27,   24,
      21,   18,   15,   12,    9,    6,    3,    0
  };

/*******/
/* OUT */
/*******/

/* dac */
#define APU_DAC_POS_MULT 8224
#define APU_DAC_NEG_MULT 8160

/* highpass filters */
#define APU_HP_MULT_A0  32768
#define APU_HP_MULT_A1 -32631
#define APU_HP_MULT_B0  32700
#define APU_HP_MULT_B1 -32700

static short S_apu_hp_in[4];  /* 2 channels, 2 inputs each  */
static short S_apu_hp_out[4]; /* 2 channels, 2 outputs each */

/* lowpass filters */
#define APU_LP_MULT_A0  32768
#define APU_LP_MULT_A1 -22395
#define APU_LP_MULT_B0   5187
#define APU_LP_MULT_B1   5187

static short S_apu_lp_in[4];
static short S_apu_lp_out[4];

/* downsampler filters */
#define APU_DS_M 64

#define APU_DS_KERNEL_SIZE ((APU_DS_M / 2) + 1)
#define APU_DS_BUFFER_SIZE (APU_DS_M + 1)

static short S_apu_ds_kernel[APU_DS_KERNEL_SIZE] = 
  {    -3,   -28,    -9,    32,    28,   -32,   -56,    21,
       93,    14,  -128,   -81,   142,   178,  -113,  -295,
       17,   403,   161,  -462,  -424,   419,   757,  -214,
    -1129,  -232,  1494,  1072, -1803, -2819,  2009, 10211,
    14318
  };

static short S_apu_ds_L_in[APU_DS_BUFFER_SIZE];
static short S_apu_ds_R_in[APU_DS_BUFFER_SIZE];

static short S_apu_ds_buf_pos;

/* stereo output */
short G_apu_out_L;
short G_apu_out_R;

/*************/
/* REGISTERS */
/*************/

/* wave voices */
enum
{
  APU_WAVE_REG_PATCH_NO = 0, 
  APU_WAVE_REG_VOLUME, 
  APU_WAVE_REG_PANNING, 
  APU_WAVE_REG_NOTE, 
  APU_WAVE_REG_VELOCITY, 
  APU_WAVE_REG_LFO_MANTISSA, 
  APU_WAVE_REG_LFO_INDEX, 
  APU_WAVE_REG_VIB_LEVEL, 
  APU_WAVE_REG_TREM_LEVEL, 
  APU_WAVE_REG_ENV_STAGE, 
  APU_WAVE_REG_ENV_PERIOD, 
  APU_WAVE_REG_ENV_BLOCK, 
  APU_WAVE_REG_ENV_PATTERN, 
  APU_WAVE_REG_ENV_STEP, 
  APU_WAVE_REG_ENV_LEVEL, 
  APU_WAVE_REG_ENV_MANTISSA, 
  APU_WAVE_REG_OSC_INDEX, 
  APU_WAVE_REG_OSC_MANTISSA, 
  APU_WAVE_REG_OSC_LEVEL, 
  APU_WAVE_REG_WHEEL_PITCH, 
  APU_WAVE_REG_WHEEL_VIB, 
  APU_WAVE_REG_WHEEL_TREM, 
  APU_WAVE_REG_SW_PORTA, 
  APU_WAVE_REG_SW_SUSTAIN, 
  APU_NUM_WAVE_REGS 
};

#define APU_NUM_WAVE_VOICES (9 + 1)

#define APU_WAVE_REGS_BANK_SIZE (APU_NUM_WAVE_VOICES * APU_NUM_WAVE_REGS)

static unsigned short S_apu_wave_regs_bank[APU_WAVE_REGS_BANK_SIZE];

#define APU_WAVE_REG(voice_num, reg)                                           \
  S_apu_wave_regs_bank[(voice_num) * APU_NUM_WAVE_REGS + APU_WAVE_REG_##reg]

/* pcm voices */
enum
{
  APU_PCM_REG_SAMPLE_NO = 0, 
  APU_PCM_REG_VOLUME, 
  APU_PCM_REG_PANNING, 
  APU_PCM_REG_VELOCITY, 
  APU_PCM_REG_PHASE, 
  APU_PCM_REG_INDEX, 
  APU_PCM_REG_LEVEL, 
  APU_NUM_PCM_REGS 
};

#define APU_NUM_PCM_VOICES (5 + 1)

#define APU_PCM_REGS_BANK_SIZE (APU_NUM_PCM_VOICES * APU_NUM_PCM_REGS)

static unsigned short S_apu_pcm_regs_bank[APU_PCM_REGS_BANK_SIZE];

#define APU_PCM_REG(voice_num, reg)                                            \
  S_apu_pcm_regs_bank[(voice_num) * APU_NUM_PCM_REGS + APU_PCM_REG_##reg]

/* sequencer tracks */
enum
{
  APU_SEQ_REG_SONG_NO = 0, 
  APU_SEQ_REG_TEMPO, 
  APU_SEQ_REG_DELAY, 
  APU_SEQ_REG_PHASE, 
  APU_SEQ_REG_INDEX, 
  APU_NUM_SEQ_REGS 
};

enum
{
  APU_SEQ_TRACK_MUSIC = 0, 
  APU_SEQ_TRACK_SFX, 
  APU_NUM_SEQ_TRACKS 
};

#define APU_SEQ_REGS_BANK_SIZE (APU_NUM_SEQ_TRACKS * APU_NUM_SEQ_REGS)

static unsigned short S_apu_seq_regs_bank[APU_SEQ_REGS_BANK_SIZE];

#define APU_SEQ_REG(track_num, reg)                                            \
  S_apu_seq_regs_bank[(track_num) * APU_NUM_SEQ_REGS + APU_SEQ_REG_##reg]

/***********/
/* PATCHES */
/***********/

/* wave patches */
enum
{
  APU_PATCH_PARAM_ENV_AR = 0, 
  APU_PATCH_PARAM_ENV_DR, 
  APU_PATCH_PARAM_ENV_SR, 
  APU_PATCH_PARAM_ENV_RR, 
  APU_PATCH_PARAM_ENV_SL, 
  APU_PATCH_PARAM_LFO_SPEED, 
  APU_PATCH_PARAM_VIB_SENS_DEPTH, 
  APU_PATCH_PARAM_TREM_SENS_DEPTH, 
  APU_NUM_PATCH_PARAMS 
};

#define APU_MAX_PATCHES 32

#define APU_PATCH_BANK_SIZE (APU_MAX_PATCHES * APU_NUM_PATCH_PARAMS)

static unsigned char S_apu_patches[APU_PATCH_BANK_SIZE];

#define APU_PATCH_PARAM(patch_num, param)                                      \
  S_apu_patches[(patch_num) * APU_NUM_PATCH_PARAMS + APU_PATCH_PARAM_##param]

/* drum kits */
enum
{
  APU_KIT_PARAM_SAMPLE_NO_BD = 0, 
  APU_KIT_PARAM_SAMPLE_NO_SD, 
  APU_KIT_PARAM_SAMPLE_NO_OH, 
  APU_KIT_PARAM_SAMPLE_NO_CH, 
  APU_KIT_PARAM_SAMPLE_NO_CY, 
  APU_KIT_PARAM_SAMPLE_NO_RD, 
  APU_KIT_PARAM_SAMPLE_NO_LT, 
  APU_KIT_PARAM_SAMPLE_NO_HT, 
  APU_NUM_KIT_PARAMS 
};

#define APU_MAX_KITS 8

#define APU_KIT_BANK_SIZE (APU_MAX_KITS * APU_NUM_KIT_PARAMS)

static unsigned char S_apu_kits[APU_KIT_BANK_SIZE];

#define APU_KIT_PARAM(kit_num, param)                                          \
  S_apu_kits[(kit_num) * APU_NUM_KIT_PARAMS + APU_KIT_PARAM_##param]

/**************/
/* NAMETABLES */
/**************/

/* sample nametable entries */
enum
{
  APU_SAMPLE_PARAM_ADDR_1 = 0, 
  APU_SAMPLE_PARAM_ADDR_2, 
  APU_SAMPLE_PARAM_ADDR_3, 
  APU_SAMPLE_PARAM_SIZE_1, 
  APU_SAMPLE_PARAM_SIZE_2, 
  APU_SAMPLE_PARAM_RATE, 
  APU_NUM_SAMPLE_PARAMS 
};

#define APU_MAX_SAMPLES 64

#define APU_SAMPLE_NAMETABLE_SIZE (APU_MAX_SAMPLES * APU_NUM_SAMPLE_PARAMS)

static unsigned char S_apu_samples[APU_SAMPLE_NAMETABLE_SIZE];

#define APU_SAMPLE_PARAM(samp_num, param)                                      \
  S_apu_samples[(samp_num) * APU_NUM_SAMPLE_PARAMS + APU_SAMPLE_PARAM_##param]

/* song nametable entries */
enum
{
  APU_SONG_PARAM_ADDR_1 = 0, 
  APU_SONG_PARAM_ADDR_2, 
  APU_SONG_PARAM_ADDR_3, 
  APU_SONG_PARAM_SIZE_1, 
  APU_SONG_PARAM_SIZE_2, 
  APU_NUM_SONG_PARAMS 
};

#define APU_MAX_SONGS 32

#define APU_SONG_NAMETABLE_SIZE (APU_MAX_SONGS * APU_NUM_SONG_PARAMS)

static unsigned char S_apu_songs[APU_SONG_NAMETABLE_SIZE];

#define APU_SONG_PARAM(song_num, param)                                        \
  S_apu_songs[(song_num) * APU_NUM_SONG_PARAMS + APU_SONG_PARAM_##param]

/********/
/* ROMS */
/********/

#define APU_MIDI_DATA_SIZE (1 << 19)
#define APU_PCM_DATA_SIZE  (1 << 19)

static unsigned char S_apu_midi_data[APU_MIDI_DATA_SIZE];
static unsigned char S_apu_pcm_data[APU_PCM_DATA_SIZE];

/******************************************************************************/
/* apu_reset()                                                                */
/******************************************************************************/
int apu_reset()
{
  int m;

  S_apu_timer = 0;

  /* reset registers */
  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    APU_WAVE_REG(m, PATCH_NO) = 0;
    APU_WAVE_REG(m, VOLUME)   = 0;
    APU_WAVE_REG(m, PANNING)  = 0;
    APU_WAVE_REG(m, NOTE)     = 0;
    APU_WAVE_REG(m, VELOCITY) = 0;

    APU_WAVE_REG(m, LFO_INDEX)    = 0;
    APU_WAVE_REG(m, LFO_MANTISSA) = 0;
    APU_WAVE_REG(m, VIB_LEVEL)    = 0;
    APU_WAVE_REG(m, TREM_LEVEL)   = 0;

    APU_WAVE_REG(m, ENV_STAGE)    = APU_ENV_STAGE_R;
    APU_WAVE_REG(m, ENV_PERIOD)   = 0;
    APU_WAVE_REG(m, ENV_BLOCK)    = 0;
    APU_WAVE_REG(m, ENV_PATTERN)  = 0;
    APU_WAVE_REG(m, ENV_STEP)     = 0;
    APU_WAVE_REG(m, ENV_LEVEL)    = APU_ENV_MAX_LEVEL;
    APU_WAVE_REG(m, ENV_MANTISSA) = 0;

    APU_WAVE_REG(m, OSC_INDEX)    = 0;
    APU_WAVE_REG(m, OSC_MANTISSA) = 0;
    APU_WAVE_REG(m, OSC_LEVEL)    = APU_OSC_MAX_LEVEL;

    APU_WAVE_REG(m, WHEEL_PITCH) = 0;
    APU_WAVE_REG(m, WHEEL_VIB)   = 0;
    APU_WAVE_REG(m, WHEEL_TREM)  = 0;
    APU_WAVE_REG(m, SW_PORTA)    = 0;
    APU_WAVE_REG(m, SW_SUSTAIN)  = 0;
  }

  for (m = 0; m < APU_NUM_PCM_VOICES; m++)
  {
    APU_PCM_REG(m, SAMPLE_NO) = 0;
    APU_PCM_REG(m, VOLUME)    = 0;
    APU_PCM_REG(m, PANNING)   = 0;
    APU_PCM_REG(m, VELOCITY)  = 0;

    APU_PCM_REG(m, PHASE) = 0;
    APU_PCM_REG(m, INDEX) = 0;
    APU_PCM_REG(m, LEVEL) = APU_OSC_MAX_LEVEL;
  }

  for (m = 0; m < APU_NUM_SEQ_TRACKS; m++)
  {
    APU_SEQ_REG(m, SONG_NO) = 0;
    APU_SEQ_REG(m, TEMPO)   = 0;
    APU_SEQ_REG(m, PHASE)   = 0;
    APU_SEQ_REG(m, INDEX)   = 0;
    APU_SEQ_REG(m, DELAY)   = 0;
  }

  /* reset params */
  for (m = 0; m < APU_MAX_PATCHES; m++)
  {
    APU_PATCH_PARAM(m, ENV_AR) = 0;
    APU_PATCH_PARAM(m, ENV_DR) = 0;
    APU_PATCH_PARAM(m, ENV_SR) = 0;
    APU_PATCH_PARAM(m, ENV_RR) = 0;
    APU_PATCH_PARAM(m, ENV_SL) = 0;
    APU_PATCH_PARAM(m, LFO_SPEED) = 0;
    APU_PATCH_PARAM(m, VIB_SENS_DEPTH) = 0;
    APU_PATCH_PARAM(m, TREM_SENS_DEPTH) = 0;
  }

  for (m = 0; m < APU_MAX_KITS; m++)
  {
    APU_KIT_PARAM(m, SAMPLE_NO_BD) = 0;
    APU_KIT_PARAM(m, SAMPLE_NO_SD) = 0;
    APU_KIT_PARAM(m, SAMPLE_NO_OH) = 0;
    APU_KIT_PARAM(m, SAMPLE_NO_CH) = 0;
    APU_KIT_PARAM(m, SAMPLE_NO_CY) = 0;
    APU_KIT_PARAM(m, SAMPLE_NO_RD) = 0;
    APU_KIT_PARAM(m, SAMPLE_NO_LT) = 0;
    APU_KIT_PARAM(m, SAMPLE_NO_HT) = 0;
  }

  /* reset nametables */
  for (m = 0; m < APU_MAX_SAMPLES; m++)
  {
    APU_SAMPLE_PARAM(m, ADDR_1) = 0x00;
    APU_SAMPLE_PARAM(m, ADDR_2) = 0x00;
    APU_SAMPLE_PARAM(m, ADDR_3) = 0x00;
    APU_SAMPLE_PARAM(m, SIZE_1) = 0x00;
    APU_SAMPLE_PARAM(m, SIZE_2) = 0x00;
    APU_SAMPLE_PARAM(m, RATE)   = 0;
  }

  for (m = 0; m < APU_MAX_SONGS; m++)
  {
    APU_SONG_PARAM(m, ADDR_1) = 0x00;
    APU_SONG_PARAM(m, ADDR_2) = 0x00;
    APU_SONG_PARAM(m, ADDR_3) = 0x00;
    APU_SONG_PARAM(m, SIZE_1) = 0x00;
    APU_SONG_PARAM(m, SIZE_2) = 0x00;
  }

  /* reset roms */
  for (m = 0; m < APU_MIDI_DATA_SIZE; m++)
    S_apu_midi_data[m] = 0;

  for (m = 0; m < APU_PCM_DATA_SIZE; m++)
    S_apu_pcm_data[m] = 0;

  /* reset filters */
  for (m = 0; m < 4; m++)
  {
    S_apu_hp_in[m] = 0;
    S_apu_hp_out[m] = 0;

    S_apu_lp_in[m] = 0;
    S_apu_lp_out[m] = 0;
  }

  for (m = 0; m < APU_DS_BUFFER_SIZE; m++)
  {
    S_apu_ds_L_in[m] = 0;
    S_apu_ds_R_in[m] = 0;
  }

  S_apu_ds_buf_pos = 0;

  /* reset output */
  G_apu_out_L = 0;
  G_apu_out_R = 0;

  /* testing: setup the 1st patch */
  APU_PATCH_PARAM(0, ENV_AR) = 20;
  APU_PATCH_PARAM(0, ENV_DR) = 25;
  APU_PATCH_PARAM(0, ENV_SR) = 50;
  APU_PATCH_PARAM(0, ENV_RR) = 40;
  APU_PATCH_PARAM(0, ENV_SL) = 60;
  APU_PATCH_PARAM(0, LFO_SPEED) = 24;
  APU_PATCH_PARAM(0, VIB_SENS_DEPTH) =  (1 << 3) | 7;
  APU_PATCH_PARAM(0, TREM_SENS_DEPTH) = (0 << 3) | 0;

  return 0;
}

/******************************************************************************/
/* apu_play_note()                                                            */
/******************************************************************************/
int apu_play_note(unsigned short inst_num, unsigned short note)
{
  unsigned short patch_num;
  unsigned short speed;

  if (inst_num >= APU_NUM_WAVE_VOICES)
    return 0;

  if (note >= 128)
    return 0;

  if (S_apu_seq_midi_note_number_table[note] == 0)
    return 0;

  APU_WAVE_REG(inst_num, NOTE) = S_apu_seq_midi_note_number_table[note];

  APU_WAVE_REG(inst_num, LFO_INDEX)    = 0;
  APU_WAVE_REG(inst_num, LFO_MANTISSA) = 0;

  APU_WAVE_REG(inst_num, ENV_STAGE)    = APU_ENV_STAGE_A;
  APU_WAVE_REG(inst_num, ENV_STEP)     = 0;
  APU_WAVE_REG(inst_num, ENV_MANTISSA) = 0;

  APU_WAVE_REG(inst_num, OSC_INDEX)    = 0;
  APU_WAVE_REG(inst_num, OSC_MANTISSA) = 0;

  /* initialize envelope block & pattern */
  patch_num = APU_WAVE_REG(inst_num, PATCH_NO);

  speed = S_apu_env_rise_time_map[APU_PATCH_PARAM(patch_num, ENV_AR)];

  APU_WAVE_REG(inst_num, ENV_BLOCK)   = speed / APU_ENV_RATE_PATTERNS_PER_BLOCK;
  APU_WAVE_REG(inst_num, ENV_PATTERN) = speed % APU_ENV_RATE_PATTERNS_PER_BLOCK;

  APU_WAVE_REG(inst_num, ENV_PERIOD) = 1;

  return 0;
}

/******************************************************************************/
/* apu_advance_sequencer()                                                    */
/******************************************************************************/
int apu_advance_sequencer()
{
  return 0;
}

/******************************************************************************/
/* apu_advance_lfo()                                                          */
/******************************************************************************/
int apu_advance_lfo()
{
  int m;

  unsigned short patch_num;

  unsigned short increment;
  unsigned short vib_level;
  unsigned short trem_level;

  unsigned char wave_step;
  unsigned char vib_depth;
  unsigned char vib_sens;
  unsigned char trem_depth;
  unsigned char trem_sens;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* obtain patch number */
    patch_num = APU_WAVE_REG(m, PATCH_NO);

    /* lookup phase increment */
    increment = S_apu_lfo_phase_incs_table[APU_PATCH_PARAM(patch_num, LFO_SPEED)];

    /* update phase (5.15 fixed point) */
    APU_WAVE_REG(m, LFO_MANTISSA) += increment;
    APU_WAVE_REG(m, LFO_INDEX) += (APU_WAVE_REG(m, LFO_MANTISSA) >> 15);

    APU_WAVE_REG(m, LFO_MANTISSA) &= 0x7FFF;
    APU_WAVE_REG(m, LFO_INDEX) &= 0x001F;

    /* wavetable lookup */
    /* testing: just a triangle for now! */
    if (APU_WAVE_REG(m, LFO_INDEX) < 16)
      wave_step = APU_WAVE_REG(m, LFO_INDEX);
    else
      wave_step = 31 - APU_WAVE_REG(m, LFO_INDEX);

    /* obtain depth and sensitivity params */
    vib_depth = APU_PATCH_PARAM(patch_num, VIB_SENS_DEPTH) & 0x07;
    vib_sens = (APU_PATCH_PARAM(patch_num, VIB_SENS_DEPTH) >> 3) & 0x03;

    trem_depth = APU_PATCH_PARAM(patch_num, TREM_SENS_DEPTH) & 0x07;
    trem_sens = (APU_PATCH_PARAM(patch_num, TREM_SENS_DEPTH) >> 3) & 0x01;

    /* determine initial levels */
    vib_level = wave_step * S_apu_lfo_step_sizes_table[8 * vib_depth + 0];
    trem_level = wave_step * S_apu_lfo_step_sizes_table[8 * trem_depth + 0];

    /* apply sensitivity */
    if (S_apu_lfo_vib_shifts_table[vib_sens] > 0)
      vib_level = vib_level >> S_apu_lfo_vib_shifts_table[vib_sens];

    if (S_apu_lfo_trem_shifts_table[trem_sens] > 0)
      trem_level = trem_level >> S_apu_lfo_trem_shifts_table[trem_sens];

    /* set levels */
    APU_WAVE_REG(m, VIB_LEVEL) = vib_level;
    APU_WAVE_REG(m, TREM_LEVEL) = trem_level;
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_env()                                                          */
/******************************************************************************/
int apu_advance_env()
{
  int m;

  /* local patch param variables, for clarity */
  unsigned char ar;
  unsigned char dr;
  unsigned char sr;
  unsigned char rr;
  unsigned char sl;

  /* local register variables, for clarity */
  unsigned short patch_num;
  unsigned short stage;
  unsigned short period;
  unsigned short block;
  unsigned short pattern;
  unsigned short step;
  unsigned short level;
  unsigned short mantissa;

  /* other local variables */
  unsigned short mask;
  unsigned short delta;
  unsigned short increment;
  unsigned short speed;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* check if period has elapsed */
    if (APU_WAVE_REG(m, ENV_PERIOD) > 0)
    {
      APU_WAVE_REG(m, ENV_PERIOD) -= 1;
      continue;
    }

    /* load registers to local variables */
    patch_num = APU_WAVE_REG(m, PATCH_NO);
    stage     = APU_WAVE_REG(m, ENV_STAGE);
    period    = APU_WAVE_REG(m, ENV_PERIOD);
    block     = APU_WAVE_REG(m, ENV_BLOCK);
    pattern   = APU_WAVE_REG(m, ENV_PATTERN);
    step      = APU_WAVE_REG(m, ENV_STEP);
    level     = APU_WAVE_REG(m, ENV_LEVEL);
    mantissa  = APU_WAVE_REG(m, ENV_MANTISSA);

    /* load patch params to local variables */
    ar = APU_PATCH_PARAM(patch_num, ENV_AR);
    dr = APU_PATCH_PARAM(patch_num, ENV_DR);
    sr = APU_PATCH_PARAM(patch_num, ENV_SR);
    rr = APU_PATCH_PARAM(patch_num, ENV_RR);
    sl = APU_PATCH_PARAM(patch_num, ENV_SL);

    ar = (ar > 99) ? 99 : ar;
    dr = (dr > 99) ? 99 : dr;
    sr = (sr > 99) ? 99 : sr;
    rr = (rr > 99) ? 99 : rr;
    sl = (sl > 99) ? 99 : sl;

    /* update pattern step */
    step += 1;
    step &= 0x0F;

    /* determine delta for this step */
    if (step == 0)
      mask = 1;
    else
      mask = 1 << step;

    if (block <= APU_ENV_RATE_BASE_BLOCK)
    {
      if (S_apu_env_step_patterns[8 + pattern] & mask)
        delta = 1;
      else
        delta = 0;
    }
    else
    {
      if (S_apu_env_step_patterns[2 * pattern] & mask)
        delta = 1 + block - APU_ENV_RATE_BASE_BLOCK;
      else
        delta = 0 + block - APU_ENV_RATE_BASE_BLOCK;

      if (delta > 4)
        delta = 4;
    }

    /* update level */
    if (delta > 0)
    {
      if (stage == APU_ENV_STAGE_A)
      {
        increment = level >> (5 - delta);

        if (increment == 0)
          increment = 1;

        if (level >= increment)
          level -= increment;
        else
          level = 0;

        if (level == 0)
          stage = APU_ENV_STAGE_D;
      }
      else
      {
        if (delta > 1)
          increment = 1 << (delta - 1);
        else
          increment = 1;

        level += increment;

        if (level > APU_ENV_MAX_LEVEL)
          level = APU_ENV_MAX_LEVEL;

        if ((stage == APU_ENV_STAGE_D) && 
            (level >= S_apu_env_sustain_level_map[sl]))
        {
          stage = APU_ENV_STAGE_S;
        }
      }
    }

    /* reset period countdown timer */
    if (stage == APU_ENV_STAGE_A)
      speed = S_apu_env_rise_time_map[ar];
    else if (stage == APU_ENV_STAGE_D)
      speed = S_apu_env_fall_time_map[dr];
    else if (stage == APU_ENV_STAGE_S)
      speed = S_apu_env_fall_time_map[sr];
    else
      speed = S_apu_env_fall_time_map[rr];

    block   = speed / APU_ENV_RATE_PATTERNS_PER_BLOCK;
    pattern = speed % APU_ENV_RATE_PATTERNS_PER_BLOCK;

    if (block < APU_ENV_RATE_BASE_BLOCK)
      period = 1 << (APU_ENV_RATE_BASE_BLOCK - block);
    else
      period = 1;

    /* store local variables to registers */
    APU_WAVE_REG(m, ENV_STAGE)    = stage;
    APU_WAVE_REG(m, ENV_PERIOD)   = period;
    APU_WAVE_REG(m, ENV_BLOCK)    = block;
    APU_WAVE_REG(m, ENV_PATTERN)  = pattern;
    APU_WAVE_REG(m, ENV_STEP)     = step;
    APU_WAVE_REG(m, ENV_LEVEL)    = level;
    APU_WAVE_REG(m, ENV_MANTISSA) = mantissa;
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_osc()                                                          */
/******************************************************************************/
int apu_advance_osc()
{
  int m;

  /* local patch param variables, for clarity */

  /* local register variables, for clarity */
  unsigned short patch_num;
  unsigned short note;
  unsigned short index;
  unsigned short mantissa;
  unsigned short osc_level;
  unsigned short env_level;

  /* other local variables */
  unsigned short block;
  unsigned short entry;
  unsigned short step;

  int current_pitch;
  int phase_inc;
  int final_level;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* load registers to local variables */
    patch_num = APU_WAVE_REG(m, PATCH_NO);
    note      = APU_WAVE_REG(m, NOTE);
    index     = APU_WAVE_REG(m, OSC_INDEX);
    mantissa  = APU_WAVE_REG(m, OSC_MANTISSA);
    osc_level = APU_WAVE_REG(m, OSC_LEVEL);
    env_level = APU_WAVE_REG(m, ENV_LEVEL);

    /* determine current pitch */
    current_pitch = 64 * note;

    if (current_pitch < 0)
      current_pitch = 0;
    else if (current_pitch > APU_OSC_MAX_PITCH)
      current_pitch = APU_OSC_MAX_PITCH;

    /* lookup phase increment */
    block = current_pitch / (12 * 64);
    entry = (current_pitch % (12 * 64)) / 16;
    step  = (current_pitch % (12 * 64)) % 16;

    phase_inc = S_apu_osc_pitch_table[entry];
    phase_inc += (step * S_apu_osc_pitch_deltas[entry]) / 16;

    if (block < APU_OSC_PITCH_BASE_BLOCK)
      phase_inc = phase_inc >> (APU_OSC_PITCH_BASE_BLOCK - block);
    else if (block > APU_OSC_PITCH_BASE_BLOCK)
      phase_inc = phase_inc << (block - APU_OSC_PITCH_BASE_BLOCK);

    /* update phase (10.10 fixed point) */
    mantissa += phase_inc & 0x3FF; 

    index += (phase_inc >> 10) & 0x3FF;
    index += (mantissa >> 10) & 0x3FF;

    index    &= 0x3FF;
    mantissa &= 0x3FF;

    /* sine wavetable lookup */
    if (index < 256)
      final_level = S_apu_osc_sine_table[index];
    else if (index < 512)
      final_level = S_apu_osc_sine_table[511 - index];
    else if (index < 768)
      final_level = S_apu_osc_sine_table[index - 512];
    else
      final_level = S_apu_osc_sine_table[1023 - index];
    
    /* apply envelope */
    final_level += env_level << 2;

    if (final_level > APU_OSC_MAX_LEVEL)
      final_level = APU_OSC_MAX_LEVEL;
    else if (final_level < 0)
      final_level = 0;

    /* set final level and sign */
    osc_level = final_level & 0x0FFF;

    if (index >= 512)
      osc_level |= 0x1000;

    /* store local variables to registers */
    APU_WAVE_REG(m, OSC_INDEX)    = index;
    APU_WAVE_REG(m, OSC_MANTISSA) = mantissa;
    APU_WAVE_REG(m, OSC_LEVEL)    = osc_level;
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_out()                                                          */
/******************************************************************************/
int apu_advance_out()
{
  int m;
  int n;

  int samp[2];

  unsigned short val;
  unsigned short block;
  unsigned short entry;

  unsigned int level;

  /* 2 channels (left & right) */
  for (n = 0; n < 2; n++)
  {
    /* compute mixed output (14 bit signed) */
    samp[n] = 0;

    for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
    {
      if (n == 0)
        val = APU_WAVE_REG(m, OSC_LEVEL); /* OSC_LEVEL_LEFT in the future...  */
      else
        val = APU_WAVE_REG(m, OSC_LEVEL); /* OSC_LEVEL_RIGHT in the future... */

      block = (val & 0x0FFF) / APU_OSC_LEVEL_TABLE_SIZE;
      entry = (val & 0x0FFF) % APU_OSC_LEVEL_TABLE_SIZE;

      level = S_apu_osc_level_table[entry];

      if (block > 0)
      {
        if (block >= APU_OSC_LEVEL_ZERO_BLOCK)
          level = 0;
        else
          level = level >> block;
      }

      if (val & 0x1000)
        samp[n] -= level;
      else
        samp[n] += level;
    }

    if (samp[n] > 8191)
      samp[n] = 8191;
    else if (samp[n] < -8192)
      samp[n] = -8192;

    /* apply dac (9 bits signed input, 16 bits signed output) */
    samp[n] = (samp[n] + 8192) / 32;

    if (samp[n] >= 256)
      samp[n] = (APU_DAC_POS_MULT * (samp[n] - 256)) / 64;
    else
      samp[n] = -32768 + ((APU_DAC_NEG_MULT * samp[n]) / 64);

    if (samp[n] > 32767)
      samp[n] = 32767;
    else if (samp[n] < -32768)
      samp[n] = -32768;

    /* apply highpass filter */
    S_apu_hp_in[2 * n + 1]  = S_apu_hp_in[2 * n + 0];
    S_apu_hp_out[2 * n + 1] = S_apu_hp_out[2 * n + 0];

    S_apu_hp_in[2 * n + 0] = samp[n];
    samp[n] = ((APU_HP_MULT_B0 * S_apu_hp_in[2 * n + 0]) / 32768) + 
              ((APU_HP_MULT_B1 * S_apu_hp_in[2 * n + 1]) / 32768) - 
              ((APU_HP_MULT_A1 * S_apu_hp_out[2 * n + 1]) / 32768);

    if (samp[n] > 32767)
      samp[n] = 32767;
    else if (samp[n] < -32768)
      samp[n] = -32768;

    S_apu_hp_out[2 * n + 0] = samp[n];

    /* apply lowpass filter */
    S_apu_lp_in[2 * n + 1]  = S_apu_lp_in[2 * n + 0];
    S_apu_lp_out[2 * n + 1] = S_apu_lp_out[2 * n + 0];

    S_apu_lp_in[2 * n + 0] = samp[n];
    samp[n] = ((APU_LP_MULT_B0 * S_apu_lp_in[2 * n + 0]) / 32768) + 
              ((APU_LP_MULT_B1 * S_apu_lp_in[2 * n + 1]) / 32768) - 
              ((APU_LP_MULT_A1 * S_apu_lp_out[2 * n + 1]) / 32768);

    if (samp[n] > 32767)
      samp[n] = 32767;
    else if (samp[n] < -32768)
      samp[n] = -32768;

    S_apu_lp_out[2 * n + 0] = samp[n];
  }

  /* update downsampler filter input buffers (left & right) */
  S_apu_ds_L_in[S_apu_ds_buf_pos] = S_apu_lp_out[2 * 0 + 0];
  S_apu_ds_R_in[S_apu_ds_buf_pos] = S_apu_lp_out[2 * 1 + 0];

  S_apu_ds_buf_pos = (S_apu_ds_buf_pos + 1) % APU_DS_BUFFER_SIZE;

  return 0;
}

/******************************************************************************/
/* apu_compute_sample()                                                       */
/******************************************************************************/
int apu_compute_sample()
{
  int m;

  int samp_L;
  int samp_R;

  int adj_pos;
  int inv_pos;

  short mult;

  /* apply downsampler filters (left & right) */
  adj_pos = (S_apu_ds_buf_pos + (APU_DS_M / 2)) % APU_DS_BUFFER_SIZE;

  mult = S_apu_ds_kernel[APU_DS_M / 2];

  samp_L = (mult * S_apu_ds_L_in[adj_pos]) / 32768;
  samp_R = (mult * S_apu_ds_R_in[adj_pos]) / 32768;

  for (m = 0; m < (APU_DS_M / 2); m++)
  {
    adj_pos = (S_apu_ds_buf_pos + m) % APU_DS_BUFFER_SIZE;
    inv_pos = (S_apu_ds_buf_pos + APU_DS_M - m) % APU_DS_BUFFER_SIZE;

    mult = S_apu_ds_kernel[m];

    samp_L += 
      (mult * (S_apu_ds_L_in[adj_pos] + S_apu_ds_L_in[inv_pos])) / 32768;

    samp_R += 
      (mult * (S_apu_ds_R_in[adj_pos] + S_apu_ds_R_in[inv_pos])) / 32768;
  }

  if (samp_L > 32767)
    samp_L = 32767;
  else if (samp_L < -32768)
    samp_L = -32768;

  if (samp_R > 32767)
    samp_R = 32767;
  else if (samp_R < -32768)
    samp_R = -32768;

  G_apu_out_L = samp_L;
  G_apu_out_R = samp_R;

  return 0;
}

/******************************************************************************/
/* apu_update()                                                               */
/******************************************************************************/
int apu_update()
{
  int m;

  for (m = 0; m < APU_CLOCKS_PER_SAMPLE; m++)
  {
    if ((S_apu_timer % APU_SEQ_DIVIDER) == 0)
      apu_advance_sequencer();

    if ((S_apu_timer % APU_LFO_DIVIDER) == 0)
      apu_advance_lfo();

    if ((S_apu_timer % APU_ENV_DIVIDER) == 0)
      apu_advance_env();

    if ((S_apu_timer % APU_OSC_DIVIDER) == 0)
      apu_advance_osc();

#if 0
    if ((S_apu_timer % APU_PCM_DIVIDER) == 0)
      apu_advance_pcm();
#endif

    apu_advance_out();

    S_apu_timer += 1;

    if ((S_apu_timer % APU_TMR_DIVIDER) == 0)
      S_apu_timer = 0;
  }

  apu_compute_sample();

  return 0;
}

