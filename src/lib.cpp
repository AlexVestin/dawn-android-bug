// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lib.h"
#include "util.h"
#include "helpers.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <utility>
#include <cassert>
#include <chrono>
#include <thread>

static const wgpu::BackendType backendType = wgpu::BackendType::Vulkan;

static const char *shader = R"(
// SPDX-License-Identifier: Apache-2.0 OR MIT OR Unlicense

struct PathInfo {
    bb_tl: u32,
    bb_br: u32
}

struct ComputeUniforms {
    path_count: u32,
}

@group(0) @binding(0) var<storage, read> path_info: array<PathInfo>;
@group(0) @binding(1) var<storage, read_write> bin_header: array<atomic<u32>>;
@group(0) @binding(2) var<uniform> compute_uniforms: ComputeUniforms;

const WG_SIZE = 256u;
const N_TILE = 256u;
const TILE_SIZE = 16u;
const uniform_width = 1080u;

var<workgroup> sh_counts: array<atomic<u32>, 256>;

fn div_up(v: u32, c: u32) -> u32 {
    return (v + (c - 1u)) / c; 
}

struct TRBLRect{
    t: u32,
    r: u32,
    b: u32,
    l: u32
}

fn get_trbl_rect(tl: u32, br: u32) -> TRBLRect {
    let t = (tl >> 16u) & 0xffffu;
    let l = tl & 0xffffu;
    let b = (br >> 16u) & 0xffffu;
    let r = br & 0xffffu;
    return TRBLRect(t, r, b, l);
}

@compute @workgroup_size(256)
fn main(
    @builtin(global_invocation_id) global_id: vec3<u32>,
    @builtin(local_invocation_id) local_id: vec3<u32>,
    @builtin(workgroup_id) wg_id: vec3<u32>,
) {
    atomicStore(&sh_counts[local_id.x], 0u);   
    workgroupBarrier();
    let element_ix = global_id.x;
    
    var path_area = TRBLRect(0u, 0u, 0u, 0u);
    if element_ix < 303u {
        let info = path_info[element_ix];
        path_area = get_trbl_rect(info.bb_tl, info.bb_br);
    }

    let width_in_bins = div_up(div_up(uniform_width, TILE_SIZE), TILE_SIZE);
    
    let x0 = path_area.l / TILE_SIZE;
    let y0 = path_area.t / TILE_SIZE;
    let x1 = div_up(path_area.r, TILE_SIZE) * u32(element_ix < 303u);
    var y1 = div_up(path_area.b, TILE_SIZE) * u32(element_ix < 303u);

    if x0 == x1 {
        y1 = y0;
    }
    
    workgroupBarrier();
    // --- 1 --- 
    for (var y = y0; y < y1; y++) {
        for (var x = x0; x < x1; x++) {
            atomicAdd(&sh_counts[y * width_in_bins + x], 1u);
        }
    }
    
    // --- 2 ---
    // if (local_id.x < 128u) {
    //     for (var y = y0; y < y1; y++) {
    //         for (var x = x0; x < x1; x++) {
    //             atomicAdd(&sh_counts[y * width_in_bins + x], 1u);
    //         }
    //     }
    // }
    // workgroupBarrier();
    // if (local_id.x >= 128u) {
    //     for (var y = y0; y < y1; y++) {
    //         for (var x = x0; x < x1; x++) {
    //             atomicAdd(&sh_counts[y * width_in_bins + x], 1u);
    //         }
    //     }
    // }

    // --- 3 ---
    // atomicStore(&sh_counts[local_id.x], local_id.x + 4u);

    workgroupBarrier();
    let num_partitions = div_up(compute_uniforms.path_count, WG_SIZE);
    let v = atomicLoad(&sh_counts[local_id.x]);
    
    // -- a ---
    for (var i = wg_id.x; i < 2u; i++) {
        atomicAdd(&bin_header[local_id.x * 2u + i], v);
    }

    // --- b ---
    // for (var i = 0u; i < 2u; i++) {
    //     atomicAdd(&bin_header[local_id.x * 2u + i], v);
    // }
}
)";

namespace DawnAndroid
{
    dawn::native::Instance instance;
    wgpu::Device device;

    wgpu::Buffer pathAreaBuffer;
    wgpu::Buffer outputBuffer;
    wgpu::Buffer uniformBuffer;

    wgpu::ComputePipeline pipeline;
    wgpu::BindGroup bindGroup;

    void PrintDeviceError(WGPUErrorType errorType, const char *message, void *)
    {
        printf("%s error: %s", "Unknown", message);
    }

    wgpu::Device AndroidCreateDevice()
    {
        wgpu::RequestAdapterOptions options = {};
        options.backendType = backendType;
        std::vector<dawn::native::Adapter> adapters = instance.EnumerateAdapters(&options);
        if (adapters.size() == 0)
        {
            std::cout << "Failed to find valid adapter" << std::endl;
            exit(0);
        }

        dawn::native::Adapter backendAdapter = adapters[0];

        WGPUDevice device = backendAdapter.CreateDevice();
        DawnProcTable procs = dawn::native::GetProcs();

        dawnProcSetProcs(&procs);
        procs.deviceSetUncapturedErrorCallback(device, PrintDeviceError, nullptr);
        return wgpu::Device::Acquire(device);
    }

    void Init(uint32_t width, uint32_t height)
    {
        device = AndroidCreateDevice();

        pathAreaBuffer =
            dawn::utils::CreateBufferFromData(device, pathAreaData, 512 * sizeof(uint32_t), wgpu::BufferUsage::Storage);

        uint32_t numPaths = 303;
        uniformBuffer = dawn::utils::CreateBufferFromData(device, &numPaths, sizeof(uint32_t), wgpu::BufferUsage::Uniform);

        wgpu::BufferDescriptor descriptor;
        descriptor.size = 512 * sizeof(uint32_t);
        descriptor.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopySrc;
        outputBuffer = device.CreateBuffer(&descriptor);

        auto bgl =
            dawn::utils::MakeBindGroupLayout(device, {
                                                         {0, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::ReadOnlyStorage},
                                                         {1, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Storage},
                                                         {2, wgpu::ShaderStage::Compute, wgpu::BufferBindingType::Uniform},
                                                     });
        pipeline = CreatePipeline(device, bgl, shader, "Binning");
        bindGroup = dawn::utils::MakeBindGroup(device, bgl, {{0, pathAreaBuffer}, {1, outputBuffer}, {2, uniformBuffer}});
    }

    void Frame()
    {
        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::ComputePassDescriptor descriptor;
        wgpu::ComputePassEncoder passEncoder = encoder.BeginComputePass(&descriptor);

        passEncoder.SetPipeline(pipeline);
        passEncoder.SetBindGroup(0, bindGroup);
        passEncoder.DispatchWorkgroups(1);
        passEncoder.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        device.GetQueue().Submit(1, &commands);

        bool done = false;
        device.GetQueue().OnSubmittedWorkDone(
            [](WGPUQueueWorkDoneStatus status, void *userdata) -> void
            { *(static_cast<bool *>(userdata)) = true; },
            &done);

        while (!done)
        {
            device.Tick();
            std::this_thread::sleep_for(std::chrono::microseconds{1});
        }

        std::vector<uint32_t> outputData = CopyReadBackBuffer<uint32_t>(device, outputBuffer, 256 * sizeof(uint32_t));

        for (int i = 0; i < 4; i++)
        {
            LOGI("%d ", outputData[i]);
        }
        LOGI("\nDone\n");
    }
}
