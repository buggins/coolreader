
#include "lvfontglyphcache_a.h"
#include "lvfontglyphcache_b.h"
#include "lvtypes.h"

#include <stdio.h>
#include <sys/time.h>
#include <limits.h>

// collected from real book with cyrillic text, order of characters is saved.
// captured glyph cache filling
lChar16 glyphCodes_tofill[] = {
    52,
    54,
    56,
    32,
    47,
    49,
    58,
    48,
    1040,
    1083,
    1072,
    1089,
    1090,
    1077,
    1088,
    1056,
    1081,
    1085,
    1086,
    1100,
    1076,
    46,
    1047,
    1074,
    1079,
    1099,
    8211,
    160,
    1041,
    44,
    1057,
    1087,
    1080,
    1082,
    1103,
    1091,
    1061,
    1084,
    1045,
    1095,
    1073,
    1078,
    1094,
    1093,
    1097,
    1075,
    1096,
    1051,
    1102,
    171,
    187,
    1098,
    1055,
    1101,
    1053,
    1048,
    63,
    1052,
    1050,
    1092,
    1042,
    1054,
    1044,
    1071,
    1058,
    57,
    173,
    33,
    45,
    55,
    1059,
    1060,
    1043,
    50,
    51,
    1069,
    42,
    8230,
    1064,
    53,
    1063,
    1046,
    1062,
    1065,
    91,
    93,
    105,
    103,
    1105,
    59,
    769,
    40,
    41,
    66,
    117,
    121,
    110,
    84,
    109,
    101,
    83,
    115,
    112,
    100,
    65,
    97,
    116,
    111,
    99,
    102,
    114,
    82,
    104,
    89,
    87,
    1049,
    108,
    80,
    85,
    72,
    73,
    78,
    71,
    67,
    69,
    169,
    98,
    118,
    8222,
    8220,
    174,
    107,
    122,
    76,
    70,
    43,
    1070,
    79,
    68,
    8212,
};

// collected from real book with cyrillic text, order of characters is saved.
// captured glyph cache lookup
lChar16 lookup_seq[] = {
    50,
    32,
    47,
    32,
    54,
    49,
    52,
    49,
    48,
    58,
    50,
    48,
    1040,
    1083,
    1072,
    1089,
    1090,
    1077,
    1088,
    32,
    32,
    1056,
    1077,
    1081,
    1085,
    1086,
    1083,
    1100,
    1076,
    1089,
    46,
    32,
    32,
    1047,
    1074,
    1077,
    1079,
    1076,
    1085,
    1099,
    1081,
    32,
    1083,
    1077,
    1076,
    1042,
    32,
    50,
    48,
    53,
    55,
    32,
    1075,
    1086,
    1076,
    1091,
    32,
    1089,
    1087,
    1091,
    1090,
    1085,
    1080,
    1082,
    32,
    1057,
    1072,
    1090,
    1091,
    1088,
    1085,
    1072,
    32,
    1071,
    1085,
    1091,
    1089,
    32,
    1074,
    1085,
    1077,
    1079,
    1072,
    1087,
    1085,
    1086,
    32,
    1089,
    1086,
    1096,
    1077,
    1083,
    32,
    1089,
    32,
    1086,
    1088,
    1073,
    1080,
    1090,
};

int64_t timevalcmp(const struct timeval* t1, const struct timeval* t2);

int main(int /*argc*/, char* /*argv*/[])
{
    const int glyphCodes_tofill_sz = sizeof(glyphCodes_tofill)/sizeof(lChar16);
    const int lookup_seq_sz = sizeof(lookup_seq)/sizeof(lChar16);
    const int bench_sz = 100000;
    
    LVFontGlobalGlyphCacheA globalCacheA(0x40000);
    LVFontLocalGlyphCacheA localCacheA(&globalCacheA);
    LVFontGlyphCacheItemA* itemA;
    LVFontGlobalGlyphCacheB globalCacheB(0x40000);
    LVFontLocalGlyphCacheB localCacheB(&globalCacheB, 256);
    LVFontGlyphCacheItemB* itemB;
    u_int64_t tmp;
    struct timeval ts1;
    struct timeval ts2;
    int64_t elapsed;
    int i, j;

    printf("size of cache data: %u\n", glyphCodes_tofill_sz);

    // fill cache
    for (i = 0; i < glyphCodes_tofill_sz; i++)
    {
        itemA = LVFontGlyphCacheItemA::newItem(&localCacheA, glyphCodes_tofill[i], 10, 10);
        if (itemA)
        {
            itemA->origin_x = 0;
            itemA->origin_y = 0;
            localCacheA.put(itemA);
        }
        itemB = LVFontGlyphCacheItemB::newItem(&localCacheB, glyphCodes_tofill[i], 10, 10);
        if (itemB)
        {
            itemB->origin_x = 0;
            itemB->origin_y = 0;
            localCacheB.put(itemB);
        }
    }

    printf("size of global cacheA: %u\n", globalCacheA.getSize());
    printf("size of global cacheB: %u\n", globalCacheB.getSize());

    // bench lookup based on linked list
    printf("bench cache based on linked list...\n");
    gettimeofday(&ts1, NULL);
    tmp = 0;
    for (j = 0; j < bench_sz; j++)
    {
        for (i = 0; i < lookup_seq_sz; i++)
        {
            itemA = localCacheA.getByChar(lookup_seq[i]);
            tmp += itemA->origin_x;
        }
    }
    gettimeofday(&ts2, NULL);
    elapsed = timevalcmp(&ts2, &ts1);
    printf("%lld us\n", elapsed);
    printf("t = %lld\n", tmp);

    // bench lookup based on hash table
    printf("bench cache based on hash table...\n");
    gettimeofday(&ts1, NULL);
    for (j = 0; j < bench_sz; j++)
    {
        for (i = 0; i < lookup_seq_sz; i++)
        {
            itemB = localCacheB.getByChar(lookup_seq[i]);
            tmp += itemB->origin_x;
        }
    }
    gettimeofday(&ts2, NULL);
    elapsed = timevalcmp(&ts2, &ts1);
    printf("%lld us\n", elapsed);
    printf("t = %lld\n", tmp);

    // cleanup
    globalCacheA.clear();
    globalCacheB.clear();
    return 0;
}

int64_t timevalcmp(const struct timeval* t1, const struct timeval* t2)
{
	if (!t1 || !t2)
		return 0;
	// check for overflow exclusion
	if (t1->tv_sec - t2->tv_sec > 2000000L)
		return LONG_MAX;
	if (t2->tv_sec - t1->tv_sec > 2000000L)
		return LONG_MIN;
	return (t1->tv_sec - t2->tv_sec)*1000000 + (t1->tv_usec - t2->tv_usec);
}
