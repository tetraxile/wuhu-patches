#include "pe/Hacks/FSHacks.h"
#include "flips/libbps.h"
#include "heap/seadExpHeap.h"
#include "heap/seadHeapMgr.h"
#include "hk/Result.h"
#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"
#include "hk/ro/RoUtil.h"
#include "hk/util/hash.h"
#include "nn/fs.h"
#include <sead/prim/seadEndian.h>
#include <sead/resource/seadResourceMgr.h>

static sead::ExpHeap* sPatchHeap = nullptr;

namespace pe {

    static hk_noinline bool isFileExist(const char* path) {
        nn::fs::DirectoryEntryType type;
        nn::fs::GetEntryType(&type, path);

        return type == nn::fs::DirectoryEntryType_File;
    }

    hk::Result writeFileToPath(void* buf, size_t size, const char* path) {
        static nn::fs::FileHandle handle;

        if (isFileExist(path))
            nn::fs::DeleteFile(path);

        HK_TRY(nn::fs::CreateFile(path, size).GetInnerValueForDebug());
        HK_TRY(nn::fs::OpenFile(&handle, path, nn::fs::OpenMode_Write).GetInnerValueForDebug());
        HK_TRY(nn::fs::WriteFile(handle, 0, buf, size, nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush)).GetInnerValueForDebug());
        nn::fs::CloseFile(handle);

        return hk::ResultSuccess();
    }

    constexpr int cMaxPathLength = nn::fs::PathLengthMax * 2 + 1;
    using PathStr = sead::FixedSafeString<cMaxPathLength>;
    using PathStrFormat = sead::FormatFixedSafeString<cMaxPathLength>;

#define YAZ0 0
#define ZSTD 1

#define COMPRESSION YAZ0
#define CACHE_DIR "sd:/WuhuKingdom/cache/"
#define ROMFS_MOUNT "content:"

    constexpr char cBaseRomFsMount[] = ROMFS_MOUNT;
    constexpr char cBaseRomFsDir[] = ROMFS_MOUNT "/";
    constexpr char cCacheDir[] = CACHE_DIR;
    constexpr char cCacheHashFile[] = CACHE_DIR "patch.hash";
    constexpr bool cIsZstd = COMPRESSION;
    constexpr char cCompressionExt[] =
#if COMPRESSION == YAZ0
        ".szs"
#elif COMPRESSION == ZSTD
        ".zs"
#endif
        ;

    static PathStr sVersionExt("");

}

#if COMPRESSION == ZSTD
#include "zstd.h"
#else
s32 decodeSZSNxAsm64_(void* dst, const void* src);
#endif

namespace pe {
    HkTrampoline<nn::Result, nn::fs::FileHandle*, const char*, int>
        openFileHook = hk::hook::trampoline([](nn::fs::FileHandle* out, const char* filePath, int mode) -> nn::Result {
            if (std::strncmp(filePath, cBaseRomFsDir, 9) == 0) {
                char path[cMaxPathLength] { 0 };
                strcat(path, cCacheDir);
                strcat(path, filePath + 9);
                if (isFileExist(path))
                    return openFileHook.orig(out, path, mode);
            }

            return openFileHook.orig(out, filePath, mode);
        });

    HkTrampoline<sead::Resource*, sead::ResourceMgr*, const sead::ResourceMgr::LoadArg&,
#if not SEAD_RESOURCEMGR_TRYCREATE_NO_FACTORY_NAME
        const sead::SafeString&,
#endif
        sead::Decompressor*>
        loadResourceHook = hk::hook::trampoline([](sead::ResourceMgr* thisPtr, const sead::ResourceMgr::LoadArg& arg,
#if not SEAD_RESOURCEMGR_TRYCREATE_NO_FACTORY_NAME
                                                    const sead::SafeString& factory_name,
#endif
                                                    sead::Decompressor* decompressor) -> sead::Resource* {
            if (arg.path.endsWith(cCompressionExt)) {
                PathStr decompressedPath(arg.path);
                decompressedPath.removeSuffix(cCompressionExt);

                bool cacheExists = [&]() -> bool {
                    PathStr cachePath(cCacheDir);
                    cachePath.append(decompressedPath);
                    return isFileExist(cachePath.cstr());
                }();

                if (cacheExists) {
                    auto decompressedArg = arg;
                    decompressedArg.path = decompressedPath;
                    return thisPtr->tryLoadWithoutDecomp(decompressedArg);
                }
            }
            return loadResourceHook.orig(thisPtr, arg,
#if not SEAD_RESOURCEMGR_TRYCREATE_NO_FACTORY_NAME
                factory_name,
#endif
                decompressor);
        });

