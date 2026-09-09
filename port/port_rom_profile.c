#include "port_rom_profile.h"

#include "port_config.h"

#include <stdio.h>
#include <string.h>

#define RETAIL_ROM_SIZE 0x01000000u
#define RETAIL_TEXT_REMAP_SIZE 346u
#define ANGEL_TEXT_REMAP_SIZE 0xA0u

static const PortRomProfile kProfiles[] = {
    {PORT_ROM_VARIANT_USA_RETAIL, ROM_REGION_USA, "usa-retail", "USA retail", "BZME",
     "b4bd50e4131b027c334547b4524e2dbbd4227130",
     "bedc74df62755f705398273de8ed3bc59be610cf55760d0b9aa277f1f5035e73",
     RETAIL_ROM_SIZE, 1, PORT_TEXT_CODEC_RETAIL, 0, 0, RETAIL_TEXT_REMAP_SIZE, 9, 5, 7, NULL},
    {PORT_ROM_VARIANT_EU_RETAIL, ROM_REGION_EU, "eu-retail", "Europe retail", "BZMP",
     "cff199b36ff173fb6faf152653d1bccf87c26fb7",
     "c84645731952b7677f514ae222683504066334ab2af904e64a8a84ffc1af46c6",
     RETAIL_ROM_SIZE, 1, PORT_TEXT_CODEC_RETAIL, 0, 0, RETAIL_TEXT_REMAP_SIZE, 9, 5, 7, NULL},
    {PORT_ROM_VARIANT_JP_RETAIL, ROM_REGION_JP, "jp-retail", "Japan retail", "BZMJ",
     "6c5404a1effb17f481f352181d0f1c61a2765c5d",
     "16ac2572ba17e9cb2a70093d41f97ef8cff66c56417e45ea98adacdc51bb4b38",
     RETAIL_ROM_SIZE, 1, PORT_TEXT_CODEC_RETAIL, 0, 0, RETAIL_TEXT_REMAP_SIZE, 9, 5, 7, NULL},
    {PORT_ROM_VARIANT_JP_ANGEL_SP4, ROM_REGION_JP, "jp-angel-sp4-zh-cn", "Japan Angel Team Chinese SP4", "BZMJ",
     "ba04cfbe93d12d2ad684c52234472fa12a5b53d7",
     "f51c6c2f90e18ee91203dd767307271e06901b5bff35c3a567d52f61a39d166d",
     RETAIL_ROM_SIZE, 1, PORT_TEXT_CODEC_ANGEL_SP4, 0x00DC9F00u, 0x00E4F000u, ANGEL_TEXT_REMAP_SIZE, 16, 3, 3,
     "tmc_jp_angel_sp4.sav"},
    {PORT_ROM_VARIANT_USA_DEMO, ROM_REGION_USA, "usa-demo", "USA demo", "BZHE",
     "63fcad218f9047b6a9edbb68c98bd0dec322d7a1", "", 0, 0, PORT_TEXT_CODEC_RETAIL, 0, 0, RETAIL_TEXT_REMAP_SIZE, 9, 5,
     7, NULL},
    {PORT_ROM_VARIANT_JP_DEMO, ROM_REGION_JP, "jp-demo", "Japan demo", "BZMJ",
     "9cdb56fa79bba13158b81925c1f3641251326412", "", 0, 0, PORT_TEXT_CODEC_RETAIL, 0, 0, RETAIL_TEXT_REMAP_SIZE, 9, 5,
     7, NULL},
};

static const PortRomProfile* sActiveProfile;

static int HashMatches(const char* expected, const char* actual) {
    return expected == NULL || expected[0] == '\0' || strcmp(expected, actual) == 0;
}

const PortRomProfile* Port_IdentifyRomHashes(const PortRomHashes* hashes, const char gameCode[4]) {
    if (hashes == NULL) return NULL;
    for (size_t i = 0; i < sizeof(kProfiles) / sizeof(kProfiles[0]); ++i) {
        const PortRomProfile* profile = &kProfiles[i];
        if (profile->expectedSize != 0 && hashes->size != profile->expectedSize) continue;
        if (gameCode != NULL && memcmp(gameCode, profile->gameCode, 4) != 0) continue;
        if (HashMatches(profile->sha1, hashes->sha1) && HashMatches(profile->sha256, hashes->sha256)) return profile;
    }
    return NULL;
}

