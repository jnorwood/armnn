//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//
#pragma once

#include "ActivationFixture.hpp"
#include "QuantizeHelper.hpp"

#include <armnn/ArmNN.hpp>
#include <armnn/Tensor.hpp>
#include <armnn/TypesUtils.hpp>

#include <backendsCommon/CpuTensorHandle.hpp>
#include <backendsCommon/IBackendInternal.hpp>
#include <backendsCommon/WorkloadFactory.hpp>

#include <test/TensorHelpers.hpp>

#include <algorithm>

template<armnn::DataType ArmnnType, typename T = armnn::ResolveType<ArmnnType>>
LayerTestResult<T, 4> BoundedReLuTestCommon(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    float upperBound,
    float lowerBound,
    float inputScale,
    int32_t inputOffset,
    float outputScale,
    int32_t outputOffset,
    const std::vector<T>& inputData,
    const std::vector<T>& outputExpectedData,
    unsigned int inputWidth,
    unsigned int inputHeight,
    unsigned int inputChannels,
    unsigned int inputBatchSize)
{
    unsigned int outputWidth = inputWidth;
    unsigned int outputHeight = inputHeight;
    unsigned int outputChannels = inputChannels;
    unsigned int outputBatchSize = inputBatchSize;

    armnn::TensorInfo inputTensorInfo({ inputBatchSize, inputChannels, inputHeight, inputWidth }, ArmnnType);

    armnn::TensorInfo outputTensorInfo({ outputBatchSize, outputChannels, outputHeight, outputWidth }, ArmnnType);

    if(armnn::IsQuantizedType<T>())
    {
        inputTensorInfo.SetQuantizationScale(inputScale);
        inputTensorInfo.SetQuantizationOffset(inputOffset);

        outputTensorInfo.SetQuantizationScale(outputScale);
        outputTensorInfo.SetQuantizationOffset(outputOffset);
    }

    LayerTestResult<T, 4> result(inputTensorInfo);

    auto input = MakeTensor<T, 4>(inputTensorInfo, inputData);

    std::unique_ptr<armnn::ITensorHandle> inputHandle = workloadFactory.CreateTensorHandle(inputTensorInfo);
    std::unique_ptr<armnn::ITensorHandle> outputHandle = workloadFactory.CreateTensorHandle(outputTensorInfo);

    // Setup bounded ReLu.
    armnn::ActivationQueueDescriptor descriptor;
    armnn::WorkloadInfo workloadInfo;
    AddInputToWorkload(descriptor, workloadInfo, inputTensorInfo, inputHandle.get());
    AddOutputToWorkload(descriptor, workloadInfo, outputTensorInfo, outputHandle.get());

    descriptor.m_Parameters.m_Function = armnn::ActivationFunction::BoundedReLu;
    descriptor.m_Parameters.m_A = upperBound;
    descriptor.m_Parameters.m_B = lowerBound;

    std::unique_ptr<armnn::IWorkload> workload = workloadFactory.CreateActivation(descriptor, workloadInfo);

    inputHandle->Allocate();
    outputHandle->Allocate();

    CopyDataToITensorHandle(inputHandle.get(), &input[0][0][0][0]);

    workload->Execute();

    CopyDataFromITensorHandle(&result.output[0][0][0][0], outputHandle.get());

    result.outputExpected = MakeTensor<T, 4>(outputTensorInfo, outputExpectedData);

    return result;
}

LayerTestResult<float, 4> BoundedReLuUpperAndLowerBoundTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager)
{
    unsigned int inputWidth = 4u;
    unsigned int inputHeight = 5u;
    unsigned int inputChannels = 1u;
    unsigned int inputBatchSize = 1;

    std::vector<float> input = std::vector<float>{
      -2.0f,       0.1f,     0.5f,     1.25f,
     0.786f,    0.9875f,    -1.5f,    0.384f,
    1.0001f,       3.5f,     7.5f,    0.896f,
     2.126f,       2.0f,     0.3f,     0.15f,
     0.999f,       1.2f,    0.89f,      6.1f,
    };

    // Calculated manually.
    std::vector<float> output = std::vector<float>{
      -1.0f,       0.1f,     0.5f,      1.0f,
     0.786f,    0.9875f,    -1.0f,    0.384f,
       1.0f,       1.0f,     1.0f,    0.896f,
       1.0f,       1.0f,     0.3f,     0.15f,
     0.999f,       1.0f,    0.89f,      1.0f,
    };

    return BoundedReLuTestCommon<armnn::DataType::Float32>(
        workloadFactory, memoryManager, 1.0f, -1.0f, 1.0f, 0, 1.0f, 0, input, output,
        inputWidth, inputHeight, inputChannels, inputBatchSize);
}