    void installFSHacks() {
        loadResourceHook.installAtPtr(pun<void*>(&sead::ResourceMgr::tryLoad));
    }

    static mem patchFileData(mem patch, mem source, const char* bpsPath) {
        mem patchedFile;
        bpserror error = bps_apply(patch, source, &patchedFile, nullptr, false);
        HK_ABORT_UNLESS(error == bpserror::bps_ok, "BPS patching failed with bpserror: %d (%s)", error, bpsPath);
        return patchedFile;
    }

    static void patchFile(const char* bpsPath, const char* originalPath, const char* outPath) {
        HK_ABORT_UNLESS(isFileExist(originalPath), "Patch %s exists, but source file %s does not!", bpsPath, originalPath);

        nn::fs::FileHandle patchFile;
        HK_ASSERT(nn::fs::OpenFile(&patchFile, bpsPath, nn::fs::OpenMode_Read).IsSuccess());
        s64 patchSize = 0;
        HK_ASSERT(nn::fs::GetFileSize(&patchSize, patchFile).IsSuccess());
        u8* patchData = (u8*)sPatchHeap->alloc(patchSize);
        nn::fs::ReadFile(patchFile, 0, patchData, patchSize);
        nn::fs::CloseFile(patchFile);

        nn::fs::FileHandle sourceFile;
        HK_ASSERT(nn::fs::OpenFile(&sourceFile, originalPath, nn::fs::OpenMode_Read).IsSuccess());
        s64 sourceSize = 0;
        HK_ASSERT(nn::fs::GetFileSize(&sourceSize, sourceFile).IsSuccess());
        u8* sourceData = (u8*)sPatchHeap->alloc(sourceSize);
        nn::fs::ReadFile(sourceFile, 0, sourceData, sourceSize);
        nn::fs::CloseFile(sourceFile);

        bool isCompressed = sead::SafeString(originalPath).endsWith(cCompressionExt);

        if (isCompressed) {
#if COMPRESSION == ZSTD
            size decompressedSize = ZSTD_getFrameContentSize(sourceData, sourceSize);
            HK_ABORT_UNLESS(decompressedSize != ZSTD_CONTENTSIZE_ERROR, "Failed to get decompressed size for %s", originalPath);
            HK_ABORT_UNLESS(decompressedSize != ZSTD_CONTENTSIZE_UNKNOWN, "Decompressed size for %s is unknown", originalPath);
            u8* decompressedData = (u8*)sPatchHeap->alloc(decompressedSize);
            size r = ZSTD_decompress(decompressedData, decompressedSize, sourceData, sourceSize);
            if (ZSTD_isError(r)) {
                HK_ABORT("ZSTD decompression failed for %s with error: %s", originalPath, ZSTD_getErrorName(r));
                return;
            }

            sPatchHeap->free(sourceData);
            sourceData = decompressedData;
            sourceSize = decompressedSize;
#elif COMPRESSION == YAZ0
            struct header {
                u8 magic[4];
                u32 decompressedSize;
            }* header(reinterpret_cast<struct header*>(sourceData));

            u32 decompressedSize = sead::Endian::swapU32(header->decompressedSize);
            u8* decompressedData = (u8*)sPatchHeap->alloc(decompressedSize);
            HK_ABORT_UNLESS(decodeSZSNxAsm64_(decompressedData, sourceData) >= 0, "SZS decompression failed for %s", originalPath);

            sPatchHeap->free(sourceData);
            sourceData = decompressedData;
            sourceSize = decompressedSize;
#else
#error
#endif
        }

        mem patchedFile = patchFileData({ patchData, size_t(patchSize) }, { sourceData, size_t(sourceSize) }, bpsPath);
        sPatchHeap->free(patchData);
        sPatchHeap->free(sourceData);

        writeFileToPath(patchedFile.ptr, patchedFile.len, outPath);

        sPatchHeap->free(patchedFile.ptr);
    }

