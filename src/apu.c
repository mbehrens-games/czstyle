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

#define APU_SEQ_DIVIDER     8  /* seq clock is 6000  */
#define APU_LFO_DIVIDER    32  /* lfo clock is 1500  */
#define APU_ENV_DIVIDER     3  /* env clock is 16000 */
#define APU_OSC_DIVIDER     1  /* osc clock is 48000 */
#define APU_PCM_DIVIDER     2  /* pcm clock is 24000 */

#define APU_TIMER_DIVIDER  96  /* lcm of the other dividers */

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

/* rates and step patterns */
#define APU_ENV_RATE_NUM_BLOCKS 16
#define APU_ENV_RATE_TABLE_SIZE 8

#define APU_ENV_RATE_MAX_INDEX  ((16 * 8) - 1)

#define APU_ENV_RATE_BASE_BLOCK 11

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

#define APU_ENV_LEVEL_MAX_INDEX  ((16 * 64) - 1) /* 1023 */

/* envelope keyscaling */
#define APU_ENV_KEYSCALING_DIVISOR  256
#define APU_ENV_TIME_KS_BREAKPOINT  (APU_NOTE_MIDDLE_C - 4 * 12 + 0)  /* C-0 */
#define APU_ENV_LEVEL_KS_BREAKPOINT (APU_NOTE_MIDDLE_C - 2 * 12 + 9)  /* A-2 */

/*******/
/* OSC */
/*******/

/* pitch table */
#define APU_OSC_PITCH_NUM_BLOCKS 9
#define APU_OSC_PITCH_TABLE_SIZE 1024

#define APU_OSC_PITCH_MAX_INDEX  ((9 * 1024) - 1)

#define APU_OSC_PITCH_BASE_BLOCK 6

