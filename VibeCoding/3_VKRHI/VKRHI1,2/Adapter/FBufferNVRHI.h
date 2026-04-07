/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 * 
 * NVRHI Buffer Objects
 * 
 * Vertex, Index, and Uniform buffer management using NVRHI.
 * Borrowed from RBDOOM-3-BFG's BufferObject_NVRHI with HLVM coding style.
 */

#pragma once

#include "Core/Log.h"
#include "Core/Assert.h"
#include "Renderer/RHI/Common.h"

#include <nvrhi/nvrhi.h>

/*-----------------------------------------------------------------------------
	Forward Declarations
-----------------------------------------------------------------------------*/

class FNVRHIDeviceManager;

/*-----------------------------------------------------------------------------
	Buffer Usage Types
-----------------------------------------------------------------------------*/

enum class ENVRHIBufferUsage : TUINT8
{
	Static,		// GPU-only, immutable after creation
	Dynamic,	// CPU-write, GPU-read (frequent updates)
	Transient,	// Short-lived, frequently discarded
};

/*-----------------------------------------------------------------------------
	FBufferNVRHI - Base Buffer Class
-----------------------------------------------------------------------------*/

/**
 * Base class for NVRHI buffer objects
 * 
 * Manages:
 * - NVRHI buffer handle
 * - Buffer size and usage
 * - CPU mapping for dynamic buffers
 * 
 * Usage:
 * 1. Create instance (FVertexBufferNVRHI, FIndexBufferNVRHI, or FUniformBufferNVRHI)
 * 2. Call Initialize() to allocate
 * 3. For dynamic buffers: Map(), write data, Unmap()
 * 4. For static buffers: use Update() with command list
 */
class FBufferNVRHI
{
public:
	NOCOPYMOVE(FBufferNVRHI)
	
	FBufferNVRHI();
	virtual ~FBufferNVRHI();
	
	// Initialization
	virtual bool Initialize(
		TUINT32 SizeInBytes,
		ENVRHIBufferUsage Usage,
		nvrhi::IDevice* Device,
		const void* InitialData = nullptr,
		nvrhi::ICommandList* CommandList = nullptr);
	
	// Resource access
	[[nodiscard]] nvrhi::BufferHandle GetBufferHandle() const { return BufferHandle; }
	[[nodiscard]] TUINT32 GetSize() const { return SizeInBytes; }
	[[nodiscard]] ENVRHIBufferUsage GetUsage() const { return Usage; }
	
	// CPU mapping (for dynamic buffers only)
	void* Map(nvrhi::CpuAccessMode AccessMode = nvrhi::CpuAccessMode::Write);
	void Unmap();
	[[nodiscard]] bool IsMapped() const { return bIsMapped; }
	
	// Data updates
	void Update(
		const void* Data,
		TUINT32 DataSize,
		TUINT32 Offset,
		bool bInitialUpdate,
		nvrhi::ICommandList* CommandList);
	
	// Debug name
	void SetDebugName(const TCHAR* Name);
	
protected:
	nvrhi::BufferHandle		BufferHandle;
	TUINT32					SizeInBytes;
	ENVRHIBufferUsage		Usage;
	TString<64>				DebugName;
	bool					bIsMapped;
	void*					MappedData;
	
	nvrhi::IDevice*			Device;
};

/*-----------------------------------------------------------------------------
	FVertexBufferNVRHI - Vertex Buffer
-----------------------------------------------------------------------------*/

/**
 * Vertex buffer for storing vertex data
 * 
 * Usage:
 * ```cpp
 * FVertexBufferNVRHI VertexBuffer;
 * VertexBuffer.Initialize(NumVertices * sizeof(FVertex), ENVRHIBufferUsage::Static, Device, Vertices, CommandList);
 * 
 * // In render loop:
 * CommandList->bindVertexBuffers(0, &VertexBuffer.GetBufferHandle().Get(), 1);
 * ```
 */
class FVertexBufferNVRHI final : public FBufferNVRHI
{
public:
	FVertexBufferNVRHI() = default;
	
