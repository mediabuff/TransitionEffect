#include "pch.h"
#include "ColorConverter.h"

#if defined(_M_ARM)
#include <arm_neon.h>
#endif

const float CONVERT_SCALE = 65536.f;

const int R_2_Y = round_to_int(0.299f * CONVERT_SCALE);
const int G_2_Y = round_to_int(0.578f * CONVERT_SCALE);
const int B_2_Y = round_to_int(0.114f * CONVERT_SCALE);
const int R_2_U = round_to_int(-0.169f * CONVERT_SCALE);
const int G_2_U = round_to_int(-0.331f * CONVERT_SCALE);
const int B_2_U = round_to_int(0.499f * CONVERT_SCALE);
const int R_2_V = round_to_int(0.499f * CONVERT_SCALE);
const int G_2_V = round_to_int(-0.418f * CONVERT_SCALE);
const int B_2_V = round_to_int(-0.0813f * CONVERT_SCALE);

const int Y_2_RGB = round_to_int(CONVERT_SCALE);
const int V_2_R = round_to_int(1.402f * CONVERT_SCALE);
const int U_2_G = round_to_int(-0.344f * CONVERT_SCALE);
const int V_2_G = round_to_int(-0.714f * CONVERT_SCALE);
const int U_2_B = round_to_int(1.772f * CONVERT_SCALE);

__inline void YUV2RGB(int y, int u, int v, int &r, int &g, int &b)
{
    u = u - 128;
    v = v - 128;

    r = Y_2_RGB * y + V_2_R * v;
    g = Y_2_RGB * y + U_2_G * u + V_2_G * v;
    b = Y_2_RGB * y + U_2_B * u;

    r = (r + (1 << 15)) >> 16;
    g = (g + (1 << 15)) >> 16;
    b = (b + (1 << 15)) >> 16;

    r = CLAMP(r, 255, 0);
    g = CLAMP(g, 255, 0);
    b = CLAMP(b, 255, 0);
}

__inline void RGB2YUV(int r, int g, int b, int &y, int &u, int &v)
{
    y = R_2_Y * r + G_2_Y * g + B_2_Y * b;
    u = R_2_U * r + G_2_U * g + B_2_U * b + (128 << 16);
    v = R_2_V * r + G_2_V * g + B_2_V * b + (128 << 16);

    y = (y + (1 << 15)) >> 16;
    u = (u + (1 << 15)) >> 16;
    v = (v + (1 << 15)) >> 16;
}

ColorConverter::ColorConverter()
{

}

ColorConverter::~ColorConverter()
{

}

// convert NV12 <-> YV12
void ColorConverter::ConvertNV12toYV12(BYTE *p_nv12, int nv12_stride, BYTE *p_yv12, int yv12_stride, int width, int height)
{
    _MYASSERT(p_nv12);
    _MYASSERT(p_yv12);

    // NV12 is planar: Y plane, followed by packed U-V plane.

    // Copy Y plane
    for (int i = 0; i < height; i++)
    {
        memcpy(p_yv12, p_nv12, width);
        p_yv12 += yv12_stride;
        p_nv12 += nv12_stride;
    }

    int uv_width = width / 2;
    int align_right = (uv_width & ~7);
    // Convert UV
    BYTE *p_yv12_v = p_yv12;
    BYTE *p_yv12_u = p_yv12 + (yv12_stride / 2) * (height / 2);
    for (int i = 0; i < height / 2; i++)
    {
#if defined(_M_ARM)
        for (int j = 0; j < align_right; j += 8)
        {
            uint8x16_t nv12_uv = vld1q_u8(p_nv12 + j * 2);
            uint8x8_t nv12_uv_lo = vget_low_u8(nv12_uv);
            uint8x8_t nv12_uv_hi = vget_high_u8(nv12_uv);

            uint8x8x2_t yv12_uv = vuzp_u8(nv12_uv_lo, nv12_uv_hi);

            vst1_u8(p_yv12_u + j, yv12_uv.val[0]);
            vst1_u8(p_yv12_v + j, yv12_uv.val[1]);
        }

        for (int j = align_right; j < uv_width; j++)
        {
            p_yv12_u[j] = p_nv12[j * 2];
            p_yv12_v[j] = p_nv12[j * 2 + 1];
        }
#else
        for (int j = 0; j < uv_width; j++)
        {
            p_yv12_u[j] = p_nv12[j * 2];
            p_yv12_v[j] = p_nv12[j * 2 + 1];
        }
#endif
        p_yv12_u += yv12_stride / 2;
        p_yv12_v += yv12_stride / 2;
        p_nv12 += nv12_stride;
    }
}