static unsigned short S_apu_vco_pitch_table[APU_OSC_PITCH_TABLE_SIZE] = 
  { 22861, 22877, 22892, 22908, 22923, 22939, 22954, 22970,
    22985, 23001, 23016, 23032, 23048, 23063, 23079, 23094,
    23110, 23126, 23141, 23157, 23173, 23188, 23204, 23220,
    23236, 23251, 23267, 23283, 23299, 23314, 23330, 23346,
    23362, 23378, 23393, 23409, 23425, 23441, 23457, 23473,
    23489, 23505, 23520, 23536, 23552, 23568, 23584, 23600,
    23616, 23632, 23648, 23664, 23680, 23696, 23712, 23728,
    23744, 23760, 23777, 23793, 23809, 23825, 23841, 23857,
    23873, 23890, 23906, 23922, 23938, 23954, 23970, 23987,
    24003, 24019, 24035, 24052, 24068, 24084, 24101, 24117,
    24133, 24150, 24166, 24182, 24199, 24215, 24232, 24248,
    24264, 24281, 24297, 24314, 24330, 24347, 24363, 24380,
    24396, 24413, 24429, 24446, 24462, 24479, 24495, 24512,
    24529, 24545, 24562, 24578, 24595, 24612, 24628, 24645,
    24662, 24678, 24695, 24712, 24729, 24745, 24762, 24779,
    24796, 24812, 24829, 24846, 24863, 24880, 24897, 24913,
    24930, 24947, 24964, 24981, 24998, 25015, 25032, 25049,
    25066, 25083, 25100, 25117, 25134, 25151, 25168, 25185,
    25202, 25219, 25236, 25253, 25270, 25287, 25304, 25321,
    25339, 25356, 25373, 25390, 25407, 25425, 25442, 25459,
    25476, 25493, 25511, 25528, 25545, 25563, 25580, 25597,
    25615, 25632, 25649, 25667, 25684, 25701, 25719, 25736,
    25754, 25771, 25789, 25806, 25823, 25841, 25858, 25876,
    25893, 25911, 25929, 25946, 25964, 25981, 25999, 26016,
    26034, 26052, 26069, 26087, 26105, 26122, 26140, 26158,
    26175, 26193, 26211, 26229, 26246, 26264, 26282, 26300,
    26318, 26335, 26353, 26371, 26389, 26407, 26425, 26443,
    26460, 26478, 26496, 26514, 26532, 26550, 26568, 26586,
    26604, 26622, 26640, 26658, 26676, 26694, 26712, 26730,
    26749, 26767, 26785, 26803, 26821, 26839, 26857, 26876,
    26894, 26912, 26930, 26949, 26967, 26985, 27003, 27022,
    27040, 27058, 27076, 27095, 27113, 27132, 27150, 27168,
    27187, 27205, 27224, 27242, 27260, 27279, 27297, 27316,
    27334, 27353, 27371, 27390, 27408, 27427, 27446, 27464,
    27483, 27501, 27520, 27539, 27557, 27576, 27595, 27613,
    27632, 27651, 27669, 27688, 27707, 27726, 27744, 27763,
    27782, 27801, 27820, 27838, 27857, 27876, 27895, 27914,
    27933, 27952, 27971, 27990, 28009, 28028, 28047, 28066,
    28085, 28104, 28123, 28142, 28161, 28180, 28199, 28218,
    28237, 28256, 28275, 28294, 28314, 28333, 28352, 28371,
    28390, 28410, 28429, 28448, 28467, 28487, 28506, 28525,
    28545, 28564, 28583, 28603, 28622, 28641, 28661, 28680,
    28699, 28719, 28738, 28758, 28777, 28797, 28816, 28836,
    28855, 28875, 28894, 28914, 28934, 28953, 28973, 28992,
    29012, 29032, 29051, 29071, 29091, 29110, 29130, 29150,
    29170, 29189, 29209, 29229, 29249, 29268, 29288, 29308,
    29328, 29348, 29368, 29388, 29407, 29427, 29447, 29467,
    29487, 29507, 29527, 29547, 29567, 29587, 29607, 29627,
    29647, 29667, 29687, 29708, 29728, 29748, 29768, 29788,
    29808, 29828, 29849, 29869, 29889, 29909, 29930, 29950,
    29970, 29990, 30011, 30031, 30051, 30072, 30092, 30112,
    30133, 30153, 30174, 30194, 30215, 30235, 30255, 30276,
    30296, 30317, 30338, 30358, 30379, 30399, 30420, 30440,
    30461, 30482, 30502, 30523, 30544, 30564, 30585, 30606,
    30626, 30647, 30668, 30689, 30709, 30730, 30751, 30772,
    30793, 30814, 30834, 30855, 30876, 30897, 30918, 30939,
    30960, 30981, 31002, 31023, 31044, 31065, 31086, 31107,
    31128, 31149, 31170, 31191, 31212, 31234, 31255, 31276,
    31297, 31318, 31339, 31361, 31382, 31403, 31424, 31446,
    31467, 31488, 31510, 31531, 31552, 31574, 31595, 31616,
    31638, 31659, 31681, 31702, 31724, 31745, 31767, 31788,
    31810, 31831, 31853, 31874, 31896, 31917, 31939, 31961,
    31982, 32004, 32026, 32047, 32069, 32091, 32112, 32134,
    32156, 32178, 32200, 32221, 32243, 32265, 32287, 32309,
    32331, 32352, 32374, 32396, 32418, 32440, 32462, 32484,
    32506, 32528, 32550, 32572, 32594, 32616, 32638, 32661,
    32683, 32705, 32727, 32749, 32771, 32793, 32816, 32838,
    32860, 32882, 32905, 32927, 32949, 32972, 32994, 33016,
    33039, 33061, 33083, 33106, 33128, 33151, 33173, 33195,
    33218, 33240, 33263, 33285, 33308, 33331, 33353, 33376,
    33398, 33421, 33444, 33466, 33489, 33512, 33534, 33557,
    33580, 33602, 33625, 33648, 33671, 33694, 33716, 33739,
    33762, 33785, 33808, 33831, 33854, 33876, 33899, 33922,
    33945, 33968, 33991, 34014, 34037, 34060, 34083, 34107,
    34130, 34153, 34176, 34199, 34222, 34245, 34269, 34292,
    34315, 34338, 34361, 34385, 34408, 34431, 34455, 34478,
    34501, 34525, 34548, 34571, 34595, 34618, 34642, 34665,
    34689, 34712, 34736, 34759, 34783, 34806, 34830, 34853,
    34877, 34901, 34924, 34948, 34972, 34995, 35019, 35043,
    35066, 35090, 35114, 35138, 35161, 35185, 35209, 35233,
    35257, 35281, 35305, 35328, 35352, 35376, 35400, 35424,
    35448, 35472, 35496, 35520, 35544, 35568, 35592, 35617,
    35641, 35665, 35689, 35713, 35737, 35762, 35786, 35810,
    35834, 35858, 35883, 35907, 35931, 35956, 35980, 36004,
    36029, 36053, 36078, 36102, 36126, 36151, 36175, 36200,
    36224, 36249, 36274, 36298, 36323, 36347, 36372, 36396,
    36421, 36446, 36470, 36495, 36520, 36545, 36569, 36594,
    36619, 36644, 36668, 36693, 36718, 36743, 36768, 36793,
    36818, 36843, 36868, 36893, 36918, 36943, 36968, 36993,
    37018, 37043, 37068, 37093, 37118, 37143, 37168, 37193,
    37219, 37244, 37269, 37294, 37320, 37345, 37370, 37395,
    37421, 37446, 37471, 37497, 37522, 37548, 37573, 37598,
    37624, 37649, 37675, 37700, 37726, 37751, 37777, 37803,
    37828, 37854, 37879, 37905, 37931, 37956, 37982, 38008,
    38034, 38059, 38085, 38111, 38137, 38163, 38188, 38214,
    38240, 38266, 38292, 38318, 38344, 38370, 38396, 38422,
    38448, 38474, 38500, 38526, 38552, 38578, 38604, 38630,
    38657, 38683, 38709, 38735, 38761, 38788, 38814, 38840,
    38866, 38893, 38919, 38945, 38972, 38998, 39025, 39051,
    39077, 39104, 39130, 39157, 39183, 39210, 39237, 39263,
    39290, 39316, 39343, 39370, 39396, 39423, 39450, 39476,
    39503, 39530, 39557, 39583, 39610, 39637, 39664, 39691,
    39718, 39744, 39771, 39798, 39825, 39852, 39879, 39906,
    39933, 39960, 39987, 40014, 40041, 40069, 40096, 40123,
    40150, 40177, 40204, 40232, 40259, 40286, 40313, 40341,
    40368, 40395, 40423, 40450, 40477, 40505, 40532, 40560,
    40587, 40615, 40642, 40670, 40697, 40725, 40752, 40780,
    40808, 40835, 40863, 40891, 40918, 40946, 40974, 41001,
    41029, 41057, 41085, 41113, 41140, 41168, 41196, 41224,
    41252, 41280, 41308, 41336, 41364, 41392, 41420, 41448,
    41476, 41504, 41532, 41560, 41588, 41617, 41645, 41673,
    41701, 41729, 41758, 41786, 41814, 41843, 41871, 41899,
    41928, 41956, 41984, 42013, 42041, 42070, 42098, 42127,
    42155, 42184, 42212, 42241, 42270, 42298, 42327, 42355,
    42384, 42413, 42442, 42470, 42499, 42528, 42557, 42585,
    42614, 42643, 42672, 42701, 42730, 42759, 42788, 42817,
    42846, 42875, 42904, 42933, 42962, 42991, 43020, 43049,
    43078, 43108, 43137, 43166, 43195, 43224, 43254, 43283,
    43312, 43342, 43371, 43400, 43430, 43459, 43489, 43518,
    43547, 43577, 43606, 43636, 43666, 43695, 43725, 43754,
    43784, 43814, 43843, 43873, 43903, 43932, 43962, 43992,
    44022, 44051, 44081, 44111, 44141, 44171, 44201, 44231,
    44261, 44291, 44321, 44351, 44381, 44411, 44441, 44471,
    44501, 44531, 44561, 44591, 44622, 44652, 44682, 44712,
    44743, 44773, 44803, 44834, 44864, 44894, 44925, 44955,
    44986, 45016, 45047, 45077, 45108, 45138, 45169, 45199,
    45230, 45260, 45291, 45322, 45352, 45383, 45414, 45445,
    45475, 45506, 45537, 45568, 45599, 45630, 45661, 45691
  };

