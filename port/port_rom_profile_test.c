#include "port_rom_profile.h"
#include "port_config.h"

#include <stdio.h>
#include <string.h>

static int sFailures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); ++sFailures; } } while (0)

static void SetHashes(PortRomHashes* h, size_t size, const char* sha1, const char* sha256) {
    memset(h, 0, sizeof(*h));
    h->size = size;
    snprintf(h->sha1, sizeof(h->sha1), "%s", sha1);
    snprintf(h->sha256, sizeof(h->sha256), "%s", sha256);
}

int main(void) {
    PortRomHashes hashes;
    const PortRomProfile* profile;

    SetHashes(&hashes, 0x1000000u,
              "ba04cfbe93d12d2ad684c52234472fa12a5b53d7",
              "f51c6c2f90e18ee91203dd767307271e06901b5bff35c3a567d52f61a39d166d");
    profile = Port_IdentifyRomHashes(&hashes, "BZMJ");
    CHECK(profile != NULL);
    CHECK(profile != NULL && profile->variant == PORT_ROM_VARIANT_JP_ANGEL_SP4);
    CHECK(profile != NULL && profile->textCodec == PORT_TEXT_CODEC_ANGEL_SP4);
    CHECK(profile != NULL && profile->glyphTableOffset == 0xDC9F00u);
    CHECK(profile != NULL && profile->textRemapOffset == 0xE4F000u);
    CHECK(profile != NULL && profile->textRemapSize == 0xA0u);
    CHECK(profile != NULL && profile->glyphBankCount == 16u);
    CHECK(profile != NULL && profile->wideGlyphFirstBank == 3u);
    CHECK(profile != NULL && profile->specialPaletteBank == 3u);

    Port_SetActiveRomProfile(profile);
    CHECK(Port_GetTextCodec() == PORT_TEXT_CODEC_ANGEL_SP4);
    CHECK(Port_GetGlyphBankCount() == 16u);
    CHECK(Port_GetWideGlyphFirstBank() == 3u);
    CHECK(Port_GetSpecialPaletteBank() == 3u);
    CHECK(strcmp(Port_GetVariantSaveFilename(), "tmc_jp_angel_sp4.sav") == 0);
    CHECK(strcmp(Port_GetAssetCacheSubdir(), "jp-angel-sp4") == 0);

    hashes.sha256[0] = '0';
    CHECK(Port_IdentifyRomHashes(&hashes, "BZMJ") == NULL);

    SetHashes(&hashes, 0x1000000u,
              "6c5404a1effb17f481f352181d0f1c61a2765c5d",
              "16ac2572ba17e9cb2a70093d41f97ef8cff66c56417e45ea98adacdc51bb4b38");
    profile = Port_IdentifyRomHashes(&hashes, "BZMJ");
    CHECK(profile != NULL && profile->variant == PORT_ROM_VARIANT_JP_RETAIL);
    CHECK(profile != NULL && profile->textCodec == PORT_TEXT_CODEC_RETAIL);
    CHECK(profile != NULL && profile->glyphBankCount == 9u);

    Port_SetActiveRomProfile(NULL);
    CHECK(Port_GetTextCodec() == PORT_TEXT_CODEC_RETAIL);
    CHECK(Port_GetGlyphBankCount() == 9u);

    if (sFailures) {
        fprintf(stderr, "rom_profile_test: %d failure(s)\n", sFailures);
        return 1;
    }
    puts("rom_profile_test: ALL PASS");
    return 0;
}