void ColorConverter::ConvertYV12toNV12(BYTE *p_yv12, int yv12_stride, BYTE *p_nv12, int nv12_stride, int width, int height)
{
    _MYASSERT(p_nv12);
    _MYASSERT(p_yv12);

    // NV12 is planar: Y plane, followed by packed U-V plane.

    // Copy Y plane
    for (int i = 0; i < height; i++)
    {
        memcpy(p_nv12, p_yv12, width);
        p_yv12 += yv12_stride;
        p_nv12 += nv12_stride;
    }

    // Convert UV
    int uv_width = width / 2;
    int align_right = (uv_width & ~7);
    BYTE *p_yv12_v = p_yv12;
    BYTE *p_yv12_u = p_yv12 + (yv12_stride / 2) * (height / 2);
    for (int i = 0; i < height / 2; i++)
    {
#if defined(_M_ARM)
        for (int j = 0; j < align_right; j += 8)
        {
            uint8x8_t yv12_u = vld1_u8(p_yv12_u + j);
            uint8x8_t yv12_v = vld1_u8(p_yv12_v + j);
            uint8x8x2_t yv12_uv = vzip_u8(yv12_u, yv12_v);

            uint8x16_t nv12_uv = vcombine_u8(yv12_uv.val[0], yv12_uv.val[1]);

            vst1q_u8(p_nv12 + j * 2, nv12_uv);
        }

        for (int j = align_right; j < uv_width; j++)
        {
            p_nv12[j * 2] = p_yv12_u[j];
            p_nv12[j * 2 + 1] = p_yv12_v[j];
        }
#else
        for (int j = 0; j < uv_width; j++)
        {
            p_nv12[j * 2] = p_yv12_u[j];
            p_nv12[j * 2 + 1] = p_yv12_v[j];
        }
#endif
        p_yv12_u += yv12_stride / 2;
        p_yv12_v += yv12_stride / 2;
        p_nv12 += nv12_stride;
    }
}

// convert YUY2 <-> YV12
void ColorConverter::ConvertYUY2toYV12(BYTE *p_yuy2, int yuy2_stride, BYTE *p_yv12, int yv12_stride, int width, int height)
{
    _MYASSERT(p_yuy2);
    _MYASSERT(p_yv12);

    BYTE *p_yv12_y = p_yv12;
    BYTE *p_yv12_v = p_yv12 + yv12_stride * height;
    BYTE *p_yv12_u = p_yv12 + yv12_stride * height * 5 / 4;
    for (int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j += 2)
        {
            // YUY2 pattern:
            //   line 0: Y00 U00 Y01 V00
            //   line 1: Y10 U10 Y11 V10

            BYTE y00 = p_yuy2[j * 2 + 0];
            BYTE u00 = p_yuy2[j * 2 + 1];
            BYTE y01 = p_yuy2[j * 2 + 2];
            BYTE v00 = p_yuy2[j * 2 + 3];
            BYTE y10 = p_yuy2[yuy2_stride + j * 2 + 0];
            BYTE u10 = p_yuy2[yuy2_stride + j * 2 + 1];
            BYTE y11 = p_yuy2[yuy2_stride + j * 2 + 2];
            BYTE v10 = p_yuy2[yuy2_stride + j * 2 + 3];


            p_yv12_y[j + 0] = y00;
            p_yv12_y[j + 1] = y01;
            p_yv12_y[yv12_stride + j + 0] = y10;
            p_yv12_y[yv12_stride + j + 1] = y11;

            p_yv12_u[j / 2] = (u00 + u10 + 1) / 2;
            p_yv12_v[j / 2] = (v00 + v10 + 1) / 2;
        }
        p_yuy2 += yuy2_stride * 2;
        p_yv12_y += yv12_stride * 2;
        p_yv12_u += yv12_stride / 2;
        p_yv12_v += yv12_stride / 2;
    }
}

void ColorConverter::ConvertYV12toYUY2(BYTE *p_yv12, int yv12_stride, BYTE *p_yuy2, int yuy2_stride, int width, int height)
{
    _MYASSERT(p_yuy2);
    _MYASSERT(p_yv12);

    BYTE *p_yv12_y = p_yv12;
    BYTE *p_yv12_v = p_yv12 + yv12_stride * height;
    BYTE *p_yv12_u = p_yv12 + yv12_stride * height * 5 / 4;
    for (int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j += 2)
        {
            // YUY2 pattern:
            //   line 0: Y00 U00 Y01 V00
            //   line 1: Y10 U10 Y11 V10

            BYTE y00 = p_yv12_y[j + 0];
            BYTE y01 = p_yv12_y[j + 1];
            BYTE y10 = p_yv12_y[yv12_stride + j + 0];
            BYTE y11 = p_yv12_y[yv12_stride + j + 1];

            BYTE u00 = p_yv12_u[j / 2];
            BYTE v00 = p_yv12_v[j / 2];
            BYTE u10 = p_yv12_u[j / 2];
            BYTE v10 = p_yv12_v[j / 2];

            p_yuy2[j * 2 + 0] = y00;
            p_yuy2[j * 2 + 1] = u00;
            p_yuy2[j * 2 + 2] = y01;
            p_yuy2[j * 2 + 3] = v00;
            p_yuy2[yuy2_stride + j * 2 + 0] = y10;
            p_yuy2[yuy2_stride + j * 2 + 1] = u10;
            p_yuy2[yuy2_stride + j * 2 + 2] = y11;
            p_yuy2[yuy2_stride + j * 2 + 3] = v10;
        }
        p_yuy2 += yuy2_stride * 2;
        p_yv12_y += yv12_stride * 2;
        p_yv12_u += yv12_stride / 2;
        p_yv12_v += yv12_stride / 2;
    }
}