/* note mapping table */
static unsigned short S_apu_vco_note_map[12] = 
  { 0, 85, 171, 256, 341, 427, 512, 597, 683, 768, 853, 939 };

/* sine wavetable (10 bit index, 1st quarter cycle stored) */
static unsigned short S_apu_vco_sine_table[256] = 
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

#define APU_OSC_LEVEL_MAX_INDEX  ((16 * 256) - 1) /* 4095 */

static unsigned short S_apu_vco_level_table[APU_OSC_LEVEL_TABLE_SIZE] = 
  { 32768, 32679, 32591, 32503, 32415, 32327, 32240, 32153,
    32066, 31979, 31893, 31806, 31720, 31635, 31549, 31464,
    31379, 31294, 31209, 31125, 31041, 30957, 30873, 30790,
    30706, 30623, 30541, 30458, 30376, 30293, 30212, 30130,
    30048, 29967, 29886, 29805, 29725, 29644, 29564, 29484,
    29405, 29325, 29246, 29167, 29088, 29009, 28931, 28852,
    28774, 28697, 28619, 28542, 28464, 28388, 28311, 28234,
    28158, 28082, 28006, 27930, 27855, 27779, 27704, 27629,
    27554, 27480, 27406, 27332, 27258, 27184, 27110, 27037,
    26964, 26891, 26818, 26746, 26674, 26601, 26530, 26458,
    26386, 26315, 26244, 26173, 26102, 26031, 25961, 25891,
    25821, 25751, 25681, 25612, 25543, 25474, 25405, 25336,
    25268, 25199, 25131, 25063, 24995, 24928, 24860, 24793,
    24726, 24659, 24593, 24526, 24460, 24394, 24328, 24262,
    24196, 24131, 24066, 24001, 23936, 23871, 23806, 23742,
    23678, 23614, 23550, 23486, 23423, 23359, 23296, 23233,
    23170, 23108, 23045, 22983, 22921, 22859, 22797, 22735,
    22674, 22613, 22552, 22491, 22430, 22369, 22309, 22248,
    22188, 22128, 22068, 22009, 21949, 21890, 21831, 21772,
    21713, 21654, 21595, 21537, 21479, 21421, 21363, 21305,
    21247, 21190, 21133, 21076, 21019, 20962, 20905, 20849,
    20792, 20736, 20680, 20624, 20568, 20513, 20457, 20402,
    20347, 20292, 20237, 20182, 20127, 20073, 20019, 19965,
    19911, 19857, 19803, 19750, 19696, 19643, 19590, 19537,
    19484, 19431, 19379, 19326, 19274, 19222, 19170, 19118,
    19066, 19015, 18963, 18912, 18861, 18810, 18759, 18708,
    18658, 18607, 18557, 18507, 18457, 18407, 18357, 18308,
    18258, 18209, 18160, 18110, 18061, 18013, 17964, 17915,
    17867, 17819, 17770, 17722, 17674, 17627, 17579, 17531,
    17484, 17437, 17390, 17343, 17296, 17249, 17202, 17156,
    17109, 17063, 17017, 16971, 16925, 16879, 16834, 16788,
    16743, 16697, 16652, 16607, 16562, 16518, 16473, 16428
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

/* highpass filters */
#define APU_HP_MULT_A0 32768
#define APU_HP_MULT_A1 -32650

#define APU_HP_MULT_B0 32709
#define APU_HP_MULT_B1 -32709

static short S_apu_hp_L_in[2];
static short S_apu_hp_L_out[2];

static short S_apu_hp_R_in[2];
static short S_apu_hp_R_out[2];

/* lowpass filters */
#define APU_LP_M 64

#define APU_LP_KERNEL_SIZE ((APU_LP_M / 2) + 1)
#define APU_LP_BUFFER_SIZE (APU_LP_M + 1)

static short S_apu_lp_kernel[APU_LP_KERNEL_SIZE] = 
  {    -3,   -28,    -9,    32,    28,   -32,   -56,    21,
       93,    14,  -128,   -81,   142,   178,  -113,  -295,
       17,   403,   161,  -462,  -424,   419,   757,  -214,
    -1129,  -232,  1494,  1072, -1803, -2819,  2009, 10211,
    14318
  };

static short S_apu_lp_L_in[APU_LP_BUFFER_SIZE];
static short S_apu_lp_R_in[APU_LP_BUFFER_SIZE];

static short S_apu_lp_buf_pos;

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
    APU_WAVE_REG(m, ENV_LEVEL)    = APU_ENV_LEVEL_MAX_INDEX;
    APU_WAVE_REG(m, ENV_MANTISSA) = 0;

    APU_WAVE_REG(m, OSC_INDEX)    = 0;
    APU_WAVE_REG(m, OSC_MANTISSA) = 0;
    APU_WAVE_REG(m, OSC_LEVEL)    = APU_OSC_LEVEL_MAX_INDEX;

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
    APU_PCM_REG(m, LEVEL) = APU_OSC_LEVEL_MAX_INDEX;
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
  for (m = 0; m < 2; m++)
  {
    S_apu_hp_L_in[m] = 0;
    S_apu_hp_L_out[m] = 0;

    S_apu_hp_R_in[m] = 0;
    S_apu_hp_R_out[m] = 0;
  }

  for (m = 0; m < APU_LP_BUFFER_SIZE; m++)
  {
    S_apu_lp_L_in[m] = 0;
    S_apu_lp_R_in[m] = 0;
  }

  S_apu_lp_buf_pos = 0;

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

  APU_WAVE_REG(inst_num, ENV_BLOCK)   = speed / APU_ENV_RATE_TABLE_SIZE;
  APU_WAVE_REG(inst_num, ENV_PATTERN) = speed % APU_ENV_RATE_TABLE_SIZE;

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

    /* update pattern step */
    step += 1;
    step &= 0x0F;

    /* determine increment for this step */
    if (step == 0)
      mask = 1;
    else
      mask = 1 << step;

    if (block <= APU_ENV_RATE_BASE_BLOCK)
    {
      if (S_apu_env_step_patterns[8 + pattern] & mask)
        increment = 1;
      else
        increment = 0;
    }
    else if (block == (APU_ENV_RATE_BASE_BLOCK + 1))
    {
      if (S_apu_env_step_patterns[2 * pattern] & mask)
        increment = 2;
      else
        increment = 1;
    }
    else if (block == (APU_ENV_RATE_BASE_BLOCK + 2))
    {
      if (S_apu_env_step_patterns[2 * pattern] & mask)
        increment = 4;
      else
        increment = 2;
    }
    else if (block == (APU_ENV_RATE_BASE_BLOCK + 3))
    {
      if (S_apu_env_step_patterns[2 * pattern] & mask)
        increment = 8;
      else
        increment = 4;
    }
    else
      increment = 8;

    /* update level */
    if (increment > 0)
    {
      if (stage == APU_ENV_STAGE_A)
      {
        if (increment == 8)
          increment = level >> 1;
        else if (increment == 4)
          increment = level >> 2;
        else if (increment == 2)
          increment = level >> 3;
        else
          increment = level >> 4;

        if (increment == 0)
          increment = 1;

        if (level > increment)
          level -= increment;
        else
          level = 0;

        if (level == 0)
          stage = APU_ENV_STAGE_D;
      }
      else if (stage == APU_ENV_STAGE_D)
      {
        level += increment;

        if (level > sl)
          stage = APU_ENV_STAGE_S;

        if (level > APU_ENV_LEVEL_MAX_INDEX)
          level = APU_ENV_LEVEL_MAX_INDEX;
      }
      else
      {
        level += increment;

        if (level > APU_ENV_LEVEL_MAX_INDEX)
          level = APU_ENV_LEVEL_MAX_INDEX;
      }
    }

#if 0
    if (m == 0)
      printf("Env Level: %d, Increment was: %d\n", level, increment);
#endif

    /* reset period countdown timer */
    if (stage == APU_ENV_STAGE_A)
      speed = S_apu_env_rise_time_map[ar];
    else if (stage == APU_ENV_STAGE_D)
      speed = S_apu_env_fall_time_map[dr];
    else if (stage == APU_ENV_STAGE_S)
      speed = S_apu_env_fall_time_map[sr];
    else
      speed = S_apu_env_fall_time_map[rr];

    block   = speed / APU_ENV_RATE_TABLE_SIZE;
    pattern = speed % APU_ENV_RATE_TABLE_SIZE;

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

  unsigned short patch_num;

  unsigned short block;
  unsigned short entry;
  unsigned short increment;
  unsigned short steps;

  int phase_index;
  int final_level_db;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    /* obtain patch number */
    patch_num = APU_WAVE_REG(m, PATCH_NO);

    /* lookup phase increment, apply vibrato */
    phase_index = (APU_WAVE_REG(m, NOTE) / 12) * APU_OSC_PITCH_TABLE_SIZE;
    phase_index += S_apu_vco_note_map[APU_WAVE_REG(m, NOTE) % 12];

    phase_index += APU_WAVE_REG(m, VIB_LEVEL);

    if (phase_index > APU_OSC_PITCH_MAX_INDEX)
      phase_index = APU_OSC_PITCH_MAX_INDEX;
    else if (phase_index < 0)
      phase_index = 0;

    block = phase_index / APU_OSC_PITCH_TABLE_SIZE;
    entry = phase_index % APU_OSC_PITCH_TABLE_SIZE;

    increment = S_apu_vco_pitch_table[entry];

    if (block < APU_OSC_PITCH_BASE_BLOCK)
      increment = increment >> (APU_OSC_PITCH_BASE_BLOCK - block);

    /* update phase (10.10 fixed point) */
    APU_WAVE_REG(m, OSC_MANTISSA) += increment;

    steps = (APU_WAVE_REG(m, OSC_MANTISSA) >> 10);

    if (block > APU_OSC_PITCH_BASE_BLOCK)
      steps = steps << (block - APU_OSC_PITCH_BASE_BLOCK);

    APU_WAVE_REG(m, OSC_INDEX) += steps;

    APU_WAVE_REG(m, OSC_MANTISSA) &= 0x03FF;
    APU_WAVE_REG(m, OSC_INDEX) &= 0x03FF;

    /* sine wavetable lookup */
    if (APU_WAVE_REG(m, OSC_INDEX) < 256)
      final_level_db = S_apu_vco_sine_table[APU_WAVE_REG(m, OSC_INDEX)];
    else if (APU_WAVE_REG(m, OSC_INDEX) < 512)
      final_level_db = S_apu_vco_sine_table[511 - APU_WAVE_REG(m, OSC_INDEX)];
    else if (APU_WAVE_REG(m, OSC_INDEX) < 768)
      final_level_db = S_apu_vco_sine_table[APU_WAVE_REG(m, OSC_INDEX) - 512];
    else
      final_level_db = S_apu_vco_sine_table[1023 - APU_WAVE_REG(m, OSC_INDEX)];

    /* apply envelope and tremolo */
    final_level_db += APU_WAVE_REG(m, ENV_LEVEL) << 2;
    final_level_db += APU_WAVE_REG(m, TREM_LEVEL);

    if (final_level_db > APU_OSC_LEVEL_MAX_INDEX)
      final_level_db = APU_OSC_LEVEL_MAX_INDEX;
    else if (final_level_db < 0)
      final_level_db = 0;

    /* set final level and sign */
    APU_WAVE_REG(m, OSC_LEVEL) = final_level_db & 0x0FFF;

    if (APU_WAVE_REG(m, OSC_INDEX) >= 512)
      APU_WAVE_REG(m, OSC_LEVEL) |= 0x1000;
  }

  return 0;
}