LayerTestResult<float, 4> BoundedReLuUpperBoundOnlyTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager)
{
    unsigned int inputWidth = 4u;
    unsigned int inputHeight = 5u;
    unsigned int inputChannels = 1u;
    unsigned int inputBatchSize = 1;

    std::vector<float> input = std::vector<float>{
      -1.0f,       0.1f,     0.5f,      6.25f,
     0.786f,    5.9875f,    -0.5f,     0.384f,
    6.0001f,       3.5f,     7.5f,     0.896f,
     2.126f,      12.0f,     0.3f,      0.15f,
     0.999f,       1.2f,    0.89f,       6.1f,
    };

    // Calculated manually.
    std::vector<float> output = std::vector<float>{
       0.0f,       0.1f,     0.5f,       6.0f,
     0.786f,    5.9875f,     0.0f,     0.384f,
       6.0f,       3.5f,     6.0f,     0.896f,
     2.126f,       6.0f,     0.3f,      0.15f,
     0.999f,       1.2f,    0.89f,       6.0f,
    };

    return BoundedReLuTestCommon<armnn::DataType::Float32>(
        workloadFactory, memoryManager, 6.0f, 0.0f, 1.0f, 0, 1.0f, 0, input, output,
        inputWidth, inputHeight, inputChannels, inputBatchSize);
}

LayerTestResult<uint8_t, 4> BoundedReLuUint8UpperBoundOnlyTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager)
{
    unsigned int inputWidth     = 3u;
    unsigned int inputHeight    = 2u;
    unsigned int inputChannels  = 1u;
    unsigned int inputBatchSize = 1;

    std::vector<uint8_t> input = std::vector<uint8_t>{
         51, 124, 28,
        251,   8, 92
    };

    // Calculated manually.
    std::vector<uint8_t> output = std::vector<uint8_t>{
          0, 122,  0,
        255,   0, 58
    };

    float inputScale     = 12.0f / 255.0f;
    int32_t inputOffset  = 63;
    float outputScale    = 6.0f / 255.0f;
    int32_t outputOffset = 0;

    return BoundedReLuTestCommon<armnn::DataType::QuantisedAsymm8>(
        workloadFactory, memoryManager, 6.0f, 0.0f,
        inputScale, inputOffset, outputScale, outputOffset,
        input, output, inputWidth, inputHeight, inputChannels, inputBatchSize);
}

LayerTestResult<uint8_t, 4> BoundedReLuUint8UpperAndLowerBoundTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager)
{
    unsigned int inputWidth     = 3u;
    unsigned int inputHeight    = 2u;
    unsigned int inputChannels  = 1u;
    unsigned int inputBatchSize = 1;

    std::vector<uint8_t> input = std::vector<uint8_t>{
         51, 230, 28,
        251,   8, 92
    };

    // Calculated manually.
    std::vector<uint8_t> output = std::vector<uint8_t>{
         51, 192, 32,
        192,  32, 92
    };

    int32_t inputOffset = 112;
    float inputScale    = 0.0125f;

    return BoundedReLuTestCommon<armnn::DataType::QuantisedAsymm8>(
        workloadFactory, memoryManager, 1.0f, -1.0f,
        inputScale, inputOffset, inputScale, inputOffset, // Input/output scale & offset same.
        input, output, inputWidth, inputHeight, inputChannels, inputBatchSize);
}

namespace
{

struct BoundedReLuRandomInputTestTraits
{
    constexpr static unsigned int inputHeight = 31u;
    constexpr static unsigned int inputWidth = 19u;
    constexpr static unsigned int inputChannels = 4u;
    constexpr static unsigned int inputBatchSize = 2;

