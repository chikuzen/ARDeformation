/********************************************************************************
    ARDeformation.dll

    Copyright (C) 2011 Oka Motofumi(chikuzen.mo at gmail dot com)

    author : Oka Motofumi

    Permission to use, copy, modify, and/or distribute this software for any
    purpose with or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
******************************************************************************/

#include <math.h>
#include <float.h>
#include "windows.h"
#include "avisynth26.h"

#pragma  warning(disable:4996)

class DAR_Padding : public GenericVideoFilter {

    PClip padded;

public:
    DAR_Padding(PClip _child, const float _dar_x, const float _dar_y,
                const int align, const int color, IScriptEnvironment* env);
    ~DAR_Padding() { }
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

class AR_Resize : public GenericVideoFilter {

    PClip resized;

public:
    AR_Resize(PClip _child, const char *mode, const float _ar_x, const float _ar_y,
              const bool expand, const float srcl, const float srct,
              const float srcr, const float srcb, const char *_resizer,
              const float ep0, const float ep1, const int dx, const int dy,
              IScriptEnvironment* env);
    ~AR_Resize() { }
    PVideoFrame __stdcall AR_Resize::GetFrame(int n, IScriptEnvironment* env);
};

DAR_Padding::
DAR_Padding(PClip _child, const float _dar_x, const float _dar_y, const int align,
            const int color, IScriptEnvironment* env) : GenericVideoFilter(_child)
{
    double dar_x = !_dar_x ? (double)vi.width : (double)_dar_x;
    double dar_y = !_dar_y ? (double)vi.height : (double)_dar_y;

    double flag = vi.width * dar_y - vi.height * dar_x;
    int dest_width  = flag < 0 ? (int)ceil(vi.height * dar_x / dar_y) : vi.width;
    int dest_height = flag > 0 ? (int)ceil(dest_width * dar_y / dar_x) : vi.height;

    int subsample_h = vi.SubsampleH();
    int subsample_v = vi.SubsampleV();

    dest_width += dest_width % subsample_h;
    dest_height += dest_height % subsample_v;

    int pad_left = (dest_width - vi.width) >> 1;
    pad_left -= (pad_left % subsample_h);
    while ((pad_left % align) && (pad_left > 0))
        pad_left -= subsample_h;

    int pad_right = dest_width - (vi.width + pad_left);

    int pad_top = (dest_height - vi.height) >> 1;
    pad_top -= (pad_top % subsample_v);
    while ((pad_top % align) && (pad_top > 0))
        pad_top -= subsample_v;

    int pad_bottom = dest_height - (vi.height + pad_top);

    vi.width = dest_width;
    vi.height = dest_height;

    try {
        AVSValue padargs[6] = {child, pad_left, pad_top, pad_right, pad_bottom, color};
        padded = env->Invoke("AddBorders", AVSValue(padargs, 6)).AsClip();
    } catch (IScriptEnvironment::NotFound) {
        env->ThrowError("DAR_Padding: Couldn't Invoke AddBorders.");
    }
}

PVideoFrame __stdcall DAR_Padding::
GetFrame(int n, IScriptEnvironment* env)
{
    return padded->GetFrame(n, env);
}

AR_Resize::
AR_Resize(PClip _child, const char *mode, const float _ar_x, const float _ar_y,
          const bool expand, const float srcl, const float srct, const float srcr,
          const float srcb, const char *_resizer, const float ep0, const float ep1,
          const int dx, const int dy, IScriptEnvironment* env) : GenericVideoFilter(_child)
{
    double dest_width, dest_height;
    dest_width = !srcr ? (double)vi.width :
                 srcr > 0 ? (double)srcr :
                 fabs((double)vi.width - srcl + srcr);

    dest_height = !srcb ? vi.height :
                  srcb > 0 ? ceil(srcb) :
                  fabs((double)vi.height - srct + srcb);

    double ar_x, ar_y;
    if (!stricmp(mode, "dar")) {
        ar_x = !_ar_x ? (double)vi.width : (double)_ar_x;
        ar_y = !_ar_y ? (double)vi.height : (double)_ar_y;
        double flag = dest_width * ar_y - dest_height * ar_x;
        if ((flag > 0  && expand) || (flag < 0 && !expand))
            dest_height = dest_width * ar_y / ar_x;
        else if (flag)
            dest_width = dest_height * ar_x / ar_y;
    } else { // par or sar
        ar_x = !_ar_x ? 1.0 : (double)_ar_x;
        ar_y = !_ar_y ? 1.0 : (double)_ar_y;
        if (((ar_x > ar_y) && expand) || ((ar_x < ar_y) && !expand))
            dest_width = dest_width * (ar_x / ar_y);
        else if (ar_x != ar_y)
            dest_height = dest_height * (ar_y / ar_x);
    }

    if (dx) {
        dest_height *= dx / dest_width;
        dest_width = (double)dx;
    } else if (dy) {
        dest_width *= dy / dest_height;
        dest_height = (double)dy;
    }

    dest_width = ceil(dest_width) + (int)ceil(dest_width) % vi.SubsampleH();
    dest_height = ceil(dest_height) + (int)ceil(dest_height) % vi.SubsampleV();

    vi.width = (int)dest_width;
    vi.height = (int)dest_height;

    typedef struct {
        char name[32];
        int len_name;
        int argstype;
        float ex0;
        float ex1;
    } resize_method;

    resize_method resizer = {};
    resize_method methods[] = {
        {"PointResize",    5, 1,  0, 0},
        {"BilinearResize", 8, 1,  0, 0},
        {"LanczosResize",  7, 2,  3, 0},
        {"Lanczos4Resize", 8, 1,  0, 0},
        {"Spline16Resize", 8, 1,  0, 0},
        {"Spline36Resize", 8, 1,  0, 0},
        {"Spline64Resize", 8, 1,  0, 0},
        {"BlackmanResize", 8, 2,  4, 0},
        {"SincResize",     4, 2,  4, 0},
        {"GaussResize",    5, 2, 30, 0},
        {"BicubicResize",       7, 3, 1.0 / 3, 1.0 / 3},
        {"Mitchell-Netravali", 18, 3, 1.0 / 3, 1.0 / 3},
        {"Catmull-Rom",        11, 3,     0.0,     0.5},
        {"Hermite",             7, 3,     0.0,     0.0},
        {"Robidoux",            8, 3,  0.3782,  0.3109},
        {"SoftCubic50",        11, 3,     0.5,     0.5},
        {"SoftCubic75",        11, 3,    0.75,    0.25},
        {"SoftCubic100",       12, 3,     1.0,     0.0},
        {"", 0}
    };
    for (int i = 0; methods[i].len_name; i++) {
        if (!strnicmp(methods[i].name, _resizer, methods[i].len_name))
            resizer = methods[i];
    }

    switch (resizer.argstype) {
        case 1:
            try {
                AVSValue resizeargs[]= {child, (int)dest_width, (int)dest_height,
                                        srcl, srct, srcr, srcb};
                resized = env->Invoke(resizer.name, AVSValue(resizeargs, 7)).AsClip();
            } catch (IScriptEnvironment::NotFound) {
                env->ThrowError("AR_Resize: Couldn't Invoke %s.", resizer.name);
            }
            break;
        case 2:
            resizer.ex0 = ep0 == -FLT_MAX ? resizer.ex0 : ep0;
            try {
                AVSValue resizeargs[] = {child, (int)dest_width, (int)dest_height,
                                         srcl, srct, srcr, srcb, (int)resizer.ex0};
                resized = env->Invoke(resizer.name, AVSValue(resizeargs, 8)).AsClip();
            } catch (IScriptEnvironment::NotFound) {
                env->ThrowError("AR_Resize: Couldn't Invoke %s.", resizer.name);
            }
            break;
        case 3:
            resizer.ex0 = ep0 == -FLT_MAX ? resizer.ex0 : ep0;
            resizer.ex1 = ep1 == -FLT_MAX ? resizer.ex1 : ep1;
            try {
                AVSValue resizeargs[] = {child, (int)dest_width, (int)dest_height,
                                         resizer.ex0, resizer.ex1,
                                         srcl, srct, srcr, srcb};
                resized = env->Invoke("BicubicResize", AVSValue(resizeargs, 9)).AsClip();
            } catch (IScriptEnvironment::NotFound) {
                env->ThrowError("AR_Resize: Couldn't Invoke %s.", resizer.name);
            }
            break;
        default:
            env->ThrowError("AR_Resize: Invalid argument \"%s\"", _resizer);
            break;
    }
}

PVideoFrame __stdcall AR_Resize::GetFrame(int n, IScriptEnvironment* env)
{
    return resized->GetFrame(n, env);
}

AVSValue __cdecl Create_DAR_Padding(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    const float dx = args[1].AsFloat(0.0);
    const float dy = args[2].AsFloat(0.0);
    const int align = args[3].AsInt(16);
    const int cl = args[4].AsInt(0);

    if (dx < 0)
        env->ThrowError("DAR_Padding: invalid argument \"dar_x\"");
    if (dy < 0)
        env->ThrowError("DAR_Padding: invalid argument \"dar_y\"");
    if (align < 1)
        env->ThrowError("DAR_Padding: \"align\" needs to be 1 or higher integer.");

    return new DAR_Padding(args[0].AsClip(), dx, dy, align, cl, env);
}

AVSValue __cdecl Create_AR_Resize(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    const float ax = args[1].AsFloat(0.0);
    const float ay = args[2].AsFloat(0.0);
    const float src_l = args[3].AsFloat(0.0);
    const float src_t = args[4].AsFloat(0.0);
    const float src_r = args[5].AsFloat(0.0);
    const float src_b = args[6].AsFloat(0.0);
    const float ep0 = args[7].IsFloat() ? args[10].AsFloat() : -FLT_MAX;
    const float ep1 = args[8].IsFloat() ? args[11].AsFloat() : -FLT_MAX;
    const int dx = args[9].AsInt(0);
    const int dy = args[10].AsInt(0);
    const char *resizer = args[11].AsString("Bicubic");
    const bool expand = args[12].AsBool(true);
    const char *mode = args[13].AsString("dar");

    if (stricmp(mode, "dar") && stricmp(mode, "par") && stricmp(mode, "sar"))
        env->ThrowError("ARResize: Invalid argument \"mode\".");
    if (ax < 0 || ay < 0)
        env->ThrowError("ARResize: \"ar_x\" and \"ar_y\" needs to be 0 or higher values.");
    if (dx < 0 || dy < 0)
        env->ThrowError("ARResize: \"dest_w\" and \"dest_h\" needs to be 0 or higher integers.");

    return new AR_Resize(args[0].AsClip(), mode, ax, ay, expand, src_l, src_t,
                         src_r, src_b, resizer, ep0, ep1, dx, dy, env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("DARPadding", "c[dar_x]f[dar_y]f[align]i[color]i", Create_DAR_Padding, 0);
    env->AddFunction("ARResize", "c[ar_x]f[ar_y]f[src_right]f[src_top]f[src_left]f[src_bottom]f"
                     "[ep0]f[ep1]f[dest_w]i[dest_h]i[resizer]s[expand]b[mode]s", Create_AR_Resize, 0);
    return 0;
}