// convert UYVY <-> YV12
void ColorConverter::ConvertUYVYtoYV12(BYTE *p_uyuv, int uyuv_stride, BYTE *p_yv12, int yv12_stride, int width, int height)
{
    _MYASSERT(p_uyuv);
    _MYASSERT(p_yv12);

    BYTE *p_yv12_y = p_yv12;
    BYTE *p_yv12_v = p_yv12 + yv12_stride * height;
    BYTE *p_yv12_u = p_yv12 + yv12_stride * height * 5 / 4;
    for (int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j += 2)
        {
            // UYVY pattern:
            //   line 0: U00 Y00 V00 Y01
            //   line 1: U10 Y10 V10 Y11

            BYTE u00 = p_uyuv[j * 2 + 0];
            BYTE y00 = p_uyuv[j * 2 + 1];
            BYTE v00 = p_uyuv[j * 2 + 2];
            BYTE y01 = p_uyuv[j * 2 + 3];
            BYTE u10 = p_uyuv[uyuv_stride + j * 2 + 0];
            BYTE y10 = p_uyuv[uyuv_stride + j * 2 + 1];
            BYTE v10 = p_uyuv[uyuv_stride + j * 2 + 2];
            BYTE y11 = p_uyuv[uyuv_stride + j * 2 + 3];


            p_yv12_y[j + 0] = y00;
            p_yv12_y[j + 1] = y01;
            p_yv12_y[yv12_stride + j + 0] = y10;
            p_yv12_y[yv12_stride + j + 1] = y11;

            p_yv12_u[j / 2] = (u00 + u10 + 1) / 2;
            p_yv12_v[j / 2] = (v00 + v10 + 1) / 2;
        }
        p_uyuv += uyuv_stride * 2;
        p_yv12_y += yv12_stride * 2;
        p_yv12_u += yv12_stride / 2;
        p_yv12_v += yv12_stride / 2;
    }
}

void ColorConverter::ConvertYV12toUYVY(BYTE *p_yv12, int yv12_stride, BYTE *p_uyuv, int uyuv_stride, int width, int height)
{
    _MYASSERT(p_uyuv);
    _MYASSERT(p_yv12);

    BYTE *p_yv12_y = p_yv12;
    BYTE *p_yv12_v = p_yv12 + yv12_stride * height;
    BYTE *p_yv12_u = p_yv12 + yv12_stride * height * 5 / 4;
    for (int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j += 2)
        {
            // UYVY pattern:
            //   line 0: U00 Y00 V00 Y01
            //   line 1: U10 Y10 V10 Y11

            BYTE y00 = p_yv12_y[j + 0];
            BYTE y01 = p_yv12_y[j + 1];
            BYTE y10 = p_yv12_y[yv12_stride + j + 0];
            BYTE y11 = p_yv12_y[yv12_stride + j + 1];

            BYTE u00 = p_yv12_u[j / 2];
            BYTE v00 = p_yv12_v[j / 2];
            BYTE u10 = p_yv12_u[j / 2];
            BYTE v10 = p_yv12_v[j / 2];

            p_uyuv[j * 2 + 0] = u00;
            p_uyuv[j * 2 + 1] = y00;
            p_uyuv[j * 2 + 2] = v00;
            p_uyuv[j * 2 + 3] = y01;
            p_uyuv[uyuv_stride + j * 2 + 0] = u10;
            p_uyuv[uyuv_stride + j * 2 + 1] = y10;
            p_uyuv[uyuv_stride + j * 2 + 2] = v10;
            p_uyuv[uyuv_stride + j * 2 + 3] = y11;
        }
        p_uyuv += uyuv_stride * 2;
        p_yv12_y += yv12_stride * 2;
        p_yv12_u += yv12_stride / 2;
        p_yv12_v += yv12_stride / 2;
    }
}

