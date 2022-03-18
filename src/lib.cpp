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

#include <vector>
#include <algorithm>
#include <iostream>

namespace DawnAndroid {
    wgpu::Surface surface;
    dawn::native::Instance instance;
    wgpu::Device device;
    wgpu::Buffer indexBuffer;
    wgpu::Buffer vertexBuffer;
    wgpu::Texture texture;
    wgpu::Sampler sampler;
    wgpu::Queue queue;
    wgpu::SwapChain swapchain;
    wgpu::TextureView depthStencilView;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bindGroup;
    wgpu::TextureFormat GetWGPUFormatFromAHBFormat(uint32_t ahbFormat) {
        switch(ahbFormat)
        {
        case AHARDWAREBUFFER_FORMAT_BLOB:
            return wgpu::TextureFormat::Undefined;
        case AHARDWAREBUFFER_FORMAT_D16_UNORM:
            return wgpu::TextureFormat::Depth16Unorm;
        case AHARDWAREBUFFER_FORMAT_D24_UNORM:
            printf("AHardwareBufferExternalMemory::AndroidHardwareBuffer_Format AHARDWAREBUFFER_FORMAT_D24_UNORM");
            return wgpu::TextureFormat::Depth24Plus;
        case AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT:
            printf("AHardwareBufferExternalMemory::AndroidHardwareBuffer_Format AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT");
            return wgpu::TextureFormat::Depth24UnormStencil8;
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT:
            return wgpu::TextureFormat::Depth32Float;
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT:
            return wgpu::TextureFormat::Depth32FloatStencil8;
        case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
            return wgpu::TextureFormat::RGB10A2Unorm;
        case AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT:
            return wgpu::TextureFormat::RGBA16Float;
        //case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM//:
        //	return wgpu::TextureFormat::R5;//
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
            return wgpu::TextureFormat::RGBA8Unorm;
        // case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
        // 	return wgpu::TextureFormat::RGB;
        //case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
        //	return wgpu::TextureFormat::RGB8Unorm;
        case AHARDWAREBUFFER_FORMAT_S8_UINT:
            return wgpu::TextureFormat::Stencil8;
        case AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420:
        //case AHARDWAREBUFFER_FORMAT_YV12:
        //	return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
        //case AHARDWAREBUFFER_FORMAT_YCbCr_P010:
        //	return VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
        default:
            printf("AHardwareBufferExternalMemory::AHardwareBuffer_Format %d", int(ahbFormat));
            return wgpu::TextureFormat::Undefined;
        }
    }

    static wgpu::BackendType backendType = wgpu::BackendType::Vulkan;

    wgpu::TextureView CreateDefaultDepthStencilView(const wgpu::Device& device, uint32_t width, uint32_t height) {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = width;
        descriptor.size.height = height;
        descriptor.size.depthOrArrayLayers = 1;
        descriptor.sampleCount = 1;
        descriptor.format = wgpu::TextureFormat::Depth24PlusStencil8;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::RenderAttachment;
        auto depthStencilTexture = device.CreateTexture(&descriptor);
        return depthStencilTexture.CreateView();
    }


    void PrintDeviceError(WGPUErrorType errorType, const char* message, void*) {
        const char* errorTypeName = "";
        switch (errorType) {
            case WGPUErrorType_Validation:
                errorTypeName = "Validation";
                break;
            case WGPUErrorType_OutOfMemory:
                errorTypeName = "Out of memory";
                break;
            case WGPUErrorType_Unknown:
                errorTypeName = "Unknown";
                break;
            case WGPUErrorType_DeviceLost:
                errorTypeName = "Device lost";
                break;
            default:
                UNREACHABLE();
                return;
        }
        printf("%s error: %s", errorTypeName, message);
    }

