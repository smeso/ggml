#include "ggml.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct ggml_context* make_ctx(void) {
    struct ggml_init_params params = {
        .mem_size = 2 * 1024 * 1024,
    };

    return ggml_init(params);
}

void printf_tensor(struct ggml_tensor * t) {
    if (t->type == GGML_TYPE_F32) {
        const float * t_d = ggml_get_data_f32(t);
        for (int i = 0; i < t->ne[2]; ++i) {
            for (int j = 0; j < t->ne[1]; ++j) {
                for (int k = 0; k < t->ne[0]; ++k) {
                    printf("%.1f ", t_d[i * t->ne[1] * t->ne[0] + j * t->ne[0] + k]);
                }
                printf("\n");
            }
            printf("---\n");
        }
    }
    else if (t->type == GGML_TYPE_F16) {
        const ggml_fp16_t * t_d = ggml_get_data(t);
        for (int i = 0; i < t->ne[2]; ++i) {
            for (int j = 0; j < t->ne[1]; ++j) {
                for (int k = 0; k < t->ne[0]; ++k) {
                    printf("%.1f ", ggml_fp16_to_fp32(t_d[i * t->ne[1] * t->ne[0] + j * t->ne[0] + k]));
                }
                printf("\n");
            }
            printf("---\n");
        }
    }
    else {
        printf("unknown type\n");
    }
}

void check_tensor(struct ggml_tensor * t, float * expected_t_d, int ne0, int ne1, int ne2) {
    GGML_ASSERT(t->type == GGML_TYPE_F32);
    GGML_ASSERT(t->ne[0] == ne0);
    GGML_ASSERT(t->ne[1] == ne1);
    GGML_ASSERT(t->ne[2] == ne2);
    for (int i2 = 0; i2 < ne2; ++i2) {
        for (int i1 = 0; i1 < ne1; ++i1) {
            for (int i0 = 0; i0 < ne0; ++i0) {
                float expected = *(expected_t_d + i2 * ne1 * ne0 + i1 * ne0 + i0);
                float actual = ggml_get_data_f32(t)[i2 * ne1 * ne0 + i1 * ne0 + i0];
                if (expected != actual) {
                    printf("expected %.1f, got %.1f\n", expected, actual);
                }
                GGML_ASSERT(expected == actual);
            }
        }
    }
}

void test_conv_transpose_1d(void) {

    float buf_f32[1024];
    for (int i = 0; i < 1024; ++i) {
        buf_f32[i] = (float)i;
    }

    ggml_fp16_t buf_f16[1024];
    for (int i = 0; i < 1024; ++i) {
        buf_f16[i] = ggml_fp32_to_fp16((float)i);
    }

    float expected_out_1[3][4] = {
        {18.0, 45.0, 59.0, 37.0},
        {24.0, 61.0, 83.0, 51.0},
        {30.0, 77.0, 107.0, 65.0},
    };
    float expected_out_2[3][6] = {
        {18.0, 21.0, 24.0, 29.0, 30.0, 37.0},
        {24.0, 27.0, 34.0, 39.0, 44.0, 51.0},
        {30.0, 33.0, 44.0, 49.0, 58.0, 65.0},
    };
    float expected_out_3[3][8] = {
        {18.0, 21.0, 0.0, 24.0, 29.0, 0.0, 30.0, 37.0},
        {24.0, 27.0, 0.0, 34.0, 39.0, 0.0, 44.0, 51.0},
        {30.0, 33.0, 0.0, 44.0, 49.0, 0.0, 58.0, 65.0},
    };
    float expected_out_4[] = { 18., 24., 51., 29., 37., 24., 34., 71.,
                               39., 51., 30., 44., 91., 49., 65. };
    float expected_out_5[] = { 18., 24., 30., 21., 29., 37., 24., 34., 44.,
                               27., 39., 51., 30., 44., 58., 33., 49., 65. };
    float expected_out_6[] = { 45., 59., 61., 83., 77., 107. };
    float expected_out_7[] = { 0., 0., 0. };

    // conv transpose 1d
    {
        struct ggml_context * ctx = make_ctx();

        struct ggml_tensor * t = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 3, 2); // l x cin
        memcpy(t->data, buf_f32, ggml_nbytes(t));

        struct ggml_tensor * k = ggml_new_tensor_3d(ctx, GGML_TYPE_F16, 2, 3, 2); // k x cout x cin
        memcpy(k->data, buf_f16, ggml_nbytes(k));

        struct ggml_tensor * out_1 = ggml_conv_transpose_1d(ctx, k, t, 1 /* s0 */, 0 /* p0 */, 1 /* d0 */);
        struct ggml_tensor * out_2 = ggml_conv_transpose_1d(ctx, k, t, 2 /* s0 */, 0 /* p0 */, 1 /* d0 */);
        struct ggml_tensor * out_3 = ggml_conv_transpose_1d(ctx, k, t, 3 /* s0 */, 0 /* p0 */, 1 /* d0 */);

        struct ggml_tensor * out_4 = ggml_conv_transpose_1d(ctx, k, t, 1 /* s0 */, 0 /* p0 */, 2 /* d0 */);
        struct ggml_tensor * out_5 = ggml_conv_transpose_1d(ctx, k, t, 1 /* s0 */, 0 /* p0 */, 3 /* d0 */);

        struct ggml_tensor * out_6 = ggml_conv_transpose_1d(ctx, k, t, 1 /* s0 */, 1 /* p0 */, 1 /* d0 */);

        struct ggml_tensor * out_7 = ggml_conv_transpose_1d(ctx, k, t, 2 /* s0 */, 3 /* p0 */, 2 /* d0 */);

        struct ggml_cgraph * gf_1 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_2 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_3 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_4 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_5 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_6 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_7 = ggml_new_graph(ctx);

        ggml_build_forward_expand(gf_1, out_1);
        ggml_build_forward_expand(gf_2, out_2);
        ggml_build_forward_expand(gf_3, out_3);
        ggml_build_forward_expand(gf_4, out_4);
        ggml_build_forward_expand(gf_5, out_5);
        ggml_build_forward_expand(gf_6, out_6);
        ggml_build_forward_expand(gf_7, out_7);

        ggml_graph_compute_with_ctx(ctx, gf_1, 1);
        ggml_graph_compute_with_ctx(ctx, gf_2, 1);
        ggml_graph_compute_with_ctx(ctx, gf_3, 1);
        ggml_graph_compute_with_ctx(ctx, gf_4, 1);
        ggml_graph_compute_with_ctx(ctx, gf_5, 1);
        ggml_graph_compute_with_ctx(ctx, gf_6, 1);
        ggml_graph_compute_with_ctx(ctx, gf_7, 1);

        check_tensor(out_1, (float*)expected_out_1, 4, 3, 1);
        check_tensor(out_2, (float*)expected_out_2, 6, 3, 1);
        check_tensor(out_3, (float*)expected_out_3, 8, 3, 1);
        check_tensor(out_4, (float*)expected_out_4, 5, 3, 1);
        check_tensor(out_5, (float*)expected_out_5, 6, 3, 1);
        check_tensor(out_6, (float*)expected_out_6, 2, 3, 1);
        check_tensor(out_7, (float*)expected_out_7, 1, 3, 1);
    }
}

