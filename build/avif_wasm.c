#include "avif/avif.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uint8_t *avif_encode_rgba_ex(
    uint8_t *rgba,
    int width,
    int height,
    int quality,
    int speed,
    int depth,
    int yuvFormat,
    int rangeFull,
    int aqMode,
    int enableRestoration,
    int enableCdef,
    int sharpness,
    int denoiseNoiseLevel,
    int tuneMode,
    int maxThreads,
    size_t *outSize
) {
    avifPixelFormat yuv = AVIF_PIXEL_FORMAT_YUV444;
    switch (yuvFormat) {
        case 0: yuv = AVIF_PIXEL_FORMAT_YUV400; break;
        case 1: yuv = AVIF_PIXEL_FORMAT_YUV420; break;
        case 2: yuv = AVIF_PIXEL_FORMAT_YUV422; break;
        case 3: yuv = AVIF_PIXEL_FORMAT_YUV444; break;
    }

    avifResult result;
    avifEncoder *enc = NULL;
    avifRGBImage rgb;
    avifRWData output = AVIF_DATA_EMPTY;
    uint8_t *encoded = NULL;

    avifImage *image = avifImageCreate(width, height, depth, yuv);
    if (!image) {
        return NULL;
    }
    image->yuvRange = rangeFull ? AVIF_RANGE_FULL : AVIF_RANGE_LIMITED;

    result = avifImageAllocatePlanes(image, AVIF_PLANES_YUV | AVIF_PLANES_A);
    if (result != AVIF_RESULT_OK) {
        goto cleanup;
    }

    avifRGBImageSetDefaults(&rgb, image);
    rgb.depth = 8;
    rgb.format = AVIF_RGB_FORMAT_RGBA;

    result = avifRGBImageAllocatePixels(&rgb);
    if (result != AVIF_RESULT_OK) {
        goto cleanup;
    }

    memcpy(rgb.pixels, rgba, (size_t)width * (size_t)height * 4);

    result = avifImageRGBToYUV(image, &rgb);
    if (result != AVIF_RESULT_OK) {
        goto cleanup;
    }

    enc = avifEncoderCreate();
    if (!enc) {
        goto cleanup;
    }

    enc->maxThreads = maxThreads;
    enc->quality = quality;
    enc->speed   = speed;

    char buf[32];

    // aq-mode
    snprintf(buf, sizeof(buf), "%d", aqMode);
    avifEncoderSetCodecSpecificOption(enc, "aq-mode", buf);

    // enable-restoration
    snprintf(buf, sizeof(buf), "%d", enableRestoration ? 1 : 0);
    avifEncoderSetCodecSpecificOption(enc, "enable-restoration", buf);

    // enable-cdef
    snprintf(buf, sizeof(buf), "%d", enableCdef ? 1 : 0);
    avifEncoderSetCodecSpecificOption(enc, "enable-cdef", buf);

    // sharpness
    snprintf(buf, sizeof(buf), "%d", sharpness);
    avifEncoderSetCodecSpecificOption(enc, "sharpness", buf);

    // denoise-noise-level
    snprintf(buf, sizeof(buf), "%d", denoiseNoiseLevel);
    avifEncoderSetCodecSpecificOption(enc, "denoise-noise-level", buf);

    // tune
    const char *tune = "ssim";
    switch (tuneMode) {
        case 0: tune = "psnr"; break;
        case 1: tune = "ssim"; break;
        case 2: tune = "iq";   break;
    }
    avifEncoderSetCodecSpecificOption(enc, "tune", tune);

    result = avifEncoderWrite(enc, image, &output);
    if (result != AVIF_RESULT_OK) {
        goto cleanup;
    }

    encoded = output.data;
    *outSize = output.size;
    // We intentionally DON'T free output here; JS will call avif_free_buffer(encoded)

cleanup:
    if (!encoded) {
        avifRWDataFree(&output);
    }
    if (enc) {
        avifEncoderDestroy(enc);
    }
    avifRGBImageFreePixels(&rgb);
    if (image) {
        avifImageDestroy(image);
    }
    return encoded;
}

void avif_free_buffer(uint8_t *ptr) {
    avifFree(ptr);
}