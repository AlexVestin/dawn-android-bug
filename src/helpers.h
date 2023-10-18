#pragma once

#include <vector>
#include <thread>
#include <chrono>

#include "dawn/webgpu_cpp.h"

template<typename T>
std::vector<T> ReadBackBuffer(
  const wgpu::Device& device, 
  const wgpu::Buffer& fromBuffer, 
  uint32_t byteSize
) {
  WGPUBufferMapAsyncStatus readStatus = WGPUBufferMapAsyncStatus_Unknown;
  fromBuffer.MapAsync(
      wgpu::MapMode::Read, 
      0, 
      byteSize, 
      [](WGPUBufferMapAsyncStatus status, void * userdata) {
          *static_cast<bool*>(userdata) = status;
      }, 
      &readStatus
  );

  uint32_t iterations = 0;
  while (readStatus == WGPUBufferMapAsyncStatus_Unknown) {
      #ifndef __EMSCRIPTEN__
      device.Tick();
      std::this_thread::sleep_for(std::chrono::microseconds{ 50 });
      #endif
      if (iterations++ > 100000) {
        std::cout << " ------ Failed to retrieve buffer -------- " << std::endl;
        break;
      }
  }

  if (readStatus == WGPUBufferMapAsyncStatus_Success) {
      const T* data = static_cast<const T*>(fromBuffer.GetConstMappedRange());
      fromBuffer.Unmap();
      return { &data[0], &data[byteSize / sizeof(T)] };
  }

  LOGE("Failed to read back buffer, with status: %d\n", static_cast<int>(readStatus));
  return { T() };
}

template<typename T>
std::vector<T> CopyReadBackBuffer(
  const wgpu::Device& device, 
  const wgpu::Buffer& fromBuffer, 
  uint32_t byteSize
) {
  wgpu::BufferDescriptor desc;
  desc.size = byteSize;
  desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;  
  desc.mappedAtCreation = false;
  desc.label = "ReadbackBuffer";

  wgpu::Buffer copyBuffer = device.CreateBuffer(&desc);

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
  encoder.CopyBufferToBuffer(fromBuffer, 0, copyBuffer, 0, byteSize);
  wgpu::CommandBuffer commandBuffer = encoder.Finish();
  
  commandBuffer.SetLabel("ReadBackCommandBuffer");
  auto queue = device.GetQueue();
  queue.Submit(1, &commandBuffer);  

  std::vector<T> vv = std::move(ReadBackBuffer<T>(device, copyBuffer, byteSize));
  copyBuffer.Destroy(); 

  return vv;
}

wgpu::ComputePipeline CreatePipeline(const wgpu::Device &device, const wgpu::BindGroupLayout &bgl,
                                        const std::string &shader, const char *label)
{
    wgpu::ShaderModule shaderModule = dawn::utils::CreateShaderModule(device, shader.c_str());
    wgpu::PipelineLayout pl = dawn::utils::MakeBasicPipelineLayout(device, &bgl);
    wgpu::ComputePipelineDescriptor csDesc;
    csDesc.layout = pl;
    csDesc.compute.module = shaderModule;
    csDesc.compute.entryPoint = "main";
    csDesc.label = label;
    csDesc.compute.constantCount = 0;
    return device.CreateComputePipeline(&csDesc);
}