	// Helper to get vertex buffer binding info
	[[nodiscard]] nvrhi::VertexBufferBinding GetBinding(TUINT32 Slot = 0, TUINT32 Offset = 0) const
	{
		nvrhi::VertexBufferBinding Binding;
		Binding.setIndex(Slot);
		Binding.setBuffer(BufferHandle.Get());
		Binding.setOffset(Offset);
		return Binding;
	}
};

/*-----------------------------------------------------------------------------
	FIndexBufferNVRHI - Index Buffer
-----------------------------------------------------------------------------*/

/**
 * Index buffer for storing index data (triangle lists)
 * 
 * Supports 16-bit and 32-bit indices
 * 
 * Usage:
 * ```cpp
 * FIndexBufferNVRHI IndexBuffer;
 * IndexBuffer.Initialize(NumIndices * sizeof(TUINT32), ENVRHIBufferUsage::Static, Device, Indices, CommandList);
 * 
 * // In render loop:
 * CommandList->bindIndexBuffer(IndexBuffer.GetBufferHandle().Get(), nvrhi::Format::R32_UINT, 0);
 * ```
 */
class FIndexBufferNVRHI final : public FBufferNVRHI
{
public:
	FIndexBufferNVRHI() = default;
	
	// Get index format based on buffer size
	[[nodiscard]] nvrhi::Format GetIndexFormat() const
	{
		// Assuming 32-bit indices if size is multiple of 4, else 16-bit
		return (SizeInBytes % 4 == 0) ? nvrhi::Format::R32_UINT : nvrhi::Format::R16_UINT;
	}
	
	// Get number of indices
	[[nodiscard]] TUINT32 GetIndexCount() const
	{
		const nvrhi::Format Format = GetIndexFormat();
		if (Format == nvrhi::Format::R32_UINT)
		{
			return SizeInBytes / sizeof(TUINT32);
		}
		else
		{
			return SizeInBytes / sizeof(TUINT16);
		}
	}
};

/*-----------------------------------------------------------------------------
	FUniformBufferNVRHI - Uniform/Constant Buffer
-----------------------------------------------------------------------------*/

/**
 * Uniform buffer for storing shader constants (UBO/Push Constants)
 * 
 * Automatically handles alignment requirements
 * 
 * Usage:
 * ```cpp
 * FUniformBufferNVRHI UniformBuffer;
 * UniformBuffer.Initialize(sizeof(FShaderConstants), ENVRHIBufferUsage::Dynamic, Device);
 * 
 * // Update per-frame:
 * auto* Constants = static_cast<FShaderConstants*>(UniformBuffer.Map());
 * Constants->ViewMatrix = View;
 * Constants->ProjectionMatrix = Projection;
 * UniformBuffer.Unmap();
 * 
 * CommandList->bindConstantBuffers(0, &UniformBuffer.GetBufferHandle().Get(), 1);
 * ```
 */
class FUniformBufferNVRHI final : public FBufferNVRHI
{
public:
	FUniformBufferNVRHI() = default;
	
	// Initialize with automatic alignment
	bool InitializeAligned(
		TUINT32 SizeInBytes,
		TUINT32 Alignment, // e.g., 256 for Vulkan uniform buffer offset alignment
		ENVRHIBufferUsage Usage,
		nvrhi::IDevice* Device,
		const void* InitialData = nullptr,
		nvrhi::ICommandList* CommandList = nullptr);
};

/*-----------------------------------------------------------------------------
	Inline Implementations
-----------------------------------------------------------------------------*/

FORCEINLINE FBufferNVRHI::FBufferNVRHI()
	: SizeInBytes(0)
	, Usage(ENVRHIBufferUsage::Static)
	, bIsMapped(false)
	, MappedData(nullptr)
	, Device(nullptr)
{
}

FORCEINLINE FBufferNVRHI::~FBufferNVRHI()
{
	if (bIsMapped)
	{
		Unmap();
	}
	BufferHandle.Reset();
}
