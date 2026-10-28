/* SPDX-License-Identifier: GPL-2.0-or-later
 * OnAir TETRA full-rate speech plug-in for mbelib-neo 2.x.
 * OnAir provides B1..B137 as 137 unpacked bytes, one bit per byte.
 * This plug-in returns 240 signed 16-bit PCM samples per 30 ms frame at 8 kHz.
 * Only unencrypted frames reach the plug-in; it performs no decryption.
 */
#include <mbelib-neo/mbelib.h>
#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
#define ONAIR_EXPORT __declspec(dllexport)
#else
#define ONAIR_EXPORT __attribute__((visibility("default")))
#endif

ONAIR_EXPORT int onair_speech_abi(void) {
    return 1;
}

ONAIR_EXPORT void *onair_speech_open(void) {
    return mbe_tetraDecoderAlloc();
}

ONAIR_EXPORT void onair_speech_close(void *state) {
    mbe_tetraDecoderFree((mbe_tetra_decoder *)state);
}

ONAIR_EXPORT int onair_speech_sample_rate(void) {
    return 8000;
}

ONAIR_EXPORT int onair_speech_decode(void *state,
                                     const uint8_t *coded,
                                     size_t nbytes,
                                     int16_t *pcm,
                                     size_t max_samples) {
    if (state == NULL || coded == NULL || pcm == NULL ||
        nbytes != MBE_TETRA_FRAME_BITS ||
        max_samples < MBE_TETRA_FRAME_SAMPLES) {
        return -1;
    }
    /* Invalid 0/1 input is rejected by mbelib-neo without changing state. */
    int status = mbe_tetraDecodeFrame((mbe_tetra_decoder *)state,
                                      coded, 0, pcm);
    return status == 0 ? MBE_TETRA_FRAME_SAMPLES : -1;
}