const uint32_t pathAreaData[512] = {
    1310722,
    1703943,
    1245186,
    1703943,
    1179652,
    1703944,
    1179652,
    1703944,
    1900547,
    2031627,
    1835011,
    2031627,
    1966084,
    2228235,
    1966083,
    2228235,
    1966083,
    2162698,
    1900547,
    2162698,
    1703937,
    1900553,
    1638401,
    1900553,
    1769472,
    1900552,
    1703936,
    1900552,
    1835008,
    1966088,
    1769472,
    1966088,
    1703938,
    1966089,
    1638402,
    1966089,
    1441792,
    1769479,
    1376256,
    1769479,
    1507328,
    1769479,
    1441792,
    1769479,
    1572864,
    1835015,
    1507328,
    1835015,
    1572869,
    3735607,
    1572869,
    3801144,
    5,
    1900599,
    5,
    1900599,
    1048580,
    2162742,
    1048580,
    2228278,
    1048580,
    2228278,
    1048581,
    2228278,
    1048581,
    2293814,
    1114117,
    2293814,
    1114117,
    2293814,
    1114117,
    2359350,
    1114118,
    2359350,
    1114118,
    2359350,
    1114118,
    2424886,
    1179654,
    2424886,
    2031625,
    2359310,
    1703963,
    2490402,
    2293773,
    2883602,
    2359309,
    2621459,
    2424846,
    2686996,
    2293776,
    2555925,
    2621452,
    2883602,
    2555918,
    2752531,
    2621455,
    3276831,
    2097167,
    3080224,
    2293776,
    3014687,
    2097171,
    2555934,
    2555923,
    2883612,
    2162707,
    2818077,
    2097170,
    2818077,
    2686990,
    2949140,
    2621454,
    3014676,
    2097171,
    2490397,
    2293782,
    2687000,
    2818066,
    2949140,
    2818065,
    2949140,
    2883602,
    2949140,
    2818066,
    2949140,
    2883603,
    2949141,
    2818066,
    2949140,
    2883603,
    2949141,
    2818067,
    2949141,
    2883604,
    2949142,
    2818068,
    2949142,
    2818069,
    2949143,
    2818068,
    3014679,
    2752530,
    2883606,
    2686997,
    2883611,
    2686998,
    3014681,
    2621462,
    3014681,
    2752537,
    2883611,
    2621466,
    2752540,
    2883600,
    3014676,
    2228242,
    2621462,
    2228242,
    2621462,
    2228234,
    2555918,
    2228234,
    2555919,
    2293772,
    2424846,
    2293772,
    2424846,
    2228236,
    2359314,
    2293777,
    2424851,
    2293777,
    2424851,
    2293773,
    2424847,
    2293773,
    2424847,
    2293774,
    2424848,
    2293774,
    2424848,
    2293775,
    2424849,
    2293775,
    2424849,
    2293776,
    2424850,
    2293776,
    2490386,
    2228234,
    2424846,
    2228242,
    2359318,
    589844,
    1376288,
    589845,
    1376288,
    589845,
    1376288,
    589845,
    1376288,
    589845,
    1376287,
    655381,
    1376287,
    1048598,
    1310751,
    1048598,
    1376287,
    1114135,
    1310747,
    1114135,
    1245211,
    1114137,
    1245211,
    1114136,
    1245210,
    655370,
    1245199,
    655370,
    1245199,
    655370,
    1245199,
    655370,
    1245199,
    655370,
    1245199,
    655370,
    1245199,
    983050,
    1114126,
    983051,
    1245198,
    1048588,
    1245198,
    917525,
    1507363,
    65576,
    786483,
    65581,
    589874,
    65581,
    589874,
    65581,
    589874,
    65581,
    589874,
    65581,
    589874,
    65581,
    589874,
    1245196,
    1703962,
    2687014,
    3080234,
    2490409,
    3145775,
    2293805,
    2883637,
    2687007,
    3080230,
    1900589,
    2228278,
    655396,
    3014707,
    17,
    720940,
    65567,
    327722,
    131103,
    327720,
    65564,
    196640,
    131096,
    262173,
    458776,
    589851,
    131093,
    196632,
    196620,
    1179681,
    1441811,
    1703963,
    1441807,
    1638424,
    1376269,
    1638423,
    1310734,
    1572887,
    327702,
    3080239,
    1310739,
    1572888,
    1048588,
    1179662,
    1048588,
    1179662,
    1048588,
    1179662,
    1703942,
    2228241,
    1835015,
    2228239,
    1900552,
    2228237,
    1114119,
    1900559,
    1835014,
    2162696,
    1376261,
    1900554,
    720918,
    1179674,
    720919,
    983065,
    589853,
    917537,
    655384,
    1048607,
    589838,
    1179671,
    720911,
    851987,
    720909,
    917519,
    1048585,
    1310733,
    1245192,
    1441804,
    1769507,
    1966118,
    1966115,
    2162725,
    1703951,
    2097179,
    1703954,
    1966108,
    1835024,
    2162715,
    1835026,
    2097181,
    2031635,
    2162708,
    2031634,
    2162707,
    2097169,
    2162706,
    2097168,
    2162705,
    1769492,
    1900565,
    1835027,
    1900565,
    1835026,
    1966099,
    1900561,
    1966098,
    1966096,
    2031634,
    1966100,
    2097174,
    1966102,
    2031639,
    1900567,
    2031640,
    1900568,
    1966106,
    1769493,
    1835031,
    1703958,
    1835032,
    2031635,
    2097173,
    1310731,
    1441804,
    1441801,
    1572874,
    851979,
    1114126,
    720908,
    1114126,
    851979,
    1048589,
    1048585,
    1114125,
    983049,
    1114126,
    983049,
    1114125,
    983049,
    1114126,
    983049,
    1114126,
    983049,
    1114126,
    983050,
    1114126,
    983050,
    1114126,
    983065,
    1179677,
    983065,
    1179677,
    983064,
    1179677,
    983064,
    1179677,
    983064,
    1179676,
    983064,
    1179676,
    983064,
    1179676,
    983064,
    1179676,
    1966099,
    2293789,
    1900563,
    2228253,
    2162710,
    2228247,
    2162709,
    2228247,
    2162708,
    2293782,
    2162707,
    2293781,
    2097176,
    2228249,
    2031641,
    2162715,
    2031642,
    2162716,
    1966107,
    2097181,
    2162711,
    2228248,
    1114139,
    1310751,
    1114139,
    1310751,
    1638425,
    2293792,
    1966101,
    2293796,
    1966101,
    2293796,
    1966102,
    2293801,
    1966102,
    2293801,
    1900567,
    2162734,
    1835031,
    2162734,
    1769496,
    1966122,
    1638424,
    1966122,
    2031634,
    2424853,
    2031634,
    2424853,
    2097169,
    2424851,
    2031633,
    2424852,
    2097168,
    2293778,
    2097168,
    2293779,
    1179669,
    1835042,
    1114133,
    1835042,
    1245206,
    1835048,
    1179670,
    1835048,
    1310740,
    1900571,
    1245204,
    1900571,
    1572882,
    1966107,
    1507346,
    1966107,
    1441811,
    1900572,
    1441811,
    1900572,
    1703953,
    1966105,
    1638417,
    1966105,
    1769488,
    2031639,
    1703952,
    2031639,
    1376280,
    1769515,
    1310744,
    1769515,
    2162708,
    2424854,
    2162708,
    2424855,
    2162709,
    2555929,
    2162709,
    2555929,
};