/******************************************************************************/
/* apu_advance_sample()                                                       */
/******************************************************************************/
int apu_advance_sample()
{
  int m;

  int samp_L;
  int samp_R;

  unsigned short block;
  unsigned short entry;

  unsigned int voice_level;

  /* compute current samples (left & right) */
  samp_L = 0;
  samp_R = 0;

  for (m = 0; m < APU_NUM_WAVE_VOICES; m++)
  {
    block = (APU_WAVE_REG(m, OSC_LEVEL) & 0x0FFF) / APU_OSC_LEVEL_TABLE_SIZE;
    entry = (APU_WAVE_REG(m, OSC_LEVEL) & 0x0FFF) % APU_OSC_LEVEL_TABLE_SIZE;

    voice_level = S_apu_vco_level_table[entry];

    if (block > 0)
      voice_level = voice_level >> block;

    if (APU_WAVE_REG(m, OSC_LEVEL) & 0x1000)
      samp_L -= voice_level;
    else
      samp_L += voice_level;

    if (APU_WAVE_REG(m, OSC_LEVEL) & 0x1000)
      samp_R -= voice_level;
    else
      samp_R += voice_level;
  }

  if (samp_L > 32767)
    samp_L = 32767;
  else if (samp_L < -32768)
    samp_L = -32768;

  if (samp_R > 32767)
    samp_R = 32767;
  else if (samp_R < -32768)
    samp_R = -32768;

  /* apply highpass filters (left & right) */
  S_apu_hp_L_in[1]  = S_apu_hp_L_in[0];
  S_apu_hp_L_out[1] = S_apu_hp_L_out[0];

  S_apu_hp_L_in[0] = samp_L;
  samp_L =  ((APU_HP_MULT_B0 * S_apu_hp_L_in[0]) / 32768) + 
            ((APU_HP_MULT_B1 * S_apu_hp_L_in[1]) / 32768) - 
            ((APU_HP_MULT_A1 * S_apu_hp_L_out[1]) / 32768);

  S_apu_hp_R_in[1]  = S_apu_hp_R_in[0];
  S_apu_hp_R_out[1] = S_apu_hp_R_out[0];

  S_apu_hp_R_in[0] = samp_R;
  samp_R =  ((APU_HP_MULT_B0 * S_apu_hp_R_in[0]) / 32768) + 
            ((APU_HP_MULT_B1 * S_apu_hp_R_in[1]) / 32768) - 
            ((APU_HP_MULT_A1 * S_apu_hp_R_out[1]) / 32768);

  if (samp_L > 32767)
    samp_L = 32767;
  else if (samp_L < -32768)
    samp_L = -32768;

  if (samp_R > 32767)
    samp_R = 32767;
  else if (samp_R < -32768)
    samp_R = -32768;

  S_apu_hp_L_out[0] = samp_L;
  S_apu_hp_R_out[0] = samp_R;

  /* update lowpass filter input buffers (left & right) */
  S_apu_lp_L_in[S_apu_lp_buf_pos] = S_apu_hp_L_out[0];
  S_apu_lp_R_in[S_apu_lp_buf_pos] = S_apu_hp_R_out[0];

  S_apu_lp_buf_pos = (S_apu_lp_buf_pos + 1) % APU_LP_BUFFER_SIZE;

  return 0;
}