void test_conv_transpose_1d_batched(void) {

    float buf_f32[1024];
    for (int i = 0; i < 1024; ++i) {
        buf_f32[i] = (float)i;
    }

    ggml_fp16_t buf_f16[1024];
    for (int i = 0; i < 1024; ++i) {
        buf_f16[i] = ggml_fp32_to_fp16((float)i);
    }

    float expected_out_1[] = {
        2520.,  5350.,  8495.,  8600.,  6065.,  3200.,  2640.,  5610.,  8915.,  9020.,  6365.,
        3360.,  2760.,  5870.,  9335.,  9440.,  6665.,  3520.,  2880.,  6130.,  9755.,  9860.,
        6965.,  3680.,  3000.,  6390.,  10175., 10280., 7265.,  3840.,  3120.,  6650.,  10595.,
        10700., 7565.,  4000.,  3240.,  6910.,  11015., 11120., 7865.,  4160.,  6720.,  13825.,
        21320., 21650., 14840., 7625.,  7140.,  14685., 22640., 22970., 15740., 8085.,  7560.,
        15545., 23960., 24290., 16640., 8545.,  7980.,  16405., 25280., 25610., 17540., 9005.,
        8400.,  17265., 26600., 26930., 18440., 9465.,  8820.,  18125., 27920., 28250., 19340.,
        9925.,  9240.,  18985., 29240., 29570., 20240., 10385.
    };
    float expected_out_2[] = {
        2520.,  2550.,  5380.,  5445.,  5950.,  6025.,  3160.,  3200.,  2640.,  2670.,  5640.,
        5705.,  6250.,  6325.,  3320.,  3360.,  2760.,  2790.,  5900.,  5965.,  6550.,  6625.,
        3480.,  3520.,  2880.,  2910.,  6160.,  6225.,  6850.,  6925.,  3640.,  3680.,  3000.,
        3030.,  6420.,  6485.,  7150.,  7225.,  3800.,  3840.,  3120.,  3150.,  6680.,  6745.,
        7450.,  7525.,  3960.,  4000.,  3240.,  3270.,  6940.,  7005.,  7750.,  7825.,  4120.,
        4160.,  6720.,  6825.,  13930., 14145., 14500., 14725., 7510.,  7625.,  7140.,  7245.,
        14790., 15005., 15400., 15625., 7970.,  8085.,  7560.,  7665.,  15650., 15865., 16300.,
        16525., 8430.,  8545.,  7980.,  8085.,  16510., 16725., 17200., 17425., 8890.,  9005.,
        8400.,  8505.,  17370., 17585., 18100., 18325., 9350.,  9465.,  8820.,  8925.,  18230.,
        18445., 19000., 19225., 9810.,  9925.,  9240.,  9345.,  19090., 19305., 19900., 20125.,
        10270., 10385.
    };
    float expected_out_3[] = {
        2520., 2550.,  2580.,  5410.,  2835.,  2870.,  5985.,  3120.,  3160.,  3200.,  2640.,
        2670., 2700.,  5670.,  2975.,  3010.,  6285.,  3280.,  3320.,  3360.,  2760.,  2790.,
        2820., 5930.,  3115.,  3150.,  6585.,  3440.,  3480.,  3520.,  2880.,  2910.,  2940.,
        6190., 3255.,  3290.,  6885.,  3600.,  3640.,  3680.,  3000.,  3030.,  3060.,  6450.,
        3395., 3430.,  7185.,  3760.,  3800.,  3840.,  3120.,  3150.,  3180.,  6710.,  3535.,
        3570., 7485.,  3920.,  3960.,  4000.,  3240.,  3270.,  3300.,  6970.,  3675.,  3710.,
        7785., 4080.,  4120.,  4160.,  6720.,  6825.,  6930.,  14035., 7110.,  7220.,  14610.,
        7395., 7510.,  7625.,  7140.,  7245.,  7350.,  14895., 7550.,  7660.,  15510., 7855.,
        7970., 8085.,  7560.,  7665.,  7770.,  15755., 7990.,  8100.,  16410., 8315.,  8430.,
        8545., 7980.,  8085.,  8190.,  16615., 8430.,  8540.,  17310., 8775.,  8890.,  9005.,
        8400., 8505.,  8610.,  17475., 8870.,  8980.,  18210., 9235.,  9350.,  9465.,  8820.,
        8925., 9030.,  18335., 9310.,  9420.,  19110., 9695.,  9810.,  9925.,  9240.,  9345.,
        9450., 19195., 9750.,  9860.,  20010., 10155., 10270., 10385.
    };
    float expected_out_4[] = {
        2520.,  2800.,  5630.,  2835.,  5700.,  2870., 5770.,  2905., 3200.,  2640.,  2940.,
        5910.,  2975.,  5980.,  3010.,  6050.,  3045., 3360.,  2760., 3080.,  6190.,  3115.,
        6260.,  3150.,  6330.,  3185.,  3520.,  2880., 3220.,  6470., 3255.,  6540.,  3290.,
        6610.,  3325.,  3680.,  3000.,  3360.,  6750., 3395.,  6820., 3430.,  6890.,  3465.,
        3840.,  3120.,  3500.,  7030.,  3535.,  7100., 3570.,  7170., 3605.,  4000.,  3240.,
        3640.,  7310.,  3675.,  7380.,  3710.,  7450., 3745.,  4160., 6720.,  7000.,  14105.,
        7110.,  14325., 7220.,  14545., 7330.,  7625., 7140.,  7440., 14985., 7550.,  15205.,
        7660.,  15425., 7770.,  8085.,  7560.,  7880., 15865., 7990., 16085., 8100.,  16305.,
        8210.,  8545.,  7980.,  8320.,  16745., 8430., 16965., 8540., 17185., 8650.,  9005.,
        8400.,  8760.,  17625., 8870.,  17845., 8980., 18065., 9090., 9465.,  8820.,  9200.,
        18505., 9310.,  18725., 9420.,  18945., 9530., 9925.,  9240., 9640.,  19385., 9750.,
        19605., 9860.,  19825., 9970.,  10385.
    };
    float expected_out_5[] = {
        2520., 2800., 3080.,  2550., 2835., 3120.,  2580., 2870., 3160.,  2610., 2905., 3200.,
        2640., 2940., 3240.,  2670., 2975., 3280.,  2700., 3010., 3320.,  2730., 3045., 3360.,
        2760., 3080., 3400.,  2790., 3115., 3440.,  2820., 3150., 3480.,  2850., 3185., 3520.,
        2880., 3220., 3560.,  2910., 3255., 3600.,  2940., 3290., 3640.,  2970., 3325., 3680.,
        3000., 3360., 3720.,  3030., 3395., 3760.,  3060., 3430., 3800.,  3090., 3465., 3840.,
        3120., 3500., 3880.,  3150., 3535., 3920.,  3180., 3570., 3960.,  3210., 3605., 4000.,
        3240., 3640., 4040.,  3270., 3675., 4080.,  3300., 3710., 4120.,  3330., 3745., 4160.,
        6720., 7000., 7280.,  6825., 7110., 7395.,  6930., 7220., 7510.,  7035., 7330., 7625.,
        7140., 7440., 7740.,  7245., 7550., 7855.,  7350., 7660., 7970.,  7455., 7770., 8085.,
        7560., 7880., 8200.,  7665., 7990., 8315.,  7770., 8100., 8430.,  7875., 8210., 8545.,
        7980., 8320., 8660.,  8085., 8430., 8775.,  8190., 8540., 8890.,  8295., 8650., 9005.,
        8400., 8760., 9120.,  8505., 8870., 9235.,  8610., 8980., 9350.,  8715., 9090., 9465.,
        8820., 9200., 9580.,  8925., 9310., 9695.,  9030., 9420., 9810.,  9135., 9530., 9925.,
        9240., 9640., 10040., 9345., 9750., 10155., 9450., 9860., 10270., 9555., 9970., 10385.
    };
    float expected_out_6[] = {
        5350.,  8495.,  8600.,  6065.,  5610.,  8915.,  9020.,  6365.,  5870.,  9335.,
        9440.,  6665.,  6130.,  9755.,  9860.,  6965.,  6390.,  10175., 10280., 7265.,
        6650.,  10595., 10700., 7565.,  6910.,  11015., 11120., 7865.,  13825., 21320.,
        21650., 14840., 14685., 22640., 22970., 15740., 15545., 23960., 24290., 16640.,
        16405., 25280., 25610., 17540., 17265., 26600., 26930., 18440., 18125., 27920.,
        28250., 19340., 18985., 29240., 29570., 20240.
    };
    float expected_out_7[] = { 0., 8495.,  0., 8600.,  0., 0., 8915.,  0., 9020.,  0.,
                               0., 9335.,  0., 9440.,  0., 0., 9755.,  0., 9860.,  0.,
                               0., 10175., 0., 10280., 0., 0., 10595., 0., 10700., 0.,
                               0., 11015., 0., 11120., 0., 0., 21320., 0., 21650., 0.,
                               0., 22640., 0., 22970., 0., 0., 23960., 0., 24290., 0.,
                               0., 25280., 0., 25610., 0., 0., 26600., 0., 26930., 0.,
                               0., 27920., 0., 28250., 0., 0., 29240., 0., 29570., 0. };

    // conv transpose 1d
    {
        struct ggml_context * ctx = make_ctx();

        struct ggml_tensor * t = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, 3, 5, 2); // l x cin x n
        memcpy(t->data, buf_f32, ggml_nbytes(t));

        struct ggml_tensor * k = ggml_new_tensor_3d(ctx, GGML_TYPE_F16, 4, 7, 5); // k x cout x cin
        memcpy(k->data, buf_f16, ggml_nbytes(k));

        struct ggml_tensor * out_1 = ggml_conv_transpose_1d(ctx, k, t, 1 /* s0 */, 0 /* p0 */, 1 /* d0 */);
        struct ggml_tensor * out_2 = ggml_conv_transpose_1d(ctx, k, t, 2 /* s0 */, 0 /* p0 */, 1 /* d0 */);
        struct ggml_tensor * out_3 = ggml_conv_transpose_1d(ctx, k, t, 3 /* s0 */, 0 /* p0 */, 1 /* d0 */);

        struct ggml_tensor * out_4 = ggml_conv_transpose_1d(ctx, k, t, 1 /* s0 */, 0 /* p0 */, 2 /* d0 */);
        struct ggml_tensor * out_5 = ggml_conv_transpose_1d(ctx, k, t, 1 /* s0 */, 0 /* p0 */, 3 /* d0 */);

        struct ggml_tensor * out_6 = ggml_conv_transpose_1d(ctx, k, t, 1 /* s0 */, 1 /* p0 */, 1 /* d0 */);

        struct ggml_tensor * out_7 = ggml_conv_transpose_1d(ctx, k, t, 2 /* s0 */, 3 /* p0 */, 2 /* d0 */);

        struct ggml_cgraph * gf_1 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_2 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_3 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_4 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_5 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_6 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_7 = ggml_new_graph(ctx);

        ggml_build_forward_expand(gf_1, out_1);
        ggml_build_forward_expand(gf_2, out_2);
        ggml_build_forward_expand(gf_3, out_3);
        ggml_build_forward_expand(gf_4, out_4);
        ggml_build_forward_expand(gf_5, out_5);
        ggml_build_forward_expand(gf_6, out_6);
        ggml_build_forward_expand(gf_7, out_7);

        ggml_graph_compute_with_ctx(ctx, gf_1, 1);
        ggml_graph_compute_with_ctx(ctx, gf_2, 1);
        ggml_graph_compute_with_ctx(ctx, gf_3, 1);
        ggml_graph_compute_with_ctx(ctx, gf_4, 1);
        ggml_graph_compute_with_ctx(ctx, gf_5, 1);
        ggml_graph_compute_with_ctx(ctx, gf_6, 1);
        ggml_graph_compute_with_ctx(ctx, gf_7, 1);

        check_tensor(out_1, (float*)expected_out_1, 6, 7, 2);
        check_tensor(out_2, (float*)expected_out_2, 8, 7, 2);
        check_tensor(out_3, (float*)expected_out_3, 10, 7, 2);
        check_tensor(out_4, (float*)expected_out_4, 9, 7, 2);
        check_tensor(out_5, (float*)expected_out_5, 12, 7, 2);
        check_tensor(out_6, (float*)expected_out_6, 4, 7, 2);
        check_tensor(out_7, (float*)expected_out_7, 5, 7, 2);
    }
}