    constexpr static unsigned int outputHeight = inputHeight;
    constexpr static unsigned int outputWidth = inputWidth;
    constexpr static unsigned int outputChannels = inputChannels;
    constexpr static unsigned int outputBatchSize = inputBatchSize;

    static armnn::TensorInfo GetInputTensorInfo()
    {
        return armnn::TensorInfo({ inputBatchSize, inputChannels, inputHeight, inputWidth },
            armnn::DataType::Float32);
    }

    static armnn::TensorInfo GetOutputTensorInfo()
    {
        return armnn::TensorInfo({ outputBatchSize, outputChannels, outputHeight, outputWidth },
            armnn::DataType::Float32);
    }
};

boost::multi_array<float, 4> BoundedReLuRandomInputTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    float lowerBound,
    float upperBound,
    const armnn::ActivationDescriptor& activationDescriptor)
{
    const armnn::TensorInfo inputTensorInfo = BoundedReLuRandomInputTestTraits::GetInputTensorInfo();
    const armnn::TensorInfo outputTensorInfo = BoundedReLuRandomInputTestTraits::GetOutputTensorInfo();

    boost::multi_array<float, 4> output(GetTensorShapeAsArray<4>(outputTensorInfo));

    // Min/max random values passed to MakeRandomTensor are purposely outside of the ReLu
    // range [lowerBound, upperBound].
    auto input = MakeRandomTensor<float, 4>(inputTensorInfo, 4605828, lowerBound - 5.0f, upperBound * 2.0f);

    std::unique_ptr<armnn::ITensorHandle> inputHandle = workloadFactory.CreateTensorHandle(inputTensorInfo);
    std::unique_ptr<armnn::ITensorHandle> outputHandle = workloadFactory.CreateTensorHandle(outputTensorInfo);

    // Set up bounded ReLu.
    armnn::ActivationQueueDescriptor descriptor;
    armnn::WorkloadInfo workloadInfo;
    AddInputToWorkload(descriptor, workloadInfo, inputTensorInfo, inputHandle.get());
    AddOutputToWorkload(descriptor, workloadInfo, outputTensorInfo, outputHandle.get());
    descriptor.m_Parameters = activationDescriptor;

    std::unique_ptr<armnn::IWorkload> workload = workloadFactory.CreateActivation(descriptor, workloadInfo);

    inputHandle->Allocate();
    outputHandle->Allocate();

    CopyDataToITensorHandle(inputHandle.get(), &input[0][0][0][0]);

    workload->Execute();

    CopyDataFromITensorHandle(&output[0][0][0][0], outputHandle.get());

    return output;
}

} // namespace

LayerTestResult<float, 4> CompareBoundedReLuTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    armnn::IWorkloadFactory& refWorkloadFactory,
    float upperBound,
    float lowerBound)
{
    LayerTestResult<float, 4> result(BoundedReLuRandomInputTestTraits::GetOutputTensorInfo());

    armnn::ActivationDescriptor activationDescriptor;
    activationDescriptor.m_Function = armnn::ActivationFunction::BoundedReLu;
    activationDescriptor.m_A = upperBound;
    activationDescriptor.m_B = lowerBound;

    result.output = BoundedReLuRandomInputTest(
        workloadFactory, memoryManager, 0.0f, upperBound, activationDescriptor);
    result.outputExpected = BoundedReLuRandomInputTest(
        refWorkloadFactory, nullptr, 0.0f, upperBound, activationDescriptor);

    return result;
}

template<armnn::DataType ArmnnType, typename T = armnn::ResolveType<ArmnnType>>
LayerTestResult<T,4> ConstantLinearActivationTestCommon(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    float qScale = 0.0f,
    int32_t qOffset = 0)
{
    unsigned int inputHeight    = 20;
    unsigned int inputWidth     = 17;
    unsigned int inputChannels  = 3;
    unsigned int batchSize      = 5;

    armnn::TensorInfo inputTensorInfo;
    armnn::TensorInfo outputTensorInfo;

    unsigned int shape[]  = {batchSize, inputChannels, inputHeight, inputWidth};

    inputTensorInfo = armnn::TensorInfo(4, shape, ArmnnType);
    outputTensorInfo = armnn::TensorInfo(4, shape, ArmnnType);

    // Set quantization parameters if the requested type is a quantized type.
    if(armnn::IsQuantizedType<T>())
    {
        inputTensorInfo.SetQuantizationScale(qScale);
        inputTensorInfo.SetQuantizationOffset(qOffset);
        outputTensorInfo.SetQuantizationScale(qScale);
        outputTensorInfo.SetQuantizationOffset(qOffset);
    }

    LayerTestResult<T, 4> ret(outputTensorInfo);

    std::unique_ptr<armnn::ITensorHandle> inputHandle = workloadFactory.CreateTensorHandle(inputTensorInfo);
    std::unique_ptr<armnn::ITensorHandle> outputHandle = workloadFactory.CreateTensorHandle(outputTensorInfo);

    // Do linear activation that should leave the tensor unchanged.
    armnn::ActivationQueueDescriptor data;
    armnn::WorkloadInfo info;
    AddInputToWorkload(data, info, inputTensorInfo, inputHandle.get());
    AddOutputToWorkload(data, info, outputTensorInfo, outputHandle.get());
    data.m_Parameters.m_A = 1.0f;
    data.m_Parameters.m_B = 0.0f;
    data.m_Parameters.m_Function = armnn::ActivationFunction::Linear;

    std::unique_ptr<armnn::IWorkload> workload = workloadFactory.CreateActivation(data, info);

    inputHandle->Allocate();
    outputHandle->Allocate();

    boost::multi_array<T, 4> input = MakeRandomTensor<T, 4>(inputTensorInfo, 7123561);
    CopyDataToITensorHandle(inputHandle.get(), &input[0][0][0][0]);

    workload->Execute();

    CopyDataFromITensorHandle(&ret.output[0][0][0][0], outputHandle.get());

    // Ensure output equals input.
    ret.outputExpected = input;

    return ret;
}

LayerTestResult<float, 4> ConstantLinearActivationTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager)
{
    return ConstantLinearActivationTestCommon<armnn::DataType::Float32>(workloadFactory, memoryManager);
}

LayerTestResult<uint8_t, 4> ConstantLinearActivationUint8Test(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager)
{
    return ConstantLinearActivationTestCommon<armnn::DataType::QuantisedAsymm8>(
        workloadFactory, memoryManager, 4.0f, 3);
}

template<armnn::DataType ArmnnType, typename T = armnn::ResolveType<ArmnnType>>
LayerTestResult<T, 4> SimpleActivationTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    armnn::ActivationFunction activationFunction,
    float activationParameterA,
    float activationParameterB,
    float qScale,
    int32_t qOffset,
    const std::vector<float>& inputData,
    const std::vector<float>& outputExpectedData)
{
    constexpr static unsigned int inputWidth = 16u;
    constexpr static unsigned int inputHeight = 1u;
    constexpr static unsigned int inputChannels = 1u;
    constexpr static unsigned int inputBatchSize = 1u;

    constexpr static unsigned int outputWidth = inputWidth;
    constexpr static unsigned int outputHeight = inputHeight;
    constexpr static unsigned int outputChannels = inputChannels;
    constexpr static unsigned int outputBatchSize = inputBatchSize;

    armnn::TensorInfo inputTensorInfo({ inputBatchSize, inputChannels, inputHeight, inputWidth }, ArmnnType);
    armnn::TensorInfo outputTensorInfo({ outputBatchSize, outputChannels, outputHeight, outputWidth }, ArmnnType);

    // Set quantization parameters if the requested type is a quantized type.
    if(armnn::IsQuantizedType<T>())
    {
        inputTensorInfo.SetQuantizationScale(qScale);
        inputTensorInfo.SetQuantizationOffset(qOffset);
        outputTensorInfo.SetQuantizationScale(qScale);
        outputTensorInfo.SetQuantizationOffset(qOffset);
    }

    LayerTestResult<T, 4> result(inputTensorInfo);

    auto input = MakeTensor<T, 4>(inputTensorInfo, QuantizedVector<T>(qScale, qOffset, inputData));

    std::unique_ptr<armnn::ITensorHandle> inputHandle = workloadFactory.CreateTensorHandle(inputTensorInfo);
    std::unique_ptr<armnn::ITensorHandle> outputHandle = workloadFactory.CreateTensorHandle(outputTensorInfo);

    // Setup bounded ReLu.
    armnn::ActivationQueueDescriptor descriptor;
    armnn::WorkloadInfo workloadInfo;
    AddInputToWorkload(descriptor, workloadInfo, inputTensorInfo, inputHandle.get());
    AddOutputToWorkload(descriptor, workloadInfo, outputTensorInfo, outputHandle.get());

    descriptor.m_Parameters.m_Function = activationFunction;
    descriptor.m_Parameters.m_A = activationParameterA;
    descriptor.m_Parameters.m_B = activationParameterB;

    std::unique_ptr<armnn::IWorkload> workload = workloadFactory.CreateActivation(descriptor, workloadInfo);

    inputHandle->Allocate();
    outputHandle->Allocate();

    CopyDataToITensorHandle(inputHandle.get(), &input[0][0][0][0]);

    workload->Execute();

    CopyDataFromITensorHandle(&result.output[0][0][0][0], outputHandle.get());

    // Calculated manually.
    result.outputExpected = MakeTensor<T, 4>(outputTensorInfo, QuantizedVector<T>(qScale, qOffset, outputExpectedData));

    return result;
}