    wgpu::Device AndroidCreateDevice() {
        instance.DiscoverDefaultAdapters();

        // Get an adapter for the backend to use, and create the device.
        dawn::native::Adapter backendAdapter;
        {
            std::vector<dawn::native::Adapter> adapters = instance.GetAdapters();
            auto adapterIt = std::find_if(adapters.begin(), adapters.end(),
                                        [](const dawn::native::Adapter adapter) -> bool {
                                            wgpu::AdapterProperties properties;
                                            adapter.GetProperties(&properties);
                                            return properties.backendType == backendType;
                                        });
            ASSERT(adapterIt != adapters.end());
            backendAdapter = *adapterIt;
        }

        WGPUDevice device = backendAdapter.CreateDevice();
        DawnProcTable procs = dawn::native::GetProcs();
   
        dawnProcSetProcs(&procs);
        procs.deviceSetUncapturedErrorCallback(device, PrintDeviceError, nullptr);
        return wgpu::Device::Acquire(device);
    }


    void initBuffers() {
        static const uint32_t indexData[3] = {
            0,
            1,
            2,
        };
        indexBuffer =
            utils::CreateBufferFromData(device, indexData, sizeof(indexData), wgpu::BufferUsage::Index);

        static const float vertexData[12] = {
            0.0f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f,
        };
        vertexBuffer = utils::CreateBufferFromData(device, vertexData, sizeof(vertexData),
                                                wgpu::BufferUsage::Vertex);
    }

    void initTextures() {
        wgpu::TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = 1024;
        descriptor.size.height = 1024;
        descriptor.size.depthOrArrayLayers = 1;
        descriptor.sampleCount = 1;
        descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
        descriptor.mipLevelCount = 1;
        descriptor.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
        texture = device.CreateTexture(&descriptor);

        sampler = device.CreateSampler();

        // Initialize the texture with arbitrary data until we can load images
        std::vector<uint8_t> data(4 * 1024 * 1024, 0);
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = static_cast<uint8_t>(i % 253);
        }

        wgpu::Buffer stagingBuffer = utils::CreateBufferFromData(
            device, data.data(), static_cast<uint32_t>(data.size()), wgpu::BufferUsage::CopySrc);
        wgpu::ImageCopyBuffer imageCopyBuffer =
            utils::CreateImageCopyBuffer(stagingBuffer, 0, 4 * 1024);
        wgpu::ImageCopyTexture imageCopyTexture = utils::CreateImageCopyTexture(texture, 0, {0, 0, 0});
        wgpu::Extent3D copySize = {1024, 1024, 1};

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        encoder.CopyBufferToTexture(&imageCopyBuffer, &imageCopyTexture, &copySize);

