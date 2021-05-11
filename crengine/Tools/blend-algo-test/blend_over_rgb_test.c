#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>

struct rgb_triplet {
    int16_t r;
    int16_t g;
    int16_t b;
};

typedef void (*foo_func)(uint32_t* dst, uint32_t color, uint8_t alpha_r, uint8_t alpha_g, uint8_t alpha_b);

// Reference implementation, floating point calculations, result rounded
void blend_ref_dbl_round(uint32_t* dst, uint32_t color, uint8_t alpha_r, uint8_t alpha_g, uint8_t alpha_b) {
    double alpha_r_ = ((double)alpha_r/255.0);
    double alpha_g_ = ((double)alpha_g/255.0);
    double alpha_b_ = ((double)alpha_b/255.0);
    int mask = 0xFFFFFF;
    color &= mask;

    double tr = ((double)(color>>16))/255.0;
    double tg = ((double)((color>>8) & 0x00FF))/255.0;
    double tb = ((double)(color & 0x00FF))/255.0;
    double dr = ((double)((*dst)>>16))/255.0;
    double dg = ((double)(((*dst)>>8) & 0x00FF))/255.0;
    double db = ((double)((*dst) & 0x00FF))/255.0;
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    double r = (alpha_r_*tr + (1.0 - alpha_r_)*dr);
    double g = (alpha_g_*tg + (1.0 - alpha_g_)*dg);
    double b = (alpha_b_*tb + (1.0 - alpha_b_)*db);
    *dst = ((uint32_t)round(r*255.0) << 16) | ((uint32_t)round(g*255.0) << 8) | (uint32_t)round(b*255.0);
}

// Reference implementation, floating point calculation, result truncated
void blend_ref_dbl_trunc(uint32_t* dst, uint32_t color, uint8_t alpha_r, uint8_t alpha_g, uint8_t alpha_b) {
    double alpha_r_ = ((double)alpha_r/255.0);
    double alpha_g_ = ((double)alpha_g/255.0);
    double alpha_b_ = ((double)alpha_b/255.0);
    uint32_t mask = 0xFFFFFF;
    color &= mask;

    double tr = ((double)(color>>16))/255.0;
    double tg = ((double)((color>>8) & 0x00FF))/255.0;
    double tb = ((double)(color & 0x00FF))/255.0;
    double dr = ((double)((*dst)>>16))/255.0;
    double dg = ((double)(((*dst)>>8) & 0x00FF))/255.0;
    double db = ((double)((*dst) & 0x00FF))/255.0;
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    double r = (alpha_r_*tr + (1.0 - alpha_r_)*dr);
    double g = (alpha_g_*tg + (1.0 - alpha_g_)*dg);
    double b = (alpha_b_*tb + (1.0 - alpha_b_)*db);
    *dst = ((uint32_t)(r*255.0) << 16) | ((uint32_t)(g*255.0) << 8) | (uint32_t)(b*255.0);
}

// Old CoolReader implementation, only gray (up to version 3.2.57 inclusive)
void old_impl_gray(uint32_t* dst, uint32_t color, uint8_t alpha_r, uint8_t alpha_g, uint8_t alpha_b) {
    uint32_t opaque = (alpha_r >> 1) & 0x7F;
    if (opaque >= 0x78)
        *dst = color;
    else if (opaque > 0) {
        uint32_t alpha_ = 0x7F - opaque;
        uint32_t cl1 = ((alpha_*((*dst)&0xFF00FF) + opaque*(color&0xFF00FF))>>7) & 0xFF00FF;
        uint32_t cl2 = ((alpha_*((*dst)&0x00FF00) + opaque*(color&0x00FF00))>>7) & 0x00FF00;
        *dst = cl1 | cl2;
    }
}

// integer implementation, result rounded
void blend_int_round(uint32_t* dst, uint32_t color, uint8_t alpha_r, uint8_t alpha_g, uint8_t alpha_b) {
    uint32_t tr = (color>>16);
    uint32_t tg = ((color>>8) & 0x00FF);
    uint32_t tb = (color & 0x00FF);
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    uint32_t r = (alpha_r*tr + (255 - alpha_r)*((*dst)>>16) + 127)/255;
    uint32_t g = (alpha_g*tg + (255 - alpha_g)*(((*dst)>>8) & 0x00FF) + 127)/255;
    uint32_t b = (alpha_b*tb + (255 - alpha_b)*((*dst) & 0x00FF) + 127)/255;
    *dst = (r << 16) | (g << 8) | b;
}