template<armnn::DataType ArmnnType, typename T = armnn::ResolveType<ArmnnType>>
LayerTestResult<T, 4> SimpleSigmoidTestCommon(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    float qScale,
    int32_t qOffset)
{
    std::vector<float> inputData = {
        -0.1f, -0.2f, -0.3f, -0.4f,
        0.1f,  0.2f,  0.3f,  0.4f,
        -1.0f, -2.0f, -3.0f, -4.0f,
        1.0f,  2.0f,  3.0f,  4.0f
    };

    // Calculate output values for input.
    auto f = [](float value)
    {
        return 1.0f / (1.0f + std::exp(-value));
    };
    std::vector<float> outputExpectedData(inputData.size());
    std::transform(inputData.begin(), inputData.end(), outputExpectedData.begin(), f);

    return SimpleActivationTest<ArmnnType>(workloadFactory,
                                           memoryManager,
                                           armnn::ActivationFunction::Sigmoid,
                                           0.f,
                                           0.f,
                                           qScale,
                                           qOffset,
                                           inputData,
                                           outputExpectedData);
}

LayerTestResult<float, 4> SimpleSigmoidTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager)
{
    return SimpleSigmoidTestCommon<armnn::DataType::Float32>(workloadFactory, memoryManager, 0.0f, 0);
}

LayerTestResult<uint8_t, 4> SimpleSigmoidUint8Test(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager)
{
    return SimpleSigmoidTestCommon<armnn::DataType::QuantisedAsymm8>(workloadFactory, memoryManager, 0.1f, 50);
}