// convert NV12 <-> ARGB32
void ColorConverter::ConvertNV12toARGB32(BYTE *p_nv12, int nv12_stride, BYTE *p_argb32, int argb32_stride, int width, int height)
{
    _MYASSERT(p_nv12);
    _MYASSERT(p_argb32);

    // NV12 is planar: Y plane, followed by packed U-V plane.

    BYTE *p_nv12_y = p_nv12;
    BYTE *p_nv12_uv = p_nv12 + nv12_stride * height;

    BYTE *p_argb32_scan = (BYTE *)p_argb32;

    for (int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j += 2)
        {
            int u = p_nv12_uv[j];
            int v = p_nv12_uv[j + 1];

            int y00 = p_nv12_y[j];
            int y01 = p_nv12_y[j + 1];
            int y10 = p_nv12_y[j + nv12_stride];
            int y11 = p_nv12_y[j + nv12_stride + 1];
            int u00 = u, u01 = u, u10 = u, u11 = u;
            int v00 = v, v01 = v, v10 = v, v11 = v;

            int r = 0, g = 0, b = 0, a = 255;

            DWORD *p_argb_line_0 = (DWORD *)(p_argb32_scan);
            DWORD *p_argb_line_1 = (DWORD *)(p_argb32_scan + argb32_stride);

            YUV2RGB(y00, u00, v00, r, g, b);
            p_argb_line_0[j + 0] = (DWORD)((a << 24) | (r << 16) | (g << 8) | (b));

            YUV2RGB(y01, u01, v01, r, g, b);
            p_argb_line_0[j + 1] = (DWORD)((a << 24) | (r << 16) | (g << 8) | (b));

            YUV2RGB(y10, u10, v10, r, g, b);
            p_argb_line_1[j + 0] = (DWORD)((a << 24) | (r << 16) | (g << 8) | (b));

            YUV2RGB(y11, u11, v11, r, g, b);
            p_argb_line_1[j + 1] = (DWORD)((a << 24) | (r << 16) | (g << 8) | (b));

        }
        p_nv12_y += nv12_stride * 2;
        p_nv12_uv += nv12_stride;
        p_argb32_scan += argb32_stride * 2;
    }
}

void ColorConverter::ConvertARGB32toNV12(BYTE *p_argb32, int argb32_stride, BYTE *p_nv12, int nv12_stride, int width, int height)
{
    _MYASSERT(p_nv12);
    _MYASSERT(p_argb32);

    // NV12 is planar: Y plane, followed by packed U-V plane.

    _MYASSERT(p_nv12);
    _MYASSERT(p_argb32);

    // NV12 is planar: Y plane, followed by packed U-V plane.

    BYTE *p_nv12_y = p_nv12;
    BYTE *p_nv12_uv = p_nv12 + nv12_stride * height;

    BYTE *p_argb32_scan = (BYTE *)p_argb32;

    for (int i = 0; i < height; i += 2)
    {
        for (int j = 0; j < width; j += 2)
        {
            int sum_u = 0;
            int sum_v = 0;

            int y = 0, u = 0, v = 0;
            int r = 0, g = 0, b = 0;
            
            // pixel (0, 0)
            r = p_argb32_scan[j * 4 + 2];
            g = p_argb32_scan[j * 4 + 1];
            b = p_argb32_scan[j * 4 + 0];

            RGB2YUV(r, g, b, y, u, v);
            p_nv12_y[j] = y;
            sum_u += u;
            sum_v += v;

            // pixel (0, 1)
            r = p_argb32_scan[j * 4 + 6];
            g = p_argb32_scan[j * 4 + 5];
            b = p_argb32_scan[j * 4 + 4];

            RGB2YUV(r, g, b, y, u, v);
            p_nv12_y[j + 1] = y;
            sum_u += u;
            sum_v += v;

            // pixel (1, 0)
            r = p_argb32_scan[argb32_stride + j * 4 + 2];
            g = p_argb32_scan[argb32_stride + j * 4 + 1];
            b = p_argb32_scan[argb32_stride + j * 4 + 0];

            RGB2YUV(r, g, b, y, u, v);
            p_nv12_y[nv12_stride + j] = y;
            sum_u += u;
            sum_v += v;

            // pixel (1, 1)
            r = p_argb32_scan[argb32_stride + j * 4 + 6];
            g = p_argb32_scan[argb32_stride + j * 4 + 5];
            b = p_argb32_scan[argb32_stride + j * 4 + 4];

            RGB2YUV(r, g, b, y, u, v);
            p_nv12_y[nv12_stride + j + 1] = y;
            sum_u += u;
            sum_v += v;

            p_nv12_uv[j + 0] = (sum_u + 2) >> 2;
            p_nv12_uv[j + 1] = (sum_v + 2) >> 2;    
        }
        p_nv12_y += nv12_stride * 2;
        p_nv12_uv += nv12_stride;
        p_argb32_scan += argb32_stride * 2;
    }
}