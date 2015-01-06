/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COLOR_CONVERTER_H_

#define COLOR_CONVERTER_H_

#include <sys/types.h>

#include <stdint.h>

#include <OMX_Video.h>

namespace android {

struct ColorConverter {
    ColorConverter(OMX_COLOR_FORMATTYPE from, OMX_COLOR_FORMATTYPE to);
    ~ColorConverter();

    bool isValid() const;

    void convert(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

private:
    OMX_COLOR_FORMATTYPE mSrcFormat, mDstFormat;
    uint8_t *mClip;

    uint8_t *initClip();

    void convertCbYCrY(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertYUV420Planar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertQCOMYUV420SemiPlanar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertYUV420SemiPlanar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    ColorConverter(const ColorConverter &);
    ColorConverter &operator=(const ColorConverter &);

#ifdef USE_SAMSUNG_COLORFORMAT
    /* test converter SeungBeom Kim */
    void convertSECNV12Tile(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);
    void convertNV12tToYUV420(
            uint8_t *pDst, uint8_t *pSrc,
            int32_t videoWidth, int32_t videoHeight);
    int tile_4x2_read(int x_size, int y_size, int x_pos, int y_pos);
    void Y_tile_to_linear_4x2(
            unsigned char *p_linear_addr, unsigned char *p_tiled_addr,
            unsigned int x_size, unsigned int y_size);
    void CbCr_tile_to_linear_4x2(
            unsigned char *p_linear_addr, unsigned char *p_tiled_addr,
            unsigned int x_size, unsigned int y_size);
#endif
};

#ifdef USE_SAMSUNG_COLORFORMAT
#ifdef __cplusplus
extern "C"
{
#endif
    static void tile_to_linear_64x32_4x2_neon(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size);
    static void tile_to_linear_64x32_4x2_uv_neon(unsigned char *p_linear_addr, unsigned char *p_tiled_addr, unsigned int x_size, unsigned int y_size);
#ifdef __cplusplus
}
#endif
#endif

}  // namespace android

#endif  // COLOR_CONVERTER_H_