    static PathStr getParentPath(const char* filePath) {
        PathStr out(filePath);
        while (out.endsWith("/"))
            out.removeSuffix("/");
        s32 idx = out.rfindIndex("/");
        if (idx == -1)
            return { filePath };
        out.trim(idx);
        return out;
    }

    static hk_noinline bool directoryExists(const char* dirPath) {
        nn::fs::DirectoryEntryType type = (nn::fs::DirectoryEntryType)2;
        return nn::fs::GetEntryType(&type, dirPath).IsSuccess() && type == nn::fs::DirectoryEntryType_Directory;
    }

    static void createDirectoryRecursively(const char* dirPath) {
        PathStr path(dirPath);

        while (!directoryExists(path.cstr())) {
            auto parent = getParentPath(path.cstr());
            if (directoryExists(parent.cstr())) {
                nn::fs::CreateDirectory(path.cstr());
                createDirectoryRecursively(dirPath);
            }
            path = parent;
        }
    }

    template <typename Func>
    static void iterateBpsPatches(const char* rootWalkPath, const char* walkPath, Func&& func) {
        nn::fs::DirectoryHandle dirHandle;
        PathStrFormat* actualWalkPath = new PathStrFormat("%s%s", walkPath, sead::SafeString(walkPath).endsWith(":") ? "/" : "");
        HK_ABORT_UNLESS_R(nn::fs::OpenDirectory(&dirHandle, actualWalkPath->cstr(), nn::fs::OpenDirectoryMode_All).GetInnerValueForDebug());
        s64 entryCount = -1;
        nn::fs::GetDirectoryEntryCount(&entryCount, dirHandle);

        nn::fs::DirectoryEntry* entries = (nn::fs::DirectoryEntry*)sPatchHeap->alloc(sizeof(nn::fs::DirectoryEntry) * entryCount);
        nn::fs::ReadDirectory(&entryCount, entries, dirHandle, entryCount);
        for (s64 i = 0; i < entryCount; i++) {
            nn::fs::DirectoryEntry& entry = entries[i];
            PathStrFormat* entryPath = new PathStrFormat("%s/%s", walkPath, entry.mName);

            if (entry.mType == nn::fs::DirectoryEntryType_File) {
                if (entryPath->endsWith(".bps") or entryPath->endsWith(sVersionExt))
                    func(*entryPath);
            } else if (entry.mType == nn::fs::DirectoryEntryType_Directory)
                iterateBpsPatches(rootWalkPath, entryPath->cstr(), func);

            delete entryPath;
        }
        delete actualWalkPath;

        sPatchHeap->free(entries);

        nn::fs::CloseDirectory(dirHandle);
    }

    static void patchDirRecursive(const char* rootWalkPath, const char* walkPath) {
        iterateBpsPatches(rootWalkPath, walkPath, [&](const PathStr& path) -> void {
            PathStr* rawPath = new PathStr(path);
            rawPath->removeSuffix(".bps");
            rawPath->removeSuffix(sVersionExt);

            PathStr* originalPath = new PathStr(*rawPath);
            rawPath->replaceString(rootWalkPath, "");

            PathStrFormat* outPath = new PathStrFormat(cCacheDir);
            outPath->append(rawPath->cstr() + 1);
            outPath->removeSuffix(cCompressionExt);

            hk::diag::debugLog("Patching %s with %s to %s", originalPath->cstr(), path.cstr(), outPath->cstr());
            createDirectoryRecursively(getParentPath(outPath->cstr()).cstr());
            patchFile(path.cstr(), originalPath->cstr(), outPath->cstr());

            delete originalPath;
            delete outPath;
            delete rawPath;
        });
    }

