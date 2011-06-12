ARDeinformation.dll

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
********************************************************************************

REQUIREMENT:

*AviSynth2.6.0alpha3 or later
*SSE capable CPU
*msvcr100.dll


USAGE:

DAR_Padding(clip, float "dar_x", float "dar_y", int "align", int "color")

    Do padding to the clip in specified display aspect ratio.

    dar_x: numerater of DAR (default: width of clip).
    dar_y: denominator of DAR (default: height of clip).
    align: correct width of top/left border to be multipul of specified value(default: 16).
    color: color of borders (default: $000000(black)).


AR_Resize(clip, string "mode", float "ar_x", float "ar_y", bool "expand",
          float "src_left", float "src_top", float "src_right", float "src_bottom",
          string "resizer", float "ep0", float "ep1")

    Resize the clip in specified display/sample(pixel) aspect ratio.

    mode    : type of aspect ratio.
              acceptable values -- "dar", "sar" and "par" (default: "dar").
    ar_x    : numerater of AR (default: width of clip(dar) / 1(sar/par)).
    ar_y    : denominator of AR (default: height of clip(dar) / 1(sar/par)).
    expand  : Choice of expansion(true) or reduction(false) (default: true).
    src_left: same as internal resizers argument.
    src_top : same as internal resizers argument.
    src_right: same as internal resizers argument.
    src_bottom: same as internal resizers argument.
    resizer : type of resizer.
              acceptable values -- "Point", "Bilinear", "Bicubic", "Lanczos",
                                   "Lanczos4", "Blackman", "Spline16", "Spline36",
                                   "Spline64", "Sinc", "Gauss" (default: "Bicubic")
    ep0     : optional arguments for some resizers (e.g. Lanczos's "taps", Bicubic's "b")
    ep1     : optional argument for BicubicResize("c")


SOURCECODE:
https://github.com/chikuzen/ARDeformation
