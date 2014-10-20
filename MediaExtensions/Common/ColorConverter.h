#pragma once

class ColorConverter
{
public:
    ColorConverter();
    ~ColorConverter();
    
    // convert NV12 <-> YV12
    static void ConvertNV12toYV12(BYTE *p_nv12, int nv12_stride, BYTE *p_yv12, int yv12_stride, int width, int height);
    static void ConvertYV12toNV12(BYTE *p_yv12, int yv12_stride, BYTE *p_nv12, int nv12_stride, int width, int height);

    // convert YUY2 <-> YV12
    static void ConvertYUY2toYV12(BYTE *p_yuy2, int yuy2_stride, BYTE *p_yv12, int yv12_stride, int width, int height);
    static void ConvertYV12toYUY2(BYTE *p_yv12, int yv12_stride, BYTE *p_yuy2, int yuy2_stride, int width, int height);
    
    // convert UYVY <-> YV12
    static void ConvertUYVYtoYV12(BYTE *p_uyuv, int uyuv_stride, BYTE *p_yv12, int yv12_stride, int width, int height);
    static void ConvertYV12toUYVY(BYTE *p_yv12, int yv12_stride, BYTE *p_uyuv, int uyuv_stride, int width, int height);

    // convert NV12 <-> ARGB32
    static void ConvertNV12toARGB32(BYTE *p_nv12, int nv12_stride, BYTE *p_argb32, int argb32_stride, int width, int height);
    static void ConvertARGB32toNV12(BYTE *p_argb32, int argb32_stride, BYTE *p_nv12, int nv12_stride, int width, int height);
};