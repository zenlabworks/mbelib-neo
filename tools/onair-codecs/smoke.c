/* Windows x64 runtime smoke test for OnAir mbelib-neo codec plug-ins.
 * Verifies symbols, ABI, sample rate, creation, invalid input and close.
 * It is NOT a validation of real over-the-air speech decoding.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef int (__cdecl *abi_fn)(void);
typedef void *(__cdecl *open_fn)(void);
typedef void (__cdecl *close_fn)(void *);
typedef int (__cdecl *sample_rate_fn)(void);
typedef int (__cdecl *decode_fn)(void *, const uint8_t *, size_t, int16_t *, size_t);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: onair-codec-smoke plugin.dll\n");
        return 2;
    }
    HMODULE mod = LoadLibraryA(argv[1]);
    if (mod == NULL) {
        fprintf(stderr, "LoadLibrary failed for %s (Win32 error %lu)\n",
                argv[1], (unsigned long)GetLastError());
        return 3;
    }
    abi_fn abi = (abi_fn)GetProcAddress(mod, "onair_speech_abi");
    open_fn open_codec = (open_fn)GetProcAddress(mod, "onair_speech_open");
    close_fn close_codec = (close_fn)GetProcAddress(mod, "onair_speech_close");
    sample_rate_fn sample_rate = (sample_rate_fn)GetProcAddress(mod, "onair_speech_sample_rate");
    decode_fn decode = (decode_fn)GetProcAddress(mod, "onair_speech_decode");
    if (!abi || !open_codec || !close_codec || !sample_rate || !decode) {
        fprintf(stderr, "Missing OnAir ABI export in %s\n", argv[1]);
        FreeLibrary(mod);
        return 4;
    }
    if (abi() != 1 || sample_rate() != 8000) {
        fprintf(stderr, "ABI or sample rate mismatch in %s\n", argv[1]);
        FreeLibrary(mod);
        return 5;
    }
    void *state = open_codec();
    if (state == NULL) {
        fprintf(stderr, "Decoder initialization failed in %s\n", argv[1]);
        FreeLibrary(mod);
        return 6;
    }
    uint8_t dummy[1] = {0};
    int16_t pcm[160] = {0};
    if (decode(state, dummy, 0, pcm, 160) != -1) {
        fprintf(stderr, "Invalid frame was not rejected in %s\n", argv[1]);
        close_codec(state);
        FreeLibrary(mod);
        return 7;
    }
    /* TETRA: validate full-size decoded output and strict input handling. */
    if (strstr(argv[1], "tetra") != NULL) {
        uint8_t bits[137] = {0};
        int16_t tetra_pcm[240] = {0};
        if (decode(state, bits, 137, tetra_pcm, 239) != -1) {
            fprintf(stderr, "TETRA accepted undersized PCM buffer\n");
            close_codec(state);
            FreeLibrary(mod);
            return 8;
        }
        bits[38] = 2;
        if (decode(state, bits, 137, tetra_pcm, 240) != -1) {
            fprintf(stderr, "TETRA accepted invalid bit value\n");
            close_codec(state);
            FreeLibrary(mod);
            return 9;
        }
        bits[38] = 0;
        for (int k = 0; k < 3; k++) {
            if (decode(state, bits, 137, tetra_pcm, 240) != 240) {
                fprintf(stderr, "TETRA did not return 240 samples\n");
                close_codec(state);
                FreeLibrary(mod);
                return 10;
            }
        }
    }
    close_codec(state);
    FreeLibrary(mod);
    printf("PASS: %s ABI=1 rate=8000 open/close/invalid-frame\n", argv[1]);
    return 0;
}