// integer implementation, result truncated
void blend_int_trunc(uint32_t* dst, uint32_t color, uint8_t alpha_r, uint8_t alpha_g, uint8_t alpha_b) {
    uint32_t tr = (color>>16);
    uint32_t tg = ((color>>8) & 0x00FF);
    uint32_t tb = (color & 0x00FF);
    // blending function (OVER operator)
    // See https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#ft_render_glyph
    uint32_t r = (alpha_r*tr + (255 - alpha_r)*((*dst)>>16))/255;
    uint32_t g = (alpha_g*tg + (255 - alpha_g)*(((*dst)>>8) & 0x00FF))/255;
    uint32_t b = (alpha_b*tb + (255 - alpha_b)*((*dst) & 0x00FF))/255;
    *dst = (r << 16) | (g << 8) | b;
}


#define FUNCS_COUNT		5
foo_func funcs[FUNCS_COUNT] = {
    blend_ref_dbl_round,
    blend_ref_dbl_trunc,
    old_impl_gray,
    blend_int_round,
    blend_int_trunc
};

const char* funcs_name[FUNCS_COUNT] = {
    "blend_ref_dbl_round",
    "blend_ref_dbl_trunc",
    "old_impl_gray",
    "blend_int_round",
    "blend_int_trunc"
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

void calc_rgb_diff(struct rgb_triplet* diff, uint32_t color1, uint32_t color2);
int rgb_diff_to_weight(const struct rgb_triplet* rgb);

int main() {
    //int mode = TEST_MODE_PRECISION;
    int mode = TEST_MODE_PERFORMANCE;

    const uint32_t color_step = 31;

    int i;
    uint32_t dst_r, dst_g, dst_b;
    uint32_t dst;
    uint32_t color_r, color_g, color_b;
    uint32_t color;
    uint32_t alpha;
    uint32_t dst_res[FUNCS_COUNT];
    int diff_1_vs[FUNCS_COUNT-1];		// difference between func1 and other
    int min_diff_1_vs[FUNCS_COUNT-1];
    int max_diff_1_vs[FUNCS_COUNT-1];
    int diff_1_vs_count[FUNCS_COUNT-1];
    struct rgb_triplet rgb_diff;
    if (TEST_MODE_PRECISION == mode) {
        // initial values for diff statistics
        for (i = 0; i < FUNCS_COUNT-1; i++) {
            diff_1_vs_count[i] = 0;
            min_diff_1_vs[i] = 65535;
            max_diff_1_vs[i] = -65535;
        }
        // calculations
        for (dst_r = 0; dst_r <= 255; dst_r += color_step) {
            for (dst_g = 0; dst_g <= 255; dst_g += color_step) {
                for (dst_b = 0; dst_b <= 255; dst_b += color_step) {
                    dst = (dst_r << 16) | (dst_g << 8) | dst_b;
                    for (color_r = 0; color_r <= 255; color_r += color_step) {
                        for (color_g = 0; color_g <= 255; color_g += color_step) {
                            for (color_b = 0; color_b <= 255; color_b += color_step) {
                                color = (color_r << 16) | (color_g << 8) | color_b;
                                for (alpha = 0; alpha <= 255; alpha++) {
                                    printf("dst=#%06X, color=#%06X, alpha=0x%02X => ", dst, color, alpha);
                                    for (i = 0; i < FUNCS_COUNT; i++) {
                                        dst_res[i] = dst;
                                        funcs[i](&dst_res[i], color, alpha, alpha, alpha);
                                        printf("dst%d=#%06X", i+1, dst_res[i]);
                                        if (i != FUNCS_COUNT - 1)
                                            printf(", ");
                                        else
                                            printf(": ");
                                    }
                                    for (i = 0; i < FUNCS_COUNT-1; i++) {
                                        calc_rgb_diff(&rgb_diff, dst_res[0], dst_res[i + 1]);
                                        diff_1_vs[i] = rgb_diff_to_weight(&rgb_diff);
                                        if (diff_1_vs[i] < min_diff_1_vs[i])
                                            min_diff_1_vs[i] = diff_1_vs[i];
                                        if (diff_1_vs[i] > max_diff_1_vs[i])
                                            max_diff_1_vs[i] = diff_1_vs[i];
                                        if (diff_1_vs[i] != 0) {
                                            printf(" diff1%d=%02d", i+2, diff_1_vs[i]);
                                            diff_1_vs_count[i]++;
                                        }
                                    }
                                    printf("\n");
                                }
                            }
                        }
                    }
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
        srandom(time(0));
        uint32_t* test_inp_data = (uint32_t*)malloc(TEST_POINTS_COUNT*sizeof(int));
        uint32_t* test_color_data = (uint32_t*)malloc(TEST_POINTS_COUNT*sizeof(int));
        int* test_alpha_r_data = (int*)malloc(TEST_POINTS_COUNT*sizeof(int));
        int* test_alpha_g_data = (int*)malloc(TEST_POINTS_COUNT*sizeof(int));
        int* test_alpha_b_data = (int*)malloc(TEST_POINTS_COUNT*sizeof(int));
        if (!test_inp_data || !test_color_data || !test_alpha_r_data || !test_alpha_g_data || !test_alpha_b_data) {
            fprintf(stderr, "malloc failure!\n");
            return 1;
        }
        long j;
        long r, g, b;
        for (j = 0; j < TEST_POINTS_COUNT; j++) {
            r = 300L*random()/RAND_MAX;
            if (r > 255)
                r = 255;
            g = 300L*random()/RAND_MAX;
            if (g > 255)
                g = 255;
            b = 300L*random()/RAND_MAX;
            if (b > 255)
                b = 255;
            test_inp_data[j] = (uint32_t)((r << 16) | (g << 8) | b);

            test_alpha_r_data[j] = (int)(300L*random()/RAND_MAX);
            if (test_alpha_r_data[j] > 255)
                test_alpha_r_data[j] = 255;
            test_alpha_g_data[j] = (int)(300L*random()/RAND_MAX);
            if (test_alpha_g_data[j] > 255)
                test_alpha_g_data[j] = 255;
            test_alpha_b_data[j] = (int)(300L*random()/RAND_MAX);
            if (test_alpha_b_data[j] > 255)
                test_alpha_b_data[j] = 255;

            r = 300L*random()/RAND_MAX;
            if (r > 255)
                r = 255;
            g = 300L*random()/RAND_MAX;
            if (g > 255)
                g = 255;
            b = 300L*random()/RAND_MAX;
            if (b > 255)
                b = 255;
            test_color_data[j] = (uint32_t)((r << 16) | (g << 8) | b);
        }
        printf("done (dataset size is %ld).\n", TEST_POINTS_COUNT);

        for (i = 0; i < FUNCS_COUNT; i++) {
            struct timeval start_time;
            struct timeval end_time;
            uint32_t volatile value = 0xFFFFFF;
            uint32_t dst_res;
            printf("bench function #%d /%s/... ", i, funcs_name[i]);
            gettimeofday(&start_time, NULL);
            for (j = 0; j < TEST_POINTS_COUNT; j++) {
                dst_res = (uint32_t)test_inp_data[j];
                funcs[i](&dst_res, test_color_data[j], test_alpha_r_data[j], test_alpha_g_data[j], test_alpha_b_data[j]);
                value ^= dst_res;   // so that when optimizing the compiler does not remove the function call
            }
            gettimeofday(&end_time, NULL);
            int64_t elapsed = timevalcmp(&end_time, &start_time);
            printf(" done, elapsed=%d ms (value=%d)\n", (int)(elapsed/1000), value);
        }
        free(test_inp_data);
        free(test_alpha_r_data);
        free(test_alpha_g_data);
        free(test_alpha_b_data);
        free(test_color_data);
    }
    return 0;
}

int64_t timevalcmp(const struct timeval* t1, const struct timeval* t2) {
	if (!t1 || !t2)
		return 0;
	// check for overflow exclusion
	if (t1->tv_sec - t2->tv_sec > 2000000L)
		return LONG_MAX;
	if (t2->tv_sec - t1->tv_sec > 2000000L)
		return LONG_MIN;
	return (t1->tv_sec - t2->tv_sec)*1000000 + (t1->tv_usec - t2->tv_usec);
}

void calc_rgb_diff(struct rgb_triplet* diff, uint32_t color1, uint32_t color2) {
    uint8_t c1_r = color1 >> 16;
    uint8_t c1_g = (color1 >> 8) & 0xFF;
    uint8_t c1_b = color1 & 0xFF;
    uint8_t c2_r = color2 >> 16;
    uint8_t c2_g = (color2 >> 8) & 0xFF;
    uint8_t c2_b = color2 & 0xFF;
    diff->r = (int16_t)c1_r - (int16_t)c2_r;
    diff->g = (int16_t)c1_g - (int16_t)c2_g;
    diff->b = (int16_t)c1_b - (int16_t)c2_b;
}

int rgb_diff_to_weight(const struct rgb_triplet* rgb) {
    return abs(rgb->r) + abs(rgb->g) + abs(rgb->b);
}