/******************************************************************************/
/* apu_advance_output()                                                       */
/******************************************************************************/
int apu_advance_output()
{
  int m;

  int samp_L;
  int samp_R;

  int adj_pos;
  int inv_pos;

  short mult;

  /* apply lowpass filters (left & right) */
  adj_pos = (S_apu_lp_buf_pos + (APU_LP_M / 2)) % APU_LP_BUFFER_SIZE;

  mult = S_apu_lp_kernel[APU_LP_M / 2];

  samp_L = (mult * S_apu_lp_L_in[adj_pos]) / 32768;
  samp_R = (mult * S_apu_lp_R_in[adj_pos]) / 32768;

  for (m = 0; m < (APU_LP_M / 2); m++)
  {
    adj_pos = (S_apu_lp_buf_pos + m) % APU_LP_BUFFER_SIZE;
    inv_pos = (S_apu_lp_buf_pos + APU_LP_M - m) % APU_LP_BUFFER_SIZE;

    mult = S_apu_lp_kernel[m];

    samp_L += 
      (mult * (S_apu_lp_L_in[adj_pos] + S_apu_lp_L_in[inv_pos])) / 32768;

    samp_R += 
      (mult * (S_apu_lp_R_in[adj_pos] + S_apu_lp_R_in[inv_pos])) / 32768;
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

    apu_advance_sample();

    S_apu_timer += 1;

    if ((S_apu_timer % APU_TIMER_DIVIDER) == 0)
      S_apu_timer = 0;
  }

  apu_advance_output();

  return 0;
}

