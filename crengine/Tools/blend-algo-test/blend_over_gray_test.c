#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>

typedef void (*foo_func)(uint8_t* dst, uint8_t color, uint8_t alpha);

// Reference implementation, floating point calculations, result rounded
void blend_ref_dbl_round(uint8_t* dst, uint8_t color, uint8_t alpha) {
    double alpha_ = ((double)alpha/255.0);
    uint32_t mask = 0xFF;
    color &= mask;
    double color_ = ((double)color/255.0);
    double dst_ = ((double)*dst/255.0);
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    dst_ = alpha_*color_ + (1.0 - alpha_)*dst_;
    *dst = (uint8_t)round(dst_*255.0);
}

// Reference implementation, floating point calculation, result truncated
void blend_ref_dbl_trunc(uint8_t* dst, uint8_t color, uint8_t alpha) {
    double alpha_ = ((double)alpha/255.0);
    uint32_t mask = 0xFF;
    color &= mask;
    double color_ = ((double)color/255.0);
    double dst_ = ((double)*dst/255.0);
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    dst_ = alpha_*color_ + (1.0 - alpha_)*dst_;
    *dst = (uint8_t)(dst_*255.0);
}

// Old CoolReader implementation (up to version 3.2.57 inclusive)
void old_impl(uint8_t* dst, uint8_t color, uint8_t alpha) {
    if (alpha >= 255)
        *dst = color;
    else {
        int alpha_ = alpha ^ 0xFF;
        if ( alpha_==0 ) {
            *dst = color;
        } else if ( alpha_ < 255 ) {
            int mask = 0xFF;
            color &= mask;
            uint32_t opaque = alpha_ ^ 0xFF;
            uint32_t n1 = (((*dst) * alpha_ + color * opaque) >> 8);// & mask;
            *dst = (uint8_t)n1;
        }
    }
}

// integer implementation, result rounded
void blend_int_round(uint8_t* dst, uint8_t color, uint8_t alpha) {
    uint32_t alpha_ = (uint32_t)alpha;
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    *dst = ( (alpha_*color + (255 - alpha_)*(*dst) + 127)/255 );
}

// integer implementation, result truncated
void blend_int_trunc(uint8_t* dst, uint8_t color, uint8_t alpha) {
    uint32_t alpha_ = (uint32_t)alpha;
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    *dst = ( (alpha_*color + (255 - alpha_)*(*dst) )/255);
}

void blend_light_test(uint8_t* dst, uint8_t color, uint8_t alpha) {
    uint32_t alpha_ = (uint32_t)alpha;
    uint32_t mask = 0xFF;
    color &= mask;
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    *dst = ((alpha_*color + (255 - alpha_)*(*dst)) >> 8);
}

#define FUNCS_COUNT		6
foo_func funcs[FUNCS_COUNT] = {
    blend_ref_dbl_round,
    blend_ref_dbl_trunc,
    old_impl,
    blend_int_round,
    blend_int_trunc,
    blend_light_test
};

const char* funcs_name[FUNCS_COUNT] = {
    "blend_ref_dbl_round",
    "blend_ref_dbl_trunc",
    "old_impl",
    "blend_int_round",
    "blend_int_trunc",
    "blend_light_test"
};

#define TEST_MODE_PRECISION     1
#define TEST_MODE_PERFORMANCE   2

#define TEST_POINTS_COUNT   100000000L

/** @brief Calculates the difference between t1 and t2.
  * @param t1 pointer to the first time value from which to subtract the second time.
  * @param t2 pointer to the second time value to subtract from the first.
  * @return Difference between t1 and t2 in us.
  */
int64_t timevalcmp(const struct timeval* t1, const struct timeval* t2);