void test_conv_transpose_2d(void) {

    float buf_f32[1024];
    for (int i = 0; i < 1024; ++i) {
        buf_f32[i] = (float)i;
    }

    ggml_fp16_t buf_f16[1024];
    for (int i = 0; i < 1024; ++i) {
        buf_f16[i] = ggml_fp32_to_fp16((float)i);
    }

    float expected_out_1[3][3][4] = {
        {
            {72.0, 162.0, 188.0, 106.0},
            {192.0, 430.0, 490.0, 274.0},
            {132.0, 292.0, 326.0, 180.0},
        },
        {
            {96.0, 218.0, 260.0, 146.0},
            {264.0, 590.0, 682.0, 378.0},
            {180.0, 396.0, 446.0, 244.0},
        },
        {
            {120.0, 274.0, 332.0, 186.0},
            {336.0, 750.0, 874.0, 482.0},
            {228.0, 500.0, 566.0, 308.0},
        },
    };

    float expected_out_2[3][4][6] = {
        {
            {72.0, 78.0, 84.0, 92.0, 96.0, 106.0},
            {84.0, 90.0, 100.0, 108.0, 116.0, 126.0},
            {108.0, 120.0, 120.0, 134.0, 132.0, 148.0},
            {132.0, 144.0, 148.0, 162.0, 164.0, 180.0},
        },
        {
            {96.0, 102.0, 116.0, 124.0, 136.0, 146.0},
            {108.0, 114.0, 132.0, 140.0, 156.0, 166.0},
            {156.0, 168.0, 176.0, 190.0, 196.0, 212.0},
            {180.0, 192.0, 204.0, 218.0, 228.0, 244.0},
        },
        {
            {120.0, 126.0, 148.0, 156.0, 176.0, 186.0},
            {132.0, 138.0, 164.0, 172.0, 196.0, 206.0},
            {204.0, 216.0, 232.0, 246.0, 260.0, 276.0},
            {228.0, 240.0, 260.0, 274.0, 292.0, 308.0},
        },
    };

    float expected_out_3[3][5][8] = {
        {
            {72.0, 78.0, 0.0, 84.0, 92.0, 0.0, 96.0, 106.0},
            {84.0, 90.0, 0.0, 100.0, 108.0, 0.0, 116.0, 126.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {108.0, 120.0, 0.0, 120.0, 134.0, 0.0, 132.0, 148.0},
            {132.0, 144.0, 0.0, 148.0, 162.0, 0.0, 164.0, 180.0},
        },
        {
            {96.0, 102.0, 0.0, 116.0, 124.0, 0.0, 136.0, 146.0},
            {108.0, 114.0, 0.0, 132.0, 140.0, 0.0, 156.0, 166.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {156.0, 168.0, 0.0, 176.0, 190.0, 0.0, 196.0, 212.0},
            {180.0, 192.0, 0.0, 204.0, 218.0, 0.0, 228.0, 244.0},
        },
        {
            {120.0, 126.0, 0.0, 148.0, 156.0, 0.0, 176.0, 186.0},
            {132.0, 138.0, 0.0, 164.0, 172.0, 0.0, 196.0, 206.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {204.0, 216.0, 0.0, 232.0, 246.0, 0.0, 260.0, 276.0},
            {228.0, 240.0, 0.0, 260.0, 274.0, 0.0, 292.0, 308.0},
        },
    };

    // conv transpose 2d with stride 1, 2 & 3
    {
        struct ggml_context * ctx = make_ctx();

        struct ggml_tensor * t = ggml_new_tensor_4d(ctx, GGML_TYPE_F32, 3, 2, 2, 1); // w x h x cin
        memcpy(t->data, buf_f32, ggml_nbytes(t));

        struct ggml_tensor * k = ggml_new_tensor_4d(ctx, GGML_TYPE_F16, 2, 2, 3, 2); // w x h cin x cout
        memcpy(k->data, buf_f16, ggml_nbytes(k));

        struct ggml_tensor * out_1 = ggml_conv_transpose_2d_p0(ctx, k, t, 1);
        struct ggml_tensor * out_2 = ggml_conv_transpose_2d_p0(ctx, k, t, 2);
        struct ggml_tensor * out_3 = ggml_conv_transpose_2d_p0(ctx, k, t, 3);

        struct ggml_cgraph * gf_1 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_2 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_3 = ggml_new_graph(ctx);

        ggml_build_forward_expand(gf_1, out_1);
        ggml_build_forward_expand(gf_2, out_2);
        ggml_build_forward_expand(gf_3, out_3);

        ggml_graph_compute_with_ctx(ctx, gf_1, 1);
        ggml_graph_compute_with_ctx(ctx, gf_2, 1);
        ggml_graph_compute_with_ctx(ctx, gf_3, 1);

        // printf("in\n");
        // printf_tensor(t);
        // printf("\n\nkernel\n");
        // printf_tensor(k);
        // printf("\n\nout\n");
        // printf_tensor(out);
        // printf("\n\nout_2\n");
        // printf_tensor(out_2);
        // printf("\n\nout_3\n");
        // printf_tensor(out_3);

        check_tensor(out_1, (float*)expected_out_1, 4, 3, 3);
        check_tensor(out_2, (float*)expected_out_2, 6, 4, 3);
        check_tensor(out_3, (float*)expected_out_3, 8, 5, 3);

    }
}

void test_col2im(void) {

    float buf_f32[2048];
    for (int i = 0; i < 2048; ++i) {
        buf_f32[i] = (float)i;
    }

    float expected_out_1[] = { 0.0,   8.0,   24.0,  48.0,  80.0,  85.0,  90.0,  88.0,  78.0,
                               60.0,  34.0,  35.0,  78.0,  129.0, 188.0, 255.0, 260.0, 265.0,
                               228.0, 183.0, 130.0, 69.0,  70.0,  148.0, 234.0, 328.0, 430.0,
                               435.0, 440.0, 368.0, 288.0, 200.0, 104.0, 105.0, 218.0, 339.0,
                               468.0, 605.0, 610.0, 615.0, 508.0, 393.0, 270.0, 139.0, 140.0,
                               288.0, 444.0, 608.0, 780.0, 785.0, 790.0, 648.0, 498.0, 340.0,
                               174.0, 175.0, 358.0, 549.0, 748.0, 955.0, 960.0, 965.0, 788.0,
                               603.0, 410.0, 209.0 };
    float expected_out_2[] = {
        0.0,   7.0,   15.0,  29.0,  45.0,  31.0,  48.0,  33.0,  51.0,  35.0,  54.0,  37.0,
        57.0,  39.0,  53.0,  27.0,  34.0,  35.0,  42.0,  85.0,  99.0,  150.0, 101.0, 153.0,
        103.0, 156.0, 105.0, 159.0, 107.0, 162.0, 109.0, 123.0, 62.0,  69.0,  70.0,  77.0,
        155.0, 169.0, 255.0, 171.0, 258.0, 173.0, 261.0, 175.0, 264.0, 177.0, 267.0, 179.0,
        193.0, 97.0,  104.0, 105.0, 112.0, 225.0, 239.0, 360.0, 241.0, 363.0, 243.0, 366.0,
        245.0, 369.0, 247.0, 372.0, 249.0, 263.0, 132.0, 139.0, 140.0, 147.0, 295.0, 309.0,
        465.0, 311.0, 468.0, 313.0, 471.0, 315.0, 474.0, 317.0, 477.0, 319.0, 333.0, 167.0,
        174.0, 175.0, 182.0, 365.0, 379.0, 570.0, 381.0, 573.0, 383.0, 576.0, 385.0, 579.0,
        387.0, 582.0, 389.0, 403.0, 202.0, 209.0
    };
    float expected_out_3[] = { 8.0,   24.0,  48.0,  80.0,  85.0,  90.0,  88.0,  78.0,  60.0,
                               78.0,  129.0, 188.0, 255.0, 260.0, 265.0, 228.0, 183.0, 130.0,
                               148.0, 234.0, 328.0, 430.0, 435.0, 440.0, 368.0, 288.0, 200.0,
                               218.0, 339.0, 468.0, 605.0, 610.0, 615.0, 508.0, 393.0, 270.0,
                               288.0, 444.0, 608.0, 780.0, 785.0, 790.0, 648.0, 498.0, 340.0,
                               358.0, 549.0, 748.0, 955.0, 960.0, 965.0, 788.0, 603.0, 410.0 };
    float expected_out_4[] = { 0.0,   1.0,   9.0,   11.0,  27.0,  30.0,  54.0,  51.0,  82.0,
                               72.0,  75.0,  57.0,  59.0,  33.0,  34.0,  35.0,  36.0,  79.0,
                               81.0,  132.0, 135.0, 194.0, 156.0, 222.0, 177.0, 180.0, 127.0,
                               129.0, 68.0,  69.0,  70.0,  71.0,  149.0, 151.0, 237.0, 240.0,
                               334.0, 261.0, 362.0, 282.0, 285.0, 197.0, 199.0, 103.0, 104.0,
                               105.0, 106.0, 219.0, 221.0, 342.0, 345.0, 474.0, 366.0, 502.0,
                               387.0, 390.0, 267.0, 269.0, 138.0, 139.0, 140.0, 141.0, 289.0,
                               291.0, 447.0, 450.0, 614.0, 471.0, 642.0, 492.0, 495.0, 337.0,
                               339.0, 173.0, 174.0, 175.0, 176.0, 359.0, 361.0, 552.0, 555.0,
                               754.0, 576.0, 782.0, 597.0, 600.0, 407.0, 409.0, 208.0, 209.0 };
    float expected_out_5[] = {
        0.0,   8.0,   0.0,   24.0,  0.0,   48.0,  0.0,   80.0,  0.0,   85.0,  0.0,   90.0,
        0.0,   88.0,  0.0,   78.0,  0.0,   60.0,  0.0,   0.0,   78.0,  0.0,   129.0, 0.0,
        188.0, 0.0,   255.0, 0.0,   260.0, 0.0,   265.0, 0.0,   228.0, 0.0,   183.0, 0.0,
        130.0, 0.0,   0.0,   148.0, 0.0,   234.0, 0.0,   328.0, 0.0,   430.0, 0.0,   435.0,
        0.0,   440.0, 0.0,   368.0, 0.0,   288.0, 0.0,   200.0, 0.0,   0.0,   218.0, 0.0,
        339.0, 0.0,   468.0, 0.0,   605.0, 0.0,   610.0, 0.0,   615.0, 0.0,   508.0, 0.0,
        393.0, 0.0,   270.0, 0.0,   0.0,   288.0, 0.0,   444.0, 0.0,   608.0, 0.0,   780.0,
        0.0,   785.0, 0.0,   790.0, 0.0,   648.0, 0.0,   498.0, 0.0,   340.0, 0.0,   0.0,
        358.0, 0.0,   549.0, 0.0,   748.0, 0.0,   955.0, 0.0,   960.0, 0.0,   965.0, 0.0,
        788.0, 0.0,   603.0, 0.0,   410.0, 0.0
    };
    float expected_out_6[] = {
        0.0,    13.0,   0.0,    1.0,    26.0,   0.0,    14.0,   39.0,   2.0,    27.0,   52.0,
        15.0,   40.0,   68.0,   28.0,   53.0,   94.0,   41.0,   70.0,   120.0,  54.0,   96.0,
        146.0,  72.0,   122.0,  172.0,  98.0,   148.0,  204.0,  124.0,  174.0,  100.0,  150.0,
        207.0,  126.0,  176.0,  102.0,  152.0,  210.0,  128.0,  178.0,  104.0,  154.0,  213.0,
        130.0,  180.0,  106.0,  156.0,  216.0,  132.0,  182.0,  108.0,  158.0,  219.0,  134.0,
        184.0,  110.0,  160.0,  222.0,  136.0,  186.0,  112.0,  162.0,  212.0,  138.0,  188.0,
        88.0,   164.0,  214.0,  101.0,  190.0,  89.0,   114.0,  216.0,  102.0,  127.0,  90.0,
        115.0,  140.0,  103.0,  128.0,  0.0,    116.0,  141.0,  0.0,    129.0,  0.0,    0.0,
        156.0,  0.0,    144.0,  169.0,  0.0,    157.0,  182.0,  145.0,  170.0,  195.0,  158.0,
        183.0,  354.0,  171.0,  196.0,  380.0,  184.0,  356.0,  406.0,  197.0,  382.0,  432.0,
        358.0,  408.0,  458.0,  384.0,  434.0,  633.0,  410.0,  460.0,  386.0,  436.0,  636.0,
        412.0,  462.0,  388.0,  438.0,  639.0,  414.0,  464.0,  390.0,  440.0,  642.0,  416.0,
        466.0,  392.0,  442.0,  645.0,  418.0,  468.0,  394.0,  444.0,  648.0,  420.0,  470.0,
        396.0,  446.0,  651.0,  422.0,  472.0,  398.0,  448.0,  498.0,  424.0,  474.0,  231.0,
        450.0,  500.0,  244.0,  476.0,  232.0,  257.0,  502.0,  245.0,  270.0,  233.0,  258.0,
        283.0,  246.0,  271.0,  0.0,    259.0,  284.0,  0.0,    272.0,  0.0,    0.0,    299.0,
        0.0,    287.0,  312.0,  0.0,    300.0,  325.0,  288.0,  313.0,  338.0,  301.0,  326.0,
        640.0,  314.0,  339.0,  666.0,  327.0,  642.0,  692.0,  340.0,  668.0,  718.0,  644.0,
        694.0,  744.0,  670.0,  720.0,  1062.0, 696.0,  746.0,  672.0,  722.0,  1065.0, 698.0,
        748.0,  674.0,  724.0,  1068.0, 700.0,  750.0,  676.0,  726.0,  1071.0, 702.0,  752.0,
        678.0,  728.0,  1074.0, 704.0,  754.0,  680.0,  730.0,  1077.0, 706.0,  756.0,  682.0,
        732.0,  1080.0, 708.0,  758.0,  684.0,  734.0,  784.0,  710.0,  760.0,  374.0,  736.0,
        786.0,  387.0,  762.0,  375.0,  400.0,  788.0,  388.0,  413.0,  376.0,  401.0,  426.0,
        389.0,  414.0,  0.0,    402.0,  427.0,  0.0,    415.0,  0.0,    0.0,    442.0,  0.0,
        430.0,  455.0,  0.0,    443.0,  468.0,  431.0,  456.0,  481.0,  444.0,  469.0,  926.0,
        457.0,  482.0,  952.0,  470.0,  928.0,  978.0,  483.0,  954.0,  1004.0, 930.0,  980.0,
        1030.0, 956.0,  1006.0, 1491.0, 982.0,  1032.0, 958.0,  1008.0, 1494.0, 984.0,  1034.0,
        960.0,  1010.0, 1497.0, 986.0,  1036.0, 962.0,  1012.0, 1500.0, 988.0,  1038.0, 964.0,
        1014.0, 1503.0, 990.0,  1040.0, 966.0,  1016.0, 1506.0, 992.0,  1042.0, 968.0,  1018.0,
        1509.0, 994.0,  1044.0, 970.0,  1020.0, 1070.0, 996.0,  1046.0, 517.0,  1022.0, 1072.0,
        530.0,  1048.0, 518.0,  543.0,  1074.0, 531.0,  556.0,  519.0,  544.0,  569.0,  532.0,
        557.0,  0.0,    545.0,  570.0,  0.0,    558.0,  0.0,    0.0,    585.0,  0.0,    573.0,
        598.0,  0.0,    586.0,  611.0,  574.0,  599.0,  624.0,  587.0,  612.0,  1212.0, 600.0,
        625.0,  1238.0, 613.0,  1214.0, 1264.0, 626.0,  1240.0, 1290.0, 1216.0, 1266.0, 1316.0,
        1242.0, 1292.0, 1920.0, 1268.0, 1318.0, 1244.0, 1294.0, 1923.0, 1270.0, 1320.0, 1246.0,
        1296.0, 1926.0, 1272.0, 1322.0, 1248.0, 1298.0, 1929.0, 1274.0, 1324.0, 1250.0, 1300.0,
        1932.0, 1276.0, 1326.0, 1252.0, 1302.0, 1935.0, 1278.0, 1328.0, 1254.0, 1304.0, 1938.0,
        1280.0, 1330.0, 1256.0, 1306.0, 1356.0, 1282.0, 1332.0, 660.0,  1308.0, 1358.0, 673.0,
        1334.0, 661.0,  686.0,  1360.0, 674.0,  699.0,  662.0,  687.0,  712.0,  675.0,  700.0,
        0.0,    688.0,  713.0,  0.0,    701.0,  0.0,    0.0,    728.0,  0.0,    716.0,  741.0,
        0.0,    729.0,  754.0,  717.0,  742.0,  767.0,  730.0,  755.0,  1498.0, 743.0,  768.0,
        1524.0, 756.0,  1500.0, 1550.0, 769.0,  1526.0, 1576.0, 1502.0, 1552.0, 1602.0, 1528.0,
        1578.0, 2349.0, 1554.0, 1604.0, 1530.0, 1580.0, 2352.0, 1556.0, 1606.0, 1532.0, 1582.0,
        2355.0, 1558.0, 1608.0, 1534.0, 1584.0, 2358.0, 1560.0, 1610.0, 1536.0, 1586.0, 2361.0,
        1562.0, 1612.0, 1538.0, 1588.0, 2364.0, 1564.0, 1614.0, 1540.0, 1590.0, 2367.0, 1566.0,
        1616.0, 1542.0, 1592.0, 1642.0, 1568.0, 1618.0, 803.0,  1594.0, 1644.0, 816.0,  1620.0,
        804.0,  829.0,  1646.0, 817.0,  842.0,  805.0,  830.0,  855.0,  818.0,  843.0,  0.0,
        831.0,  856.0,  0.0,    844.0,  0.0,    0.0,    871.0,  0.0,    859.0,  884.0,  0.0,
        872.0,  897.0,  860.0,  885.0,  910.0,  873.0,  898.0,  1784.0, 886.0,  911.0,  1810.0,
        899.0,  1786.0, 1836.0, 912.0,  1812.0, 1862.0, 1788.0, 1838.0, 1888.0, 1814.0, 1864.0,
        2778.0, 1840.0, 1890.0, 1816.0, 1866.0, 2781.0, 1842.0, 1892.0, 1818.0, 1868.0, 2784.0,
        1844.0, 1894.0, 1820.0, 1870.0, 2787.0, 1846.0, 1896.0, 1822.0, 1872.0, 2790.0, 1848.0,
        1898.0, 1824.0, 1874.0, 2793.0, 1850.0, 1900.0, 1826.0, 1876.0, 2796.0, 1852.0, 1902.0,
        1828.0, 1878.0, 1928.0, 1854.0, 1904.0, 946.0,  1880.0, 1930.0, 959.0,  1906.0, 947.0,
        972.0,  1932.0, 960.0,  985.0,  948.0,  973.0,  998.0,  961.0,  986.0,  0.0,    974.0,
        999.0,  0.0,    987.0,  0.0,    0.0,    1014.0, 0.0,    1002.0, 1027.0, 0.0,    1015.0,
        1040.0, 1003.0, 1028.0, 1053.0, 1016.0, 1041.0, 2070.0, 1029.0, 1054.0, 2096.0, 1042.0,
        2072.0, 2122.0, 1055.0, 2098.0, 2148.0, 2074.0, 2124.0, 2174.0, 2100.0, 2150.0, 3207.0,
        2126.0, 2176.0, 2102.0, 2152.0, 3210.0, 2128.0, 2178.0, 2104.0, 2154.0, 3213.0, 2130.0,
        2180.0, 2106.0, 2156.0, 3216.0, 2132.0, 2182.0, 2108.0, 2158.0, 3219.0, 2134.0, 2184.0,
        2110.0, 2160.0, 3222.0, 2136.0, 2186.0, 2112.0, 2162.0, 3225.0, 2138.0, 2188.0, 2114.0,
        2164.0, 2214.0, 2140.0, 2190.0, 1089.0, 2166.0, 2216.0, 1102.0, 2192.0, 1090.0, 1115.0,
        2218.0, 1103.0, 1128.0, 1091.0, 1116.0, 1141.0, 1104.0, 1129.0, 0.0,    1117.0, 1142.0,
        0.0,    1130.0, 0.0,    0.0,    1157.0, 0.0,    1145.0, 1170.0, 0.0,    1158.0, 1183.0,
        1146.0, 1171.0, 1196.0, 1159.0, 1184.0, 2356.0, 1172.0, 1197.0, 2382.0, 1185.0, 2358.0,
        2408.0, 1198.0, 2384.0, 2434.0, 2360.0, 2410.0, 2460.0, 2386.0, 2436.0, 3636.0, 2412.0,
        2462.0, 2388.0, 2438.0, 3639.0, 2414.0, 2464.0, 2390.0, 2440.0, 3642.0, 2416.0, 2466.0,
        2392.0, 2442.0, 3645.0, 2418.0, 2468.0, 2394.0, 2444.0, 3648.0, 2420.0, 2470.0, 2396.0,
        2446.0, 3651.0, 2422.0, 2472.0, 2398.0, 2448.0, 3654.0, 2424.0, 2474.0, 2400.0, 2450.0,
        2500.0, 2426.0, 2476.0, 1232.0, 2452.0, 2502.0, 1245.0, 2478.0, 1233.0, 1258.0, 2504.0,
        1246.0, 1271.0, 1234.0, 1259.0, 1284.0, 1247.0, 1272.0, 0.0,    1260.0, 1285.0, 0.0,
        1273.0, 0.0,    0.0,    1300.0, 0.0,    1288.0, 1313.0, 0.0,    1301.0, 1326.0, 1289.0,
        1314.0, 1339.0, 1302.0, 1327.0, 2642.0, 1315.0, 1340.0, 2668.0, 1328.0, 2644.0, 2694.0,
        1341.0, 2670.0, 2720.0, 2646.0, 2696.0, 2746.0, 2672.0, 2722.0, 4065.0, 2698.0, 2748.0,
        2674.0, 2724.0, 4068.0, 2700.0, 2750.0, 2676.0, 2726.0, 4071.0, 2702.0, 2752.0, 2678.0,
        2728.0, 4074.0, 2704.0, 2754.0, 2680.0, 2730.0, 4077.0, 2706.0, 2756.0, 2682.0, 2732.0,
        4080.0, 2708.0, 2758.0, 2684.0, 2734.0, 4083.0, 2710.0, 2760.0, 2686.0, 2736.0, 2786.0,
        2712.0, 2762.0, 1375.0, 2738.0, 2788.0, 1388.0, 2764.0, 1376.0, 1401.0, 2790.0, 1389.0,
        1414.0, 1377.0, 1402.0, 1427.0, 1390.0, 1415.0, 0.0,    1403.0, 1428.0, 0.0,    1416.0,
        0.0
    };

    int flips[][4] = {{0, 1, 2, 3},
                      {0, 1, 3, 2},
                      {0, 2, 3, 1},
                      {0, 2, 1, 3},
                      {0, 3, 2, 1},
                      {0, 3, 1, 2}};
    int backflips[sizeof(flips)/sizeof(flips[0])][4] = {0};
    for (size_t i = 0; i < sizeof(flips)/sizeof(flips[0]); ++i) {
        for (size_t j=0; j < 4; ++j)
            backflips[i][flips[i][j]] = j;
    }

    for (size_t i = 0; i < sizeof(flips)/sizeof(flips[0]); ++i) {
        struct ggml_context * ctx = make_ctx();

        struct ggml_tensor *a =
                ggml_new_tensor_4d(ctx, GGML_TYPE_F32, 7, 5, 3, 2);   // IW, KW, OC, N
        memcpy(a->data, buf_f32, ggml_nbytes(a));
        struct ggml_tensor *b =
                ggml_new_tensor_4d(ctx, GGML_TYPE_F32, 13, 11, 5, 2);   // IW, KW, OC, N
        memcpy(b->data, buf_f32, ggml_nbytes(b));

        a = ggml_cont(ctx, ggml_permute(ctx, a, flips[i][0], flips[i][1], flips[i][2], flips[i][3]));
        a = ggml_permute(ctx, a, backflips[i][0], backflips[i][1], backflips[i][2], backflips[i][3]);
        b = ggml_cont(ctx, ggml_permute(ctx, b, flips[i][0], flips[i][1], flips[i][2], flips[i][3]));
        b = ggml_permute(ctx, b, backflips[i][0], backflips[i][1], backflips[i][2], backflips[i][3]);

        struct ggml_tensor * out_1 = ggml_col2im(ctx, a,
                                                 1 /* s0 */, 1 /* s1 */,
                                                 0 /* p0 */, 0 /* p1 */,
                                                 1 /* d0 */, 1 /* d1 */,
                                                 1 /* KH */, 1 /* IH */);
        struct ggml_tensor * out_2 = ggml_col2im(ctx,
                                                 a,
                                                 2 /* s0 */, 1 /* s1 */,
                                                 0 /* p0 */, 0 /* p1 */,
                                                 1 /* d0 */, 1 /* d1 */,
                                                 1 /* KH */, 1 /* IH */);
        struct ggml_tensor * out_3 = ggml_col2im(ctx,
                                                 a,
                                                 1 /* s0 */, 1 /* s1 */,
                                                 1 /* p0 */, 0 /* p1 */,
                                                 1 /* d0 */, 1 /* d1 */,
                                                 1 /* KH */, 1 /* IH */);
        struct ggml_tensor * out_4 = ggml_col2im(ctx,
                                                 a,
                                                 1 /* s0 */, 1 /* s1 */,
                                                 0 /* p0 */, 0 /* p1 */,
                                                 2 /* d0 */, 1 /* d1 */,
                                                 1 /* KH */, 1 /* IH */);
        struct ggml_tensor * out_5 = ggml_col2im(ctx,
                                                 a,
                                                 2 /* s0 */, 1 /* s1 */,
                                                 1 /* p0 */, 0 /* p1 */,
                                                 2 /* d0 */, 1 /* d1 */,
                                                 1 /* KH */, 1 /* IH */);
        struct ggml_tensor * out_6 = ggml_col2im(ctx,
                                                 b,
                                                 5 /* s0 */, 1 /* s1 */,
                                                 2 /* p0 */, 0 /* p1 */,
                                                 3 /* d0 */, 1 /* d1 */,
                                                 1 /* KH */, 1 /* IH */);
        struct ggml_cgraph * gf_1 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_2 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_3 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_4 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_5 = ggml_new_graph(ctx);
        struct ggml_cgraph * gf_6 = ggml_new_graph(ctx);

        ggml_build_forward_expand(gf_1, out_1);
        ggml_build_forward_expand(gf_2, out_2);
        ggml_build_forward_expand(gf_3, out_3);
        ggml_build_forward_expand(gf_4, out_4);
        ggml_build_forward_expand(gf_5, out_5);
        ggml_build_forward_expand(gf_6, out_6);

        ggml_graph_compute_with_ctx(ctx, gf_1, 1);
        ggml_graph_compute_with_ctx(ctx, gf_2, 1);
        ggml_graph_compute_with_ctx(ctx, gf_3, 1);
        ggml_graph_compute_with_ctx(ctx, gf_4, 1);
        ggml_graph_compute_with_ctx(ctx, gf_5, 1);
        ggml_graph_compute_with_ctx(ctx, gf_6, 1);

        check_tensor(out_1, (float*)expected_out_1, 11, 3, 2);   // OW, OC, N
        check_tensor(out_2, (float*)expected_out_2, 17, 3, 2);
        check_tensor(out_3, (float*)expected_out_3, 9, 3, 2);
        check_tensor(out_4, (float*)expected_out_4, 15, 3, 2);
        check_tensor(out_5, (float*)expected_out_5, 19, 3, 2);
        check_tensor(out_6, (float*)expected_out_6, 87, 5, 2);
    }
}

int main(int argc, const char * argv[]) {
    test_conv_transpose_1d();
    test_conv_transpose_1d_batched();
    test_conv_transpose_2d();
    test_col2im();
    return 0;
}