    static void writePatchesHash(u32 hash) {
        writeFileToPath(&hash, sizeof(hash), cCacheHashFile);
    }

    static u32 readPatchesHash() {
        if (!isFileExist(cCacheHashFile))
            return 0;
        nn::fs::FileHandle hashFile;
        HK_ASSERT(nn::fs::OpenFile(&hashFile, cCacheHashFile, nn::fs::OpenMode_Read).IsSuccess());
        s64 size = 0;
        HK_ASSERT(nn::fs::GetFileSize(&size, hashFile).IsSuccess());
        if (size != sizeof(u32))
            return 0;
        u32 hash = 0;
        nn::fs::ReadFile(hashFile, 0, &hash, sizeof(hash));
        nn::fs::CloseFile(hashFile);
        return hash;
    }

    static u32 calcPatchesHash(const char* rootWalkPath, const char* walkPath) {
        hk::util::detail::HashMurmurImpl<u8, hk::util::detail::ReadDefault<u8>> hash(nullptr, 0);

        int numPatches = 0;
        iterateBpsPatches(rootWalkPath, walkPath, [&](const PathStr& path) -> void {
            nn::fs::FileHandle patchFile;
            HK_ASSERT(nn::fs::OpenFile(&patchFile, path.cstr(), nn::fs::OpenMode_Read).IsSuccess());
            s64 patchSize = 0;
            HK_ASSERT(nn::fs::GetFileSize(&patchSize, patchFile).IsSuccess());
            u32 patchChecksum = 0;
            nn::fs::ReadFile(patchFile, patchSize - sizeof(u32), &patchChecksum, sizeof(u32)); // u32 at end of file: BPS checksum
            nn::fs::CloseFile(patchFile);

            hash.feed(cast<const u8*>(&patchChecksum), sizeof(patchChecksum));
            numPatches++;
        });

        hash.feed(cast<const u8*>(&numPatches), sizeof(numPatches));

        return hash.finalize();
    }

    void applyRomFSPatches(sead::Heap* heap) {
        sPatchHeap = sead::ExpHeap::create(0, "PatchHeap", heap, 8, sead::ExpHeap::cHeapDirection_Forward, false);

        const char* versionName = hk::ro::getMainModule()->getVersionName();
        if (versionName != nullptr) {
            sVersionExt = ".bps.";
            sVersionExt.append(versionName);
        }

        nn::fs::CreateDirectory(getParentPath(cCacheDir).cstr());
        {
            sead::ScopedCurrentHeapSetter setter(sPatchHeap);
            hk::diag::debugLog("Applying RomFS patches with %.2fMB heap", sPatchHeap->getFreeSize() / 1024.f / 1024);

            const u32 patchesHash = calcPatchesHash(cBaseRomFsMount, cBaseRomFsMount);
            hk::diag::debugLog("Patches Hash: %08x", patchesHash);

            const u32 cachedPatchesHash = readPatchesHash();
            bool needsPatch = cachedPatchesHash != patchesHash;

            hk::diag::debugLog("Cached Patches Hash: %08x", cachedPatchesHash);
            openFileHook.uninstall();
            if (needsPatch) {
                nn::fs::DeleteDirectoryRecursively(cCacheDir);
                createDirectoryRecursively(cCacheDir);
                patchDirRecursive(cBaseRomFsMount, cBaseRomFsMount);
                writePatchesHash(patchesHash);
            }
            openFileHook.installAtSym<"_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci">();
        }

        sPatchHeap->destroy();
        sPatchHeap = nullptr;
    }

} // namespace pe