int main() {
    //int mode = TEST_MODE_PRECISION;
    int mode = TEST_MODE_PERFORMANCE;

    int i;
    unsigned int dst;
    unsigned int color;
    unsigned int alpha;
    uint8_t dst_res[FUNCS_COUNT];
    int diff_1_vs[FUNCS_COUNT-1];		// difference between func1 and other
    int min_diff_1_vs[FUNCS_COUNT-1];
    int max_diff_1_vs[FUNCS_COUNT-1];
    int diff_1_vs_count[FUNCS_COUNT-1];
    if (TEST_MODE_PRECISION == mode) {
        // initial values for diff statistics
        for (i = 0; i < FUNCS_COUNT-1; i++) {
            diff_1_vs_count[i] = 0;
            min_diff_1_vs[i] = 65535;
            max_diff_1_vs[i] = -65535;
        }
        // calculations
        for (dst = 0; dst <= 255; dst++) {
            for (color = 0; color <= 255; color++) {
                for (alpha = 0; alpha <= 255; alpha++) {
                    printf("dst=%03d, color=%03d, alpha=%03d => ", dst, color, alpha);
                    for (i = 0; i < FUNCS_COUNT; i++) {
                        dst_res[i] = dst;
                        funcs[i](&dst_res[i], color, alpha);
                        printf("dst%d=%d", i+1, dst_res[i]);
                        if (i != FUNCS_COUNT - 1)
                            printf(", ");
                        else
                            printf(": ");
                    }
                    for (i = 0; i < FUNCS_COUNT-1; i++) {
                        diff_1_vs[i] = dst_res[0] - dst_res[i + 1];
                        if (diff_1_vs[i] < min_diff_1_vs[i])
                            min_diff_1_vs[i] = diff_1_vs[i];
                        if (diff_1_vs[i] > max_diff_1_vs[i])
                            max_diff_1_vs[i] = diff_1_vs[i];
                        if (diff_1_vs[i] != 0) {
                            printf(" diff1%d=%d", i+2, diff_1_vs[i]);
                            diff_1_vs_count[i]++;
                        }
                    }
                    printf("\n");
                }
            }
        }
        for (i = 0; i < FUNCS_COUNT - 1; i++) {
            printf("min_diff1%d=%d, max_diff1%d=%d\n", i+2, min_diff_1_vs[i], i+2, max_diff_1_vs[i]);
            printf("Total diff1%d count: %d\n", i+2, diff_1_vs_count[i]);
        }
    } else if (TEST_MODE_PERFORMANCE == mode) {
        // build input test file
        printf("Preparing data (random)... ");
        fflush(stdout);
#ifdef _WIN32
        srand(time(0));
#else
        srandom(time(0));
#endif
        int* test_inp_data = (int*)malloc(TEST_POINTS_COUNT*sizeof(int));
        int* test_alpha_data = (int*)malloc(TEST_POINTS_COUNT*sizeof(int));
        int* test_color_data = (int*)malloc(TEST_POINTS_COUNT*sizeof(int));
        if (!test_inp_data || !test_alpha_data || !test_color_data) {
            fprintf(stderr, "malloc failure!\n");
            return 1;
        }
        long j;
        for (j = 0; j < TEST_POINTS_COUNT; j++) {
#ifdef _WIN32
            test_inp_data[j] = (int)(300L*rand()/RAND_MAX);
            test_alpha_data[j] = (int)(300L*rand()/RAND_MAX);
            test_color_data[j] = (int)(300L*rand()/RAND_MAX);
#else
            test_inp_data[j] = (int)(300L*random()/RAND_MAX);
            test_alpha_data[j] = (int)(300L*random()/RAND_MAX);
            test_color_data[j] = (int)(300L*random()/RAND_MAX);
#endif
            if (test_inp_data[j] > 255)
                test_inp_data[j] = 255;
            if (test_alpha_data[j] > 255)
                test_alpha_data[j] = 255;
            if (test_color_data[j] > 255)
                test_color_data[j] = 255;
        }
        printf("done (dataset size is %ld).\n", TEST_POINTS_COUNT);

        for (i = 0; i < FUNCS_COUNT; i++) {
            struct timeval start_time;
            struct timeval end_time;
            uint8_t volatile value = 0xFF;
            uint8_t dst_res;
            printf("bench function #%d /%s/... ", i, funcs_name[i]);
            gettimeofday(&start_time, NULL);
            for (j = 0; j < TEST_POINTS_COUNT; j++) {
                dst_res = (uint8_t)test_inp_data[j];
                funcs[i](&dst_res, test_color_data[j], test_alpha_data[j]);
                value ^= dst_res;   // so that when optimizing the compiler does not remove the function call
            }
            gettimeofday(&end_time, NULL);
            int64_t elapsed = timevalcmp(&end_time, &start_time);
            printf(" done, elapsed=%d ms (value=%d)\n", (int)(elapsed/1000), value);
        }
        free(test_inp_data);
        free(test_alpha_data);
        free(test_color_data);
    }
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