const PortRomProfile* Port_IdentifyRomBuffer(const void* data, size_t size, PortRomHashes* hashesOut) {
    PortRomHashes hashes = {0};
    char gameCode[4] = {0};
    if (hashesOut != NULL) memset(hashesOut, 0, sizeof(*hashesOut));
    if (data == NULL) return NULL;
    Port_HashBuffer(data, size, &hashes);
    if (size >= 0xB0u) memcpy(gameCode, (const uint8_t*)data + 0xACu, sizeof(gameCode));
    if (hashesOut != NULL) *hashesOut = hashes;
    return Port_IdentifyRomHashes(&hashes, size >= 0xB0u ? gameCode : NULL);
}

const PortRomProfile* Port_IdentifyRomFile(const char* path, PortRomHashes* hashesOut) {
    PortRomHashes hashes = {0};
    char gameCode[4];
    FILE* file;
    const PortRomProfile* profile;
    if (hashesOut != NULL) memset(hashesOut, 0, sizeof(*hashesOut));
    if (!Port_HashFile(path, &hashes)) return NULL;
    file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0xAC, SEEK_SET) != 0 || fread(gameCode, 1, sizeof(gameCode), file) != sizeof(gameCode)) {
        if (file != NULL) fclose(file);
        return NULL;
    }
    fclose(file);
    if (hashesOut != NULL) *hashesOut = hashes;
    profile = Port_IdentifyRomHashes(&hashes, gameCode);
    return profile;
}

int Port_RomProfileIsPlayable(const PortRomProfile* profile) { return profile != NULL && profile->playable; }
void Port_SetActiveRomProfile(const PortRomProfile* profile) { sActiveProfile = profile; }
const PortRomProfile* Port_GetActiveRomProfile(void) { return sActiveProfile; }
PortRomVariant Port_GetRomVariant(void) { return sActiveProfile ? sActiveProfile->variant : PORT_ROM_VARIANT_UNKNOWN; }
PortTextCodec Port_GetTextCodec(void) { return sActiveProfile ? sActiveProfile->textCodec : PORT_TEXT_CODEC_RETAIL; }
uint32_t Port_GetGlyphBankCount(void) { return sActiveProfile ? sActiveProfile->glyphBankCount : 9u; }
uint32_t Port_GetWideGlyphFirstBank(void) { return sActiveProfile ? sActiveProfile->wideGlyphFirstBank : 5u; }
uint32_t Port_GetSpecialPaletteBank(void) { return sActiveProfile ? sActiveProfile->specialPaletteBank : 7u; }
uint32_t Port_GetGlyphTableOffset(uint32_t regionOffset) {
    return sActiveProfile && sActiveProfile->glyphTableOffset ? sActiveProfile->glyphTableOffset : regionOffset;
}
uint32_t Port_GetTextRemapOffset(uint32_t regionOffset) {
    return sActiveProfile && sActiveProfile->textRemapOffset ? sActiveProfile->textRemapOffset : regionOffset;
}
uint32_t Port_GetTextRemapSize(void) { return sActiveProfile ? sActiveProfile->textRemapSize : RETAIL_TEXT_REMAP_SIZE; }
const char* Port_GetVariantSaveFilename(void) { return sActiveProfile ? sActiveProfile->saveFilename : NULL; }
const char* Port_GetAssetCacheSubdir(void) {
    switch (Port_GetRomVariant()) {
        case PORT_ROM_VARIANT_EU_RETAIL: return "eu";
        case PORT_ROM_VARIANT_JP_RETAIL: return "jp";
        case PORT_ROM_VARIANT_JP_ANGEL_SP4: return "jp-angel-sp4";
        case PORT_ROM_VARIANT_USA_DEMO: return "usa-demo";
        case PORT_ROM_VARIANT_JP_DEMO: return "jp-demo";
        default: return "usa";
    }
}