        wgpu::CommandBuffer copy = encoder.Finish();
        queue.Submit(1, &copy);
    }

    std::unique_ptr<wgpu::ChainedStruct> GetAndroidNativeWindowDescriptor(void* window) {
        std::unique_ptr<wgpu::SurfaceDescriptorFromAndroidNativeWindow> desc =
            std::make_unique<wgpu::SurfaceDescriptorFromAndroidNativeWindow>();
        desc->window = window;
        desc->sType = wgpu::SType::SurfaceDescriptorFromAndroidNativeWindow;
        return std::move(desc);
    }

    wgpu::Surface CreateSurfaceForWindow(const wgpu::Instance& instance, void* window) {
        std::unique_ptr<wgpu::ChainedStruct> chainedDescriptor =
            GetAndroidNativeWindowDescriptor(window);

        wgpu::SurfaceDescriptor descriptor;
        descriptor.nextInChain = chainedDescriptor.get();
        return instance.CreateSurface(&descriptor);
    }

    void Init(ANativeWindow* window, uint32_t width, uint32_t height, int32_t ahbFormat) {
        
        device = AndroidCreateDevice();
        queue = device.GetQueue();

        

        // auto mSwapchainImpl = dawn::native::vulkan::CreateNativeSwapChainImpl(device.Get(), nullptr);
        // auto fmt = dawn::native::vulkan::GetNativeSwapChainPreferredFormat(&mSwapchainImpl);
   

        wgpu::SwapChainDescriptor swapChainDesc;
        surface = CreateSurfaceForWindow(instance.Get(), window);
        // swapChainDesc.implementation = GetSwapChainImplementation();

        wgpu::TextureFormat fmt = GetWGPUFormatFromAHBFormat(ahbFormat);
        swapChainDesc.format    = fmt;
        swapChainDesc.width     = width;
        swapChainDesc.height    = height;
        swapChainDesc.implementation = 0;
        swapChainDesc.presentMode = wgpu::PresentMode::Fifo;
        swapChainDesc.usage = wgpu::TextureUsage::RenderAttachment;
        
        swapchain = device.CreateSwapChain(surface, &swapChainDesc);

        // swapchain.Configure(
        //     GetPreferredSwapChainTextureFormat(), 
        //     wgpu::TextureUsage::RenderAttachment,
        //     640, 
        //     480
        // );

        // swapchain.Acquire()

        printf("WHAT\n");
        initBuffers();
        printf("WHAT2\n");
        initTextures();

        wgpu::ShaderModule vsModule = utils::CreateShaderModule(device, R"(
            @stage(vertex) fn main(@location(0) pos : vec4<f32>)
                                -> @builtin(position) vec4<f32> {
                return pos;
            })");

        wgpu::ShaderModule fsModule = utils::CreateShaderModule(device, R"(
            @group(0) @binding(0) var mySampler: sampler;
            @group(0) @binding(1) var myTexture : texture_2d<f32>;

            @stage(fragment) fn main(@builtin(position) FragCoord : vec4<f32>)
                                -> @location(0) vec4<f32> {
                return textureSample(myTexture, mySampler, FragCoord.xy / vec2<f32>(1080.0, 2296.0));
            })");

        auto bgl = utils::MakeBindGroupLayout(
            device, {
                        {0, wgpu::ShaderStage::Fragment, wgpu::SamplerBindingType::Filtering},
                        {1, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float},
                    });

        wgpu::PipelineLayout pl = utils::MakeBasicPipelineLayout(device, &bgl);

        depthStencilView = CreateDefaultDepthStencilView(device, width, height);

        utils::ComboRenderPipelineDescriptor descriptor;
        descriptor.layout = utils::MakeBasicPipelineLayout(device, &bgl);
        descriptor.vertex.module = vsModule;
        descriptor.vertex.bufferCount = 1;
        descriptor.cBuffers[0].arrayStride = 4 * sizeof(float);
        descriptor.cBuffers[0].attributeCount = 1;
        descriptor.cAttributes[0].format = wgpu::VertexFormat::Float32x4;
        descriptor.cFragment.module = fsModule;
        descriptor.cTargets[0].format = fmt;
        descriptor.EnableDepthStencil(wgpu::TextureFormat::Depth24PlusStencil8);

        pipeline = device.CreateRenderPipeline(&descriptor);

        wgpu::TextureView view = texture.CreateView();

        bindGroup = utils::MakeBindGroup(device, bgl, {{0, sampler}, {1, view}});
    }

    struct {
        uint32_t a;
        float b;
    } s;

    void Frame() {
        s.a = (s.a + 1) % 256;
        s.b += 0.02f;
        if (s.b >= 1.0f) {
            s.b = 0.0f;
        }

        wgpu::TextureView backbufferView = swapchain.GetCurrentTextureView();
        utils::ComboRenderPassDescriptor renderPass({backbufferView}, depthStencilView);

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        {
            wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderPass);
            pass.SetPipeline(pipeline);
            pass.SetBindGroup(0, bindGroup);
            pass.SetVertexBuffer(0, vertexBuffer);
            pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32);
            pass.DrawIndexed(3);
            pass.EndPass();
        }

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit(1, &commands);
        swapchain.Present();
    }
};