template<armnn::DataType ArmnnType, typename T = armnn::ResolveType<ArmnnType>>
LayerTestResult<T,4> CompareActivationTestImpl(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    armnn::IWorkloadFactory& refWorkloadFactory,
    armnn::ActivationFunction f,
    unsigned int batchSize = 5,
    float qScale = 0.0f,
    int32_t qOffset = 0)
{
    unsigned int width     = 17;
    unsigned int height    = 29;
    unsigned int channels  = 2;

    float a = 0.234f;
    float b = -12.345f;

    armnn::TensorInfo inputTensorInfo;
    armnn::TensorInfo outputTensorInfo;

    unsigned int shape[] = {batchSize, channels, height, width};

    inputTensorInfo = armnn::TensorInfo(4, shape, ArmnnType);
    outputTensorInfo = armnn::TensorInfo(4, shape, ArmnnType);

    // Set quantization parameters if the requested type is a quantized type.
    if(armnn::IsQuantizedType<T>())
    {
        inputTensorInfo.SetQuantizationScale(qScale);
        inputTensorInfo.SetQuantizationOffset(qOffset);
        outputTensorInfo.SetQuantizationScale(qScale);
        outputTensorInfo.SetQuantizationOffset(qOffset);
    }

    float minVal = -10.f;
    if (f == armnn::ActivationFunction::Sqrt)
    {
        minVal = 0.f;
    }

    boost::multi_array<T, 4> input = MakeRandomTensor<T, 4>(inputTensorInfo, 21453, minVal, 10.f);


    LayerTestResult<T,4> ret(outputTensorInfo);
    auto boostArrayExtents = boost::extents
        [boost::numeric_cast<boost::multi_array_types::extent_gen::index>(batchSize)]
    [boost::numeric_cast<boost::multi_array_types::extent_gen::index>(channels)]
    [boost::numeric_cast<boost::multi_array_types::extent_gen::index>(height)]
    [boost::numeric_cast<boost::multi_array_types::extent_gen::index>(width)];
    ret.output.resize(boostArrayExtents);
    ret.outputExpected.resize(boostArrayExtents);


    std::unique_ptr<armnn::ITensorHandle> inputHandle = workloadFactory.CreateTensorHandle(inputTensorInfo);
    std::unique_ptr<armnn::ITensorHandle> outputHandle = workloadFactory.CreateTensorHandle(outputTensorInfo);

    std::unique_ptr<armnn::ITensorHandle> inputHandleRef = refWorkloadFactory.CreateTensorHandle(inputTensorInfo);
    std::unique_ptr<armnn::ITensorHandle> outputHandleRef = refWorkloadFactory.CreateTensorHandle(outputTensorInfo);

    armnn::ActivationQueueDescriptor data;
    armnn::WorkloadInfo info;
    AddInputToWorkload(data, info, inputTensorInfo, inputHandle.get());
    AddOutputToWorkload(data, info, outputTensorInfo, outputHandle.get());
    data.m_Parameters.m_A        = a;
    data.m_Parameters.m_B        = b;
    data.m_Parameters.m_Function = f;

    armnn::ActivationQueueDescriptor refData = data;
    armnn::WorkloadInfo refInfo = info;
    SetWorkloadInput(refData, refInfo, 0, inputTensorInfo, inputHandleRef.get());
    SetWorkloadOutput(refData, refInfo, 0, outputTensorInfo, outputHandleRef.get());

    std::unique_ptr<armnn::IWorkload> workload = workloadFactory.CreateActivation(data, info);
    BOOST_ASSERT(workload != nullptr);
    std::unique_ptr<armnn::IWorkload> workloadRef = refWorkloadFactory.CreateActivation(refData, refInfo);
    BOOST_ASSERT(workloadRef != nullptr);

    inputHandle->Allocate();
    outputHandle->Allocate();
    inputHandleRef->Allocate();
    outputHandleRef->Allocate();

    CopyDataToITensorHandle(inputHandle.get(), &input[0][0][0][0]);
    CopyDataToITensorHandle(inputHandleRef.get(), &input[0][0][0][0]);

    workload->Execute();
    workloadRef->Execute();

    CopyDataFromITensorHandle(&ret.output[0][0][0][0], outputHandle.get());
    CopyDataFromITensorHandle(&ret.outputExpected[0][0][0][0], outputHandleRef.get());

    return ret;
}

LayerTestResult<float,4> CompareActivationTest(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    armnn::IWorkloadFactory& refWorkloadFactory,
    armnn::ActivationFunction f,
    unsigned int batchSize)
{
    return CompareActivationTestImpl<armnn::DataType::Float32>(
        workloadFactory, memoryManager, refWorkloadFactory, f, batchSize);
}

LayerTestResult<uint8_t,4> CompareActivationUint8Test(
    armnn::IWorkloadFactory& workloadFactory,
    const armnn::IBackendInternal::IMemoryManagerSharedPtr& memoryManager,
    armnn::IWorkloadFactory& refWorkloadFactory,
    armnn::ActivationFunction f)
{
    return CompareActivationTestImpl<armnn::DataType::QuantisedAsymm8>(
        workloadFactory, memoryManager, refWorkloadFactory, f, 5, 0.1f, 50);
}
