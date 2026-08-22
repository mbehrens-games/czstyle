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

#define APU_SEQ_DIVIDER  8  /* seq clock is  6000 */
#define APU_LFO_DIVIDER 32  /* lfo clock is  1500 */
#define APU_ENV_DIVIDER  3  /* env clock is 16000 */
#define APU_OSC_DIVIDER  1  /* osc clock is 48000 */
#define APU_SYN_DIVIDER  1  /* syn clock is 48000 */
#define APU_PCM_DIVIDER  1  /* pcm clock is 48000 */

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

/* volume and panning (15 bit mantissas) */
static unsigned short S_apu_inst_vol_table[128] = 
  {     0,     2,     8,    18,    33,    51,    73,   100,
      130,   165,   203,   246,   293,   343,   398,   457,
      520,   587,   658,   733,   813,   896,   983,  1075,
     1170,  1270,  1373,  1481,  1593,  1709,  1828,  1952,
     2080,  2212,  2349,  2489,  2633,  2781,  2934,  3090,
     3251,  3415,  3584,  3756,  3933,  4114,  4299,  4488,
     4681,  4878,  5079,  5284,  5494,  5707,  5924,  6146,
     6371,  6601,  6834,  7072,  7314,  7560,  7810,  8064,
     8322,  8584,  8850,  9120,  9394,  9673,  9955, 10241,
    10532, 10827, 11125, 11428, 11735, 12045, 12360, 12679,
    13002, 13329, 13661, 13996, 14335, 14678, 15026, 15377,
    15733, 16092, 16456, 16824, 17196, 17571, 17951, 18335,
    18723, 19116, 19512, 19912, 20316, 20725, 21137, 21553,
    21974, 22399, 22827, 23260, 23697, 24138, 24583, 25032,
    25485, 25942, 26403, 26868, 27337, 27811, 28288, 28770,
    29255, 29745, 30239, 30736, 31238, 31744, 32254, 32768
  };  

static unsigned short S_apu_inst_pan_L_table[128] = 
  { 32768, 32766, 32758, 32746, 32729, 32706, 32679, 32647,
    32610, 32568, 32522, 32470, 32413, 32352, 32286, 32214,
    32138, 32058, 31972, 31881, 31786, 31686, 31581, 31471,
    31357, 31238, 31114, 30986, 30853, 30715, 30572, 30425,
    30274, 30118, 29957, 29792, 29622, 29448, 29269, 29086,
    28899, 28707, 28511, 28311, 28106, 27897, 27684, 27467,
    27246, 27020, 26791, 26557, 26320, 26078, 25833, 25583,
    25330, 25073, 24812, 24548, 24279, 24008, 23732, 23453,
    23170, 22737, 22443, 22146, 21846, 21542, 21235, 20925,
    20611, 20294, 19975, 19652, 19326, 18997, 18666, 18331,
    17994, 17654, 17311, 16965, 16617, 16267, 15914, 15558,
    15200, 14840, 14478, 14113, 13746, 13377, 13006, 12633,
    12258, 11882, 11503, 11123, 10741, 10357,  9972,  9585,
     9196,  8807,  8416,  8023,  7630,  7235,  6839,  6442,
     6045,  5646,  5246,  4846,  4444,  4043,  3640,  3237,
     2833,  2430,  2025,  1620,  1216,   810,   405,     0
  };

static unsigned short S_apu_inst_pan_R_table[128] = 
  {     0,   402,   804,  1206,  1608,  2009,  2411,  2811,
     3212,  3612,  4011,  4410,  4808,  5205,  5602,  5998,
     6393,  6787,  7180,  7571,  7962,  8351,  8740,  9127,
     9512,  9896, 10279, 10660, 11039, 11417, 11793, 12167,
    12540, 12910, 13279, 13646, 14010, 14373, 14733, 15091,
    15447, 15800, 16151, 16500, 16846, 17190, 17531, 17869,
    18205, 18538, 18868, 19195, 19520, 19841, 20160, 20475,
    20788, 21097, 21403, 21706, 22006, 22302, 22595, 22884,
    23170, 23596, 23876, 24151, 24424, 24692, 24956, 25217,
    25474, 25727, 25976, 26221, 26462, 26699, 26932, 27161,
    27386, 27606, 27822, 28034, 28242, 28445, 28644, 28839,
    29029, 29215, 29396, 29573, 29745, 29913, 30076, 30235,
    30389, 30538, 30683, 30823, 30958, 31088, 31214, 31335,
    31451, 31562, 31669, 31771, 31867, 31959, 32046, 32128,
    32206, 32278, 32345, 32408, 32465, 32518, 32565, 32608,
    32645, 32678, 32705, 32728, 32745, 32758, 32765, 32768
  };

/*******/
/* LFO */
/*******/



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

#define APU_ENV_MAX_RATE  ((16 * 8) - 1)    /* 127  */
#define APU_ENV_MAX_INDEX ((16 * 64) - 1)   /* 1023 */
#define APU_ENV_MAX_LEVEL ((16 * 256) - 1)  /* 4095 */

#define APU_ENV_RATE_BASE_BLOCK 11
#define APU_ENV_ZERO_INDEX      832

/* step patterns */
static unsigned short S_apu_env_step_patterns[16] = 
  { 0x0000, 0x0080, 0x0808, 0x0888, 0x2222, 0x22A2, 0x2A2A, 0x2AAA,
    0x5555, 0x55D5, 0x5D5D, 0x5DDD, 0x7777, 0x77F7, 0x7F7F, 0x7FFF
  };

/* parameter mapping */
static unsigned short S_apu_env_adsr_rate_map[100] = 
  { 127, 126, 124, 123, 122, 121, 119, 118, 117, 115,
    114, 113, 112, 110, 109, 108, 106, 105, 104, 103,
    101, 100,  99,  97,  96,  95,  94,  92,  91,  90,
     89,  87,  86,  85,  83,  82,  81,  80,  78,  77,
     76,  74,  73,  72,  71,  69,  68,  67,  65,  64,
     63,  62,  60,  59,  58,  56,  55,  54,  53,  51,
     50,  49,  47,  46,  45,  44,  42,  41,  40,  38,
     37,  36,  35,  33,  32,  31,  30,  28,  27,  26,
     24,  23,  22,  21,  19,  18,  17,  15,  14,  13,
     12,  10,   9,   8,   6,   5,   4,   3,   1,   0
  };

static unsigned short S_apu_env_total_level_map[100] = 
  { 1023,  824,  815,  807,  798,  790,  782,  773,  765,  756,
     748,  740,  731,  723,  714,  706,  698,  689,  681,  672,
     664,  656,  647,  639,  630,  622,  613,  605,  597,  588,
     580,  571,  563,  555,  546,  538,  529,  521,  513,  504,
     496,  487,  479,  471,  462,  454,  445,  437,  429,  420,
     412,  403,  395,  387,  378,  370,  361,  353,  345,  336,
     328,  319,  311,  303,  294,  286,  277,  269,  261,  252,
     244,  235,  227,  219,  210,  202,  193,  185,  176,  168,
     160,  151,  143,  134,  126,  118,  109,  101,   92,   84,
      76,   67,   59,   50,   42,   34,   25,   17,    8,    0
  };

static unsigned short S_apu_env_sustain_level_map[100] = 
  { 1023,  448,  443,  439,  434,  430,  425,  421,  416,  412,
     407,  403,  398,  394,  389,  385,  380,  376,  371,  367,
     362,  357,  353,  348,  344,  339,  335,  330,  326,  321,
     317,  312,  308,  303,  299,  294,  290,  285,  281,  276,
     272,  267,  262,  258,  253,  249,  244,  240,  235,  231,
     226,  222,  217,  213,  208,  204,  199,  195,  190,  186,
     181,  176,  172,  167,  163,  158,  154,  149,  145,  140,
     136,  131,  127,  122,  118,  113,  109,  104,  100,   95,
      91,   86,   81,   77,   72,   68,   63,   59,   54,   50,
      45,   41,   36,   32,   27,   23,   18,   14,    9,    5
  };

static unsigned short S_apu_env_rate_ks_map[100] = 
  {   21,   22,   22,   23,   23,   24,   24,   25,   25,   26,
      26,   27,   27,   28,   29,   29,   30,   30,   31,   32,
      32,   33,   34,   35,   35,   36,   37,   38,   38,   39,
      40,   41,   42,   43,   44,   44,   45,   46,   47,   48,
      49,   50,   52,   53,   54,   55,   56,   57,   58,   60,
      61,   62,   64,   65,   66,   68,   69,   71,   72,   74,
      75,   77,   78,   80,   82,   84,   85,   87,   89,   91,
      93,   95,   97,   99,  101,  103,  105,  108,  110,  112,
     115,  117,  119,  122,  125,  127,  130,  133,  135,  138,
     141,  144,  147,  150,  154,  157,  160,  164,  167,  171
  };

static unsigned short S_apu_env_level_ks_map[100] = 
  {  171,  174,  178,  182,  186,  190,  194,  198,  202,  206,
     211,  215,  220,  224,  229,  234,  239,  244,  249,  254,
     260,  265,  271,  277,  283,  289,  295,  301,  307,  314,
     320,  327,  334,  341,  349,  356,  364,  371,  379,  387,
     395,  404,  412,  421,  430,  439,  449,  458,  468,  478,
     488,  498,  509,  520,  531,  542,  553,  565,  577,  589,
     602,  615,  628,  641,  655,  668,  683,  697,  712,  727,
     743,  758,  774,  791,  808,  825,  842,  860,  878,  897,
     916,  935,  955,  976,  996, 1017, 1039, 1061, 1084, 1107,
    1130, 1154, 1179, 1204, 1229, 1255, 1282, 1309, 1337, 1365
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



/*******/
/* OUT */
/*******/

/* dac (6 bit mantissas) */
#define APU_DAC_POS_MULT 8224
#define APU_DAC_NEG_MULT 8160

/* highpass filters (15 bit mantissas) */
#define APU_HP_MULT_A0  32768
#define APU_HP_MULT_A1 -32631
#define APU_HP_MULT_B0  32700
#define APU_HP_MULT_B1 -32700

static short S_apu_hp_in[4];  /* 2 channels, 2 inputs each  */
static short S_apu_hp_out[4]; /* 2 channels, 2 outputs each */

/* lowpass filters (15 bit mantissas) */
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

/* fm voices */
enum
{
  APU_KBD_REG_PATCH_NO = 0, 
  APU_KBD_REG_VOLUME, 
  APU_KBD_REG_PANNING, 
  APU_KBD_REG_NOTE, 
  APU_KBD_REG_VELOCITY, 
  APU_KBD_REG_WHEEL_PITCH, 
  APU_KBD_REG_WHEEL_VIB, 
  APU_KBD_REG_WHEEL_TREM, 
  APU_KBD_REG_WHEEL_BOOST, 
  APU_KBD_REG_SW_PORTA, 
  APU_KBD_REG_SW_SUSTAIN, 
  APU_NUM_KBD_REGS 
};

enum
{
  APU_SYN_REG_FEEDIN_0 = 0, 
  APU_SYN_REG_FEEDIN_1, 
  APU_SYN_REG_LEVEL, 
  APU_NUM_SYN_REGS 
};

enum
{
  APU_LFO_REG_INDEX = 0, 
  APU_LFO_REG_MANTISSA, 
  APU_LFO_REG_VIB_LEVEL, 
  APU_LFO_REG_TREM_LEVEL, 
  APU_NUM_LFO_REGS 
};

enum
{
  APU_ENV_REG_STAGE = 0, 
  APU_ENV_REG_PERIOD, 
  APU_ENV_REG_BLOCK, 
  APU_ENV_REG_PATTERN, 
  APU_ENV_REG_STEP, 
  APU_ENV_REG_INDEX, 
  APU_ENV_REG_MANTISSA, 
  APU_ENV_REG_LEVEL, 
  APU_NUM_ENV_REGS 
};

enum
{
  APU_OSC_REG_INDEX = 0, 
  APU_OSC_REG_MANTISSA, 
  APU_NUM_OSC_REGS 
};

#define APU_NUM_FM_VOICES (9 + 1)

#define APU_NUM_KBDS (1 * APU_NUM_FM_VOICES)
#define APU_NUM_SYNS (1 * APU_NUM_FM_VOICES)
#define APU_NUM_LFOS (1 * APU_NUM_FM_VOICES)
#define APU_NUM_ENVS (4 * APU_NUM_FM_VOICES)
#define APU_NUM_OSCS (4 * APU_NUM_FM_VOICES)

#define APU_KBD_REGS_BANK_SIZE (APU_NUM_KBDS * APU_NUM_KBD_REGS)
#define APU_SYN_REGS_BANK_SIZE (APU_NUM_SYNS * APU_NUM_SYN_REGS)
#define APU_LFO_REGS_BANK_SIZE (APU_NUM_LFOS * APU_NUM_LFO_REGS)
#define APU_ENV_REGS_BANK_SIZE (APU_NUM_ENVS * APU_NUM_ENV_REGS)
#define APU_OSC_REGS_BANK_SIZE (APU_NUM_OSCS * APU_NUM_OSC_REGS)

static unsigned short S_apu_kbd_regs_bank[APU_KBD_REGS_BANK_SIZE];
static unsigned short S_apu_syn_regs_bank[APU_SYN_REGS_BANK_SIZE];
static unsigned short S_apu_lfo_regs_bank[APU_LFO_REGS_BANK_SIZE];
static unsigned short S_apu_env_regs_bank[APU_ENV_REGS_BANK_SIZE];
static unsigned short S_apu_osc_regs_bank[APU_OSC_REGS_BANK_SIZE];

#define APU_KBD_REG(v_no, reg)                                                 \
  S_apu_kbd_regs_bank[(v_no) * APU_NUM_KBD_REGS + APU_KBD_REG_##reg]

#define APU_SYN_REG(v_no, reg)                                                 \
  S_apu_syn_regs_bank[(v_no) * APU_NUM_SYN_REGS + APU_SYN_REG_##reg]

#define APU_LFO_REG(v_no, reg)                                                 \
  S_apu_lfo_regs_bank[(v_no) * APU_NUM_LFO_REGS + APU_LFO_REG_##reg]

#define APU_ENV_REG(v_no, e_no, reg)                                           \
  S_apu_env_regs_bank[(4 * v_no + e_no) * APU_NUM_ENV_REGS + APU_ENV_REG_##reg]

#define APU_OSC_REG(v_no, o_no, reg)                                           \
  S_apu_osc_regs_bank[(4 * v_no + o_no) * APU_NUM_OSC_REGS + APU_OSC_REG_##reg]

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
  APU_PATCH_PARAM_SYN_FB = 0, 
  APU_PATCH_PARAM_SYN_ALG, 
  APU_PATCH_PARAM_ENV_AR, 
  APU_PATCH_PARAM_ENV_DR, 
  APU_PATCH_PARAM_ENV_SR, 
  APU_PATCH_PARAM_ENV_RR, 
  APU_PATCH_PARAM_ENV_SL, 
  APU_PATCH_PARAM_ENV_TL, 
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
  int n;

  S_apu_timer = 0;

  /* reset fm voice registers */
  for (m = 0; m < APU_NUM_KBDS; m++)
  {
    APU_KBD_REG(m, PATCH_NO) = 0;
    APU_KBD_REG(m, VOLUME)   = 0;
    APU_KBD_REG(m, PANNING)  = 64;
    APU_KBD_REG(m, NOTE)     = 0;
    APU_KBD_REG(m, VELOCITY) = 0;

    APU_KBD_REG(m, WHEEL_PITCH) = 0;
    APU_KBD_REG(m, WHEEL_VIB)   = 0;
    APU_KBD_REG(m, WHEEL_TREM)  = 0;
    APU_KBD_REG(m, WHEEL_BOOST) = 0;
    APU_KBD_REG(m, SW_PORTA)    = 0;
    APU_KBD_REG(m, SW_SUSTAIN)  = 0;
  }

  for (m = 0; m < APU_NUM_SYNS; m++)
  {
    APU_SYN_REG(m, FEEDIN_0)  = 0;
    APU_SYN_REG(m, FEEDIN_1)  = 0;
    APU_SYN_REG(m, LEVEL)     = 0;
  }

  for (m = 0; m < APU_NUM_LFOS; m++)
  {
    APU_LFO_REG(m, INDEX)       = 0;
    APU_LFO_REG(m, MANTISSA)    = 0;
    APU_LFO_REG(m, VIB_LEVEL)   = 0;
    APU_LFO_REG(m, TREM_LEVEL)  = 0;
  }

  for (m = 0; m < APU_NUM_FM_VOICES; m++)
  {
    for (n = 0; n < 4; n++)
    {
      APU_ENV_REG(m, n, STAGE)    = APU_ENV_STAGE_R;
      APU_ENV_REG(m, n, PERIOD)   = 0;
      APU_ENV_REG(m, n, BLOCK)    = 0;
      APU_ENV_REG(m, n, PATTERN)  = 0;
      APU_ENV_REG(m, n, STEP)     = 0;
      APU_ENV_REG(m, n, INDEX)    = APU_ENV_MAX_INDEX;
      APU_ENV_REG(m, n, MANTISSA) = 0;
      APU_ENV_REG(m, n, LEVEL)    = APU_ENV_MAX_LEVEL;
    }
  }

  for (m = 0; m < APU_NUM_FM_VOICES; m++)
  {
    for (n = 0; n < 4; n++)
    {
      APU_OSC_REG(m, n, INDEX)    = 0;
      APU_OSC_REG(m, n, MANTISSA) = 0;
    }
  }

  /* reset other registers */
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
    APU_PATCH_PARAM(m, SYN_FB)  = 0;
    APU_PATCH_PARAM(m, SYN_ALG) = 0;

    APU_PATCH_PARAM(m, ENV_AR) = 0;
    APU_PATCH_PARAM(m, ENV_DR) = 0;
    APU_PATCH_PARAM(m, ENV_SR) = 0;
    APU_PATCH_PARAM(m, ENV_RR) = 0;
    APU_PATCH_PARAM(m, ENV_SL) = 0;
    APU_PATCH_PARAM(m, ENV_TL) = 0;

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
  APU_KBD_REG(0, VOLUME)   = 127;
  APU_KBD_REG(0, PANNING)  = 64;

  APU_PATCH_PARAM(0, ENV_AR) = 20;
  APU_PATCH_PARAM(0, ENV_DR) = 25;
  APU_PATCH_PARAM(0, ENV_SR) = 50;
  APU_PATCH_PARAM(0, ENV_RR) = 40;
  APU_PATCH_PARAM(0, ENV_SL) = 60;
  APU_PATCH_PARAM(0, ENV_TL) = 99;
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
  int n;

  unsigned short patch_num;
  unsigned short speed;

  if (inst_num >= APU_NUM_FM_VOICES)
    return 0;

  if (note >= 128)
    return 0;

  if (S_apu_seq_midi_note_number_table[note] == 0)
    return 0;

  APU_KBD_REG(inst_num, NOTE) = S_apu_seq_midi_note_number_table[note];

  APU_LFO_REG(inst_num, INDEX)    = 0;
  APU_LFO_REG(inst_num, MANTISSA) = 0;

  for (n = 0; n < 4; n++)
  {
    APU_ENV_REG(inst_num, n, STAGE)    = APU_ENV_STAGE_A;
    APU_ENV_REG(inst_num, n, STEP)     = 0;
    APU_ENV_REG(inst_num, n, MANTISSA) = 0;
  }

  for (n = 0; n < 4; n++)
  {
    APU_OSC_REG(inst_num, n, INDEX)    = 0;
    APU_OSC_REG(inst_num, n, MANTISSA) = 0;
  }

  /* initialize envelope block & pattern */
  patch_num = APU_KBD_REG(inst_num, PATCH_NO);

  for (n = 0; n < 4; n++)
  {
    speed = S_apu_env_adsr_rate_map[APU_PATCH_PARAM(patch_num, ENV_AR)];

    APU_ENV_REG(inst_num, n, BLOCK)   = speed / APU_ENV_RATE_PATTERNS_PER_BLOCK;
    APU_ENV_REG(inst_num, n, PATTERN) = speed % APU_ENV_RATE_PATTERNS_PER_BLOCK;
    APU_ENV_REG(inst_num, n, PERIOD)  = 1;
  }

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

  for (m = 0; m < APU_NUM_LFOS; m++)
  {

  }

  return 0;
}

/******************************************************************************/
/* apu_advance_env()                                                          */
/******************************************************************************/
int apu_advance_env()
{
  int m;
  int n;

  /* local patch param variables, for clarity */
  unsigned char ar;
  unsigned char dr;
  unsigned char sr;
  unsigned char rr;
  unsigned char sl;
  unsigned char tl;

  /* local register variables, for clarity */
  unsigned short patch_num;
  unsigned short stage;
  unsigned short period;
  unsigned short block;
  unsigned short pattern;
  unsigned short step;
  unsigned short index;
  unsigned short mantissa;
  unsigned short level;

  /* other local variables */
  unsigned short mask;
  unsigned short delta;
  unsigned short increment;
  unsigned short speed;

  for (m = 0; m < APU_NUM_FM_VOICES; m++)
  {
    for (n = 0; n < 4; n++)
    {
      /* check if period has elapsed */
      if (APU_ENV_REG(m, n, PERIOD) > 0)
      {
        APU_ENV_REG(m, n, PERIOD) -= 1;
        continue;
      }

      /* load registers to local variables */
      stage     = APU_ENV_REG(m, n, STAGE);
      period    = APU_ENV_REG(m, n, PERIOD);
      block     = APU_ENV_REG(m, n, BLOCK);
      pattern   = APU_ENV_REG(m, n, PATTERN);
      step      = APU_ENV_REG(m, n, STEP);
      index     = APU_ENV_REG(m, n, INDEX);
      mantissa  = APU_ENV_REG(m, n, MANTISSA);
      level     = APU_ENV_REG(m, n, LEVEL);

      /* load patch params to local variables */
      patch_num = APU_KBD_REG(m, PATCH_NO);

      ar = APU_PATCH_PARAM(patch_num, ENV_AR);
      dr = APU_PATCH_PARAM(patch_num, ENV_DR);
      sr = APU_PATCH_PARAM(patch_num, ENV_SR);
      rr = APU_PATCH_PARAM(patch_num, ENV_RR);
      sl = APU_PATCH_PARAM(patch_num, ENV_SL);
      tl = APU_PATCH_PARAM(patch_num, ENV_TL);

      ar = (ar > 99) ? 99 : ar;
      dr = (dr > 99) ? 99 : dr;
      sr = (sr > 99) ? 99 : sr;
      rr = (rr > 99) ? 99 : rr;
      sl = (sl > 99) ? 99 : sl;
      tl = (tl > 99) ? 99 : tl;

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

      /* update index */
      if (delta > 0)
      {
        if (stage == APU_ENV_STAGE_A)
        {
          increment = index >> (5 - delta);

          if (increment == 0)
            increment = 1;

          if (index >= increment)
            index -= increment;
          else
            index = 0;

          if (index == 0)
            stage = APU_ENV_STAGE_D;
        }
        else
        {
          if (delta > 1)
            increment = 1 << (delta - 1);
          else
            increment = 1;

          index += increment;

          if (index > APU_ENV_MAX_INDEX)
            index = APU_ENV_MAX_INDEX;

          if ((stage == APU_ENV_STAGE_D) && 
              (index >= S_apu_env_sustain_level_map[sl]))
          {
            stage = APU_ENV_STAGE_S;
          }
        }
      }

      /* update level */
      level = (index + S_apu_env_total_level_map[tl]) << 2;

      if (level > APU_ENV_MAX_LEVEL)
        level = APU_ENV_MAX_LEVEL;

      /* reset period countdown timer */
      if (stage == APU_ENV_STAGE_A)
        speed = S_apu_env_adsr_rate_map[ar];
      else if (stage == APU_ENV_STAGE_D)
        speed = S_apu_env_adsr_rate_map[dr];
      else if (stage == APU_ENV_STAGE_S)
        speed = S_apu_env_adsr_rate_map[sr];
      else
        speed = S_apu_env_adsr_rate_map[rr];

      block   = speed / APU_ENV_RATE_PATTERNS_PER_BLOCK;
      pattern = speed % APU_ENV_RATE_PATTERNS_PER_BLOCK;

      if (block < APU_ENV_RATE_BASE_BLOCK)
        period = 1 << (APU_ENV_RATE_BASE_BLOCK - block);
      else
        period = 1;

      /* store local variables to registers */
      APU_ENV_REG(m, n, STAGE)    = stage;
      APU_ENV_REG(m, n, PERIOD)   = period;
      APU_ENV_REG(m, n, BLOCK)    = block;
      APU_ENV_REG(m, n, PATTERN)  = pattern;
      APU_ENV_REG(m, n, STEP)     = step;
      APU_ENV_REG(m, n, INDEX)    = index;
      APU_ENV_REG(m, n, MANTISSA) = mantissa;
      APU_ENV_REG(m, n, LEVEL)    = level;
    }
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_osc()                                                          */
/******************************************************************************/
int apu_advance_osc()
{
  int m;
  int n;

  /* local patch param variables, for clarity */

  /* local register variables, for clarity */
  unsigned short patch_num;
  unsigned short note;
  unsigned short index;
  unsigned short mantissa;

  /* other local variables */
  unsigned short block;
  unsigned short entry;
  unsigned short step;

  int          current_pitch;
  unsigned int phase_inc;

  for (m = 0; m < APU_NUM_FM_VOICES; m++)
  {
    for (n = 0; n < 4; n++)
    {
      /* load registers to local variables */
      patch_num = APU_KBD_REG(m, PATCH_NO);
      note      = APU_KBD_REG(m, NOTE);
      index     = APU_OSC_REG(m, n, INDEX);
      mantissa  = APU_OSC_REG(m, n, MANTISSA);

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

      /* store local variables to registers */
      APU_OSC_REG(m, n, INDEX)    = index;
      APU_OSC_REG(m, n, MANTISSA) = mantissa;
    }
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_syn()                                                          */
/******************************************************************************/
int apu_advance_syn()
{
  int m;
  int n;

  /* local patch param variables, for clarity */
  unsigned char fb;
  unsigned char alg;

  /* local register variables, for clarity */
  unsigned short patch_num;
  unsigned short feedin_0;
  unsigned short feedin_1;
  unsigned short syn_level;

  /* other local variables */
  unsigned short adj_index;
  unsigned short adj_level;

  unsigned short block;
  unsigned short entry;

  int osc_level[4];
  int combined_level;

  for (m = 0; m < APU_NUM_FM_VOICES; m++)
  {
    /* load registers to local variables */
    feedin_0 = APU_SYN_REG(m, FEEDIN_0);
    feedin_1 = APU_SYN_REG(m, FEEDIN_1);
    syn_level = APU_SYN_REG(m, LEVEL);

    /* load patch params to local variables */
    patch_num = APU_KBD_REG(m, PATCH_NO);

    fb = APU_PATCH_PARAM(patch_num, SYN_FB);
    alg = APU_PATCH_PARAM(patch_num, SYN_ALG);

    fb = (fb > 99) ? 99 : fb;
    alg = (alg > 7) ? 7 : alg;

    for (n = 0; n < 4; n++)
    {
      adj_index = APU_OSC_REG(m, n, INDEX);

      /* sine wavetable lookup */
      if (adj_index < 256)
        adj_level = S_apu_osc_sine_table[adj_index];
      else if (adj_index < 512)
        adj_level = S_apu_osc_sine_table[511 - adj_index];
      else if (adj_index < 768)
        adj_level = S_apu_osc_sine_table[adj_index - 512];
      else
        adj_level = S_apu_osc_sine_table[1023 - adj_index];

      /* apply envelope */
      adj_level += APU_ENV_REG(m, n, LEVEL);

      if (adj_level > APU_OSC_MAX_LEVEL)
        adj_level = APU_OSC_MAX_LEVEL;

      /* set final adjusted level and sign */
      adj_level = adj_level & 0x0FFF;

      if (adj_index >= 512)
        adj_level |= 0x1000;

      /* convert from db to linear and store the output */
      block = (adj_level & 0x0FFF) / APU_OSC_LEVEL_TABLE_SIZE;
      entry = (adj_level & 0x0FFF) % APU_OSC_LEVEL_TABLE_SIZE;

      osc_level[n] = S_apu_osc_level_table[entry];

      if (block > 0)
      {
        if (block >= APU_OSC_LEVEL_ZERO_BLOCK)
          osc_level[n] = 0;
        else
          osc_level[n] = osc_level[n] >> block;
      }

      if (adj_level & 0x1000)
        osc_level[n] = -osc_level[n];
    }

    /* for now, just output the 1st operator... */
    combined_level = osc_level[0];

    if (combined_level > 8191)
      combined_level = 8191;
    else if (combined_level < -8191)
      combined_level = -8191;

    if (combined_level < 0)
      syn_level = ((-combined_level) & 0x1FFF) | 0x2000; 
    else
      syn_level = (combined_level & 0x1FFF);

    /* store local variables to registers */
    APU_SYN_REG(m, LEVEL) = syn_level;
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

  int samp;

  unsigned short val;
  unsigned short adj_level;
  unsigned short mult;

  /* 2 channels (left & right) */
  for (n = 0; n < 2; n++)
  {
    /* compute mixed output (14 bit signed) */
    samp = 0;

    for (m = 0; m < APU_NUM_FM_VOICES; m++)
    {
      val = APU_SYN_REG(m, LEVEL);
      adj_level = val & 0x1FFF;

      mult = S_apu_inst_vol_table[APU_KBD_REG(m, VOLUME)];
      adj_level = (adj_level * mult) / 32768;

      if (n == 0)
      {
        mult = S_apu_inst_pan_L_table[APU_KBD_REG(m, PANNING)];
        adj_level = (adj_level * mult) / 32768;
      }
      else
      {
        mult = S_apu_inst_pan_R_table[APU_KBD_REG(m, PANNING)];
        adj_level = (adj_level * mult) / 32768;
      }

      if (val & 0x2000)
        samp -= adj_level;
      else
        samp += adj_level;
    }

    if (samp > 8191)
      samp = 8191;
    else if (samp < -8192)
      samp = -8192;

    /* apply dac (9 bits signed input, 16 bits signed output) */
    samp = (samp + 8192) / 32;

    if (samp > 511)
      samp = 511;
    else if (samp < 0)
      samp = 0;

    if (samp >= 256)
      samp = (APU_DAC_POS_MULT * (samp - 256)) / 64;
    else
      samp = -32768 + ((APU_DAC_NEG_MULT * samp) / 64);

    if (samp > 32767)
      samp = 32767;
    else if (samp < -32768)
      samp = -32768;

    /* apply highpass filter */
    S_apu_hp_in[2 * n + 1]  = S_apu_hp_in[2 * n + 0];
    S_apu_hp_out[2 * n + 1] = S_apu_hp_out[2 * n + 0];

    S_apu_hp_in[2 * n + 0] = samp;
    samp =  ((APU_HP_MULT_B0 * S_apu_hp_in[2 * n + 0]) / 32768) + 
            ((APU_HP_MULT_B1 * S_apu_hp_in[2 * n + 1]) / 32768) - 
            ((APU_HP_MULT_A1 * S_apu_hp_out[2 * n + 1]) / 32768);

    if (samp > 32767)
      samp = 32767;
    else if (samp < -32768)
      samp = -32768;

    S_apu_hp_out[2 * n + 0] = samp;

    /* apply lowpass filter */
    S_apu_lp_in[2 * n + 1]  = S_apu_lp_in[2 * n + 0];
    S_apu_lp_out[2 * n + 1] = S_apu_lp_out[2 * n + 0];

    S_apu_lp_in[2 * n + 0] = samp;
    samp =  ((APU_LP_MULT_B0 * S_apu_lp_in[2 * n + 0]) / 32768) + 
            ((APU_LP_MULT_B1 * S_apu_lp_in[2 * n + 1]) / 32768) - 
            ((APU_LP_MULT_A1 * S_apu_lp_out[2 * n + 1]) / 32768);

    if (samp > 32767)
      samp = 32767;
    else if (samp < -32768)
      samp = -32768;

    S_apu_lp_out[2 * n + 0] = samp;
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

    apu_advance_syn();
    apu_advance_out();

    S_apu_timer += 1;

    if ((S_apu_timer % APU_TMR_DIVIDER) == 0)
      S_apu_timer = 0;
  }

  apu_compute_sample();

  return 0;
}

