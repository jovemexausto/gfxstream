// Copyright (C) 2024 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

#include <memory>
#include <mutex>
#include <unordered_map>

extern "C" {
#include "gfxstream/virtio-gpu-gfxstream-renderer-unstable.h"
#include "gfxstream/virtio-gpu-gfxstream-renderer.h"
}  // extern "C"

#include "virtio_gpu.h"
#include "virtio_gpu_context.h"
#include "virtio_gpu_format_utils.h"
#ifdef GFXSTREAM_BUILD_WITH_SNAPSHOT_FRONTEND_SUPPORT
#include "virtio_gpu_frontend_snapshot.pb.h"
#endif
#include "gfxstream/host/features.h"
#include "render-utils/Renderer.h"
#include "virtio_gpu_resource.h"
#include "virtio_gpu_timelines.h"

namespace gfxstream {
namespace host {

class CleanupThread;

class VirtioGpuFrontend {
   public:
    VirtioGpuFrontend();

    int init(RendererPtr renderer, void* cookie, const gfxstream::host::FeatureSet& features,
             stream_renderer_fence_callback fence_callback);

    void teardown();

    int createContext(VirtioGpuContextId ctx_id, uint32_t nlen, const char* name,
                      uint32_t context_init);

    int destroyContext(VirtioGpuContextId handle);

    int processCommand(const struct stream_renderer_command* cmd);
    int processAddressSpaceCommand(VirtioGpuContextId ctxId, const uint8_t* cmd_buffer,
                                  uint32_t cmd_buffer_size);

    int createFence(uint64_t fence_id, const VirtioGpuRing& ring);

    int acquireContextFence(uint32_t ctx_id, uint64_t fenceId);

    void poll();

    int createResource(struct stream_renderer_resource_create_args* args, struct iovec* iov,
                       uint32_t num_iovs);
    int importResource(uint32_t res_handle, const struct stream_renderer_handle* import_handle,
                       const struct stream_renderer_import_data* import_data);
    void unrefResource(uint32_t toUnrefId);

    int attachIov(int resId, iovec* iov, int num_iovs);
    void detachIov(int resId);

    int transferReadIov(int resId, uint64_t offset, stream_renderer_box* box, struct iovec* iov,
                        int iovec_cnt);
    int transferWriteIov(int resId, uint64_t offset, stream_renderer_box* box, struct iovec* iov,
                         int iovec_cnt);

    void getCapset(uint32_t set, uint32_t* max_size);
    void fillCaps(uint32_t set, void* caps);

    void attachResource(uint32_t ctxId, uint32_t resId);
    void detachResource(uint32_t ctxId, uint32_t toUnrefId);

    int getResourceInfo(uint32_t resId, struct stream_renderer_resource_info* info);

    void flushResource(uint32_t res_handle);

    int createRingBlob(VirtioGpuResource& entry, uint32_t res_handle,
                       const struct stream_renderer_create_blob* create_blob,
                       const struct stream_renderer_handle* handle);

    int createBlob(uint32_t ctx_id, uint32_t res_handle,
                   const struct stream_renderer_create_blob* create_blob,
                   const struct stream_renderer_handle* handle);

    int resourceMap(uint32_t resourceId, void** hvaOut, uint64_t* sizeOut);
    int resourceUnmap(uint32_t res_handle);

    void* platformCreateSharedEglContext();

    int platformDestroySharedEglContext(void* context);

    int resourceMapInfo(uint32_t resourceId, uint32_t* map_info);

    int exportBlob(uint32_t resourceId, struct stream_renderer_handle* handle);

    int exportFence(uint64_t fenceId, struct stream_renderer_handle* handle);
    int vulkanInfo(uint32_t res_handle, struct stream_renderer_vulkan_info* vulkan_info);

    void setupWindow(void* nativeWindowHandle, int32_t windowX, int32_t windowY,
                     int32_t windowWidth, int32_t windowHeight, int32_t framebufferWidth,
                     int32_t framebufferHeight);

    void setScreenMask(int width, int height, const uint8_t* rgbaData);
    void setScreenBackground(int width, int height, const uint8_t* rgbaData);

#ifdef GFXSTREAM_BUILD_WITH_SNAPSHOT_FRONTEND_SUPPORT
    int snapshot(const char* directory);
    int restore(const char* directory);
#endif  // GFXSTREAM_BUILD_WITH_SNAPSHOT_FRONTEND_SUPPORT

   private:
    VirtioGpuTimelines::FenceCompletionCallback getFenceCompletionCallback();

    int destroyVirtioGpuObjects();

#ifdef GFXSTREAM_BUILD_WITH_SNAPSHOT_FRONTEND_SUPPORT
    int snapshotRenderer(const char* directory);
    int snapshotFrontend(const char* directory);
    int snapshotAsg(const char* directory);

    int restoreRenderer(const char* directory);
    int restoreFrontend(const char* directory);
    int restoreAsg(const char* directory);
#endif  // GFXSTREAM_BUILD_WITH_SNAPSHOT_FRONTEND_SUPPORT

    RendererPtr mRenderer;
    void* mCookie = nullptr;
    gfxstream::host::FeatureSet mFeatures;
    stream_renderer_fence_callback mFenceCallback;
    uint32_t mPageSize = 4096;

    // State that is preserved across snapshots:
    //
    // LINT.IfChange(virtio_gpu_frontend)
    std::unordered_map<VirtioGpuContextId, VirtioGpuContext> mContexts;
    //
    // mResources is read from both the dispatch thread (holding the Rust-side
    // Arc<Mutex<VirtioGpu>>) and a deferred TransferFromHost3d reader thread
    // (deliberately NOT holding that lock, by design -- see
    // rutabaga_gfx::transfer_read_blocking_by_id and libkrun
    // worker.rs/process_queue's comments) -- plus, per gfxstream's own
    // architecture, any of SyncThread's worker-pool threads that end up
    // running a fence/compose completion callback. None of those call sites
    // share a single lock today, so a `VirtioGpuResource` stored by value can
    // be erased/move-assigned out from under an in-flight reader -> dangling
    // mIovs/mLinear -> SIGSEGV in TransferWithIov (observed repeatedly; see
    // patches/README.md libkrun 0010 and the capivara-compose-loop-blocker
    // memory note for the full diagnostic trail).
    //
    // shared_ptr + a short-lived mutex around just the map operations (not
    // any blocking I/O) fixes this without reintroducing the dispatcher
    // deadlock libkrun 0007 exists to avoid: a caller copies the shared_ptr
    // under mResourcesMutex, releases the lock immediately, then does its
    // (possibly slow) work through that copy -- the resource stays alive for
    // as long as any copy of the shared_ptr does, even if another thread
    // erases it from the map in the meantime.
    mutable std::mutex mResourcesMutex;
    std::unordered_map<VirtioGpuResourceId, std::shared_ptr<VirtioGpuResource>> mResources;
    std::unordered_map<uint64_t, std::shared_ptr<SyncDescriptorInfo>> mSyncMap;
    // When we wait for gpu or wait for gpu vulkan, the next (and subsequent)
    // fences created for that context should not be signaled immediately.
    // Rather, they should get in line.
    std::unique_ptr<VirtioGpuTimelines> mVirtioGpuTimelines = nullptr;
    // LINT.ThenChange(VirtioGpuFrontend.h:virtio_gpu_frontend)

    std::unique_ptr<CleanupThread> mCleanupThread;
    bool mLogCalls = false;
};

}  // namespace host
}  // namespace gfxstream
