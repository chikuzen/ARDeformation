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
    DAR_Padding(PClip _child, const int _dar_x, const int _dar_Y,
                const int _color, IScriptEnvironment* env);
    ~DAR_Padding() { }
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

class AR_Resize : public GenericVideoFilter {

    PClip resized;

public:
    AR_Resize(PClip _child, const char *mode, const int _ar_x, const int _ar_y,
              const bool expand, const float srcl, const float srct,
              const float srcr, const float srcb, const char *_resizer,
              const float ep0, const float ep1, IScriptEnvironment* env);
    ~AR_Resize() { }
    PVideoFrame __stdcall AR_Resize::GetFrame(int n, IScriptEnvironment* env);
};

DAR_Padding::
DAR_Padding(PClip _child, const int _dar_x, const int _dar_y, const int _color,
            IScriptEnvironment* env) : GenericVideoFilter(_child)
{
    int dar_x = !_dar_x ? vi.width : _dar_x;
    int dar_y = !_dar_y ? vi.height : _dar_y;
    int color = !_color ? 0x000000 : _color;

    int dest_width = vi.width;
    int dest_height = vi.height;

    int subsample_h = vi.SubsampleH();
    int subsample_v = vi.SubsampleV();

    if (((double)dest_width / dest_height) > ((double)dar_x / dar_y))
        dest_height = (int)ceil((double)dest_width * dar_y / dar_x);
    else if (((double)dest_width / dest_height) < ((double)dar_x / dar_y)) 
        dest_width = (int)ceil((double)dest_height * dar_x / dar_y);

    dest_width += dest_width % subsample_h;
    dest_height += dest_height % subsample_v;

    int pad_left   = ((dest_width - vi.width) >> 1)
                    - (((dest_width - vi.width) >> 1) % subsample_h);
    int pad_right  = dest_width - (vi.width + pad_left);
    int pad_top    = ((dest_height - vi.height) >> 1)
                    - (((dest_height - vi.height) >> 1) % subsample_v);
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
AR_Resize(PClip _child, const char *mode, const int _ar_x, const int _ar_y,
          const bool expand, const float srcl, const float srct, const float srcr,
          const float srcb, const char *_resizer, const float ep0, const float ep1,
          IScriptEnvironment* env) : GenericVideoFilter(_child)
{
    int dest_width = srcr == 0.0 ? vi.width :
                     srcr > 0 ? (int)ceil(srcr) :
                     abs(vi.width - (int)ceil(srcl - srcr));

    int dest_height = srcb == 0.0 ? vi.height :
                      srcb > 0 ? (int)ceil(srcb) :
                      abs(vi.height - (int)ceil(srct - srcb));

    int ar_x, ar_y;

    if (!stricmp(mode, "dar")) {
        ar_x = !_ar_x ? vi.width : _ar_x;
        ar_y = !_ar_y ? vi.height : _ar_y;
        if (((((double)dest_width / dest_height) > ((double)ar_x / ar_y)) && expand) ||
            ((((double)dest_width / dest_height) < ((double)ar_x / ar_y)) && !expand))
            dest_height = (int)ceil((double)dest_width * ar_y / ar_x);
        else if (((double)dest_width / dest_height) != ((double)ar_x / ar_y))
            dest_width = (int)ceil((double)dest_height * ar_x / ar_y);
    } else { // par or sar
        ar_x = !_ar_x ? 1 : _ar_x;
        ar_y = !_ar_y ? 1 : _ar_y;
        if (((ar_x > ar_y) && expand) || ((ar_x < ar_y) && !expand))
            dest_width = (int)ceil(dest_width * ((double)ar_x / ar_y));
        else if (ar_x != ar_y)
            dest_height = (int)ceil(dest_height * ((double)ar_y / ar_x));
    }

    dest_width += dest_width % vi.SubsampleH();
    dest_height += dest_height % vi.SubsampleV();

    vi.width = dest_width;
    vi.height = dest_height;

    typedef struct {
        char name[32];
        int len_name;
        int argstype;
        float ex0;
        float ex1;
    } resize_method;

    resize_method resizer = {};
    resize_method method[] = {
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
    };
    for (int i = 0; i < 15; i++) {
        if (!strnicmp(method[i].name, _resizer, method[i].len_name))
            resizer = method[i];
    }

    switch (resizer.argstype) {
        case 1:
            try {
                AVSValue resizeargs[] = {child, dest_width, dest_height, srcl, srct, srcr, srcb};
                resized = env->Invoke(resizer.name, AVSValue(resizeargs, 7)).AsClip();
            } catch (IScriptEnvironment::NotFound) {
                env->ThrowError("AR_Resize: Couldn't Invoke %s.", resizer.name);
            }
            break;
        case 2:
            resizer.ex0 = ep0 == -FLT_MAX ? resizer.ex0 : ep0;
            try {
                AVSValue resizeargs[] = {child, dest_width, dest_height,
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
                AVSValue resizeargs[] = {child, dest_width, dest_height,
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
    const int dx = args[1].AsInt(0);
    const int dy = args[2].AsInt(0);
    const int cl = args[3].AsInt(0);

    if (dx < 0)
        env->ThrowError("DAR_Padding: invalid argument \"dar_x\"");
    if (dy < 0)
        env->ThrowError("DAR_Padding: invalid argument \"dar_y\"");
    if (cl < 0 || cl > 0xffffff)
        env->ThrowError("DAR_Padding: invalid argument \"color\"");

    return new DAR_Padding(args[0].AsClip(), dx, dy, cl, env);
}

AVSValue __cdecl Create_AR_Resize(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    const char *mode = args[1].AsString("dar");
    const int ax = args[2].AsInt(0);
    const int ay = args[3].AsInt(0);
    const bool expand = args[4].AsBool(true);
    const float src_l = args[5].AsFloat(0.0);
    const float src_t = args[6].AsFloat(0.0);
    const float src_r = args[7].AsFloat(0.0);
    const float src_b = args[8].AsFloat(0.0);
    const char *resizer = args[9].AsString("Bicubic");
    const float ep0 = args[10].IsFloat() ? args[10].AsFloat() : -FLT_MAX;
    const float ep1 = args[11].IsFloat() ? args[11].AsFloat() : -FLT_MAX;

    if (stricmp(mode, "dar") && stricmp(mode, "par") && stricmp(mode, "sar"))
        env->ThrowError("AR_Resize: Invalid arguments \"mode\".");
    if (ax < 0)
        env->ThrowError("AR_Resize: Invalid arguments \"ar_x\".");
    if (ay < 0)
        env->ThrowError("AR_Resize: Invalid arguments \"ar_y\".");

    return new AR_Resize(args[0].AsClip(), mode, ax, ay, expand, src_l, src_t,
                         src_r, src_b, resizer, ep0, ep1, env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("DAR_Padding", "c[dar_x]i[dar_y]i[color]i", Create_DAR_Padding, 0);
    env->AddFunction("AR_Resize", "c[mode]s[ar_x]i[ar_y]i[expand]b[src_right]f[src_top]f"
                     "[src_left]f[src_bottom]f[resizer]s[ep0]f[ep1]f", Create_AR_Resize, 0);
    return 0;
}
