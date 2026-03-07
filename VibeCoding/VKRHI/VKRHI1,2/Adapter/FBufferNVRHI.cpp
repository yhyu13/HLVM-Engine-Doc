/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 * 
 * NVRHI Buffer Implementation
 */

#include "Renderer/FBufferNVRHI.h"
#include "Renderer/FDeviceManagerNVRHI.h"

#include <cstring>

/*-----------------------------------------------------------------------------
	FBufferNVRHI Implementation
-----------------------------------------------------------------------------*/

bool FBufferNVRHI::Initialize(
	TUINT32 InSizeInBytes,
	ENVRHIBufferUsage InUsage,
	nvrhi::IDevice* InDevice,
	const void* InitialData,
	nvrhi::ICommandList* CommandList)
{
	HLVM_ENSURE_MSG(!BufferHandle, TXT("Buffer already initialized"));
	HLVM_ENSURE_MSG(InSizeInBytes > 0, TXT("Buffer size must be > 0"));
	HLVM_ENSURE_MSG(InDevice, TXT("Device is null"));
	
	SizeInBytes = InSizeInBytes;
	Usage = InUsage;
	Device = InDevice;
	
	// Create buffer descriptor
	nvrhi::BufferDesc Desc;
	Desc.byteSize = SizeInBytes;
	Desc.debugName = DebugName.empty() ? "Buffer" : DebugName.c_str();
	
	// Set usage flags based on buffer type
	switch (Usage)
	{
		case ENVRHIBufferUsage::Static:
			Desc.setInitialState(nvrhi::ResourceStates::ShaderResource);
			Desc.setKeepInitialState(true);
			break;
			
		case ENVRHIBufferUsage::Dynamic:
			Desc.setCpuAccess(nvrhi::CpuAccessMode::Write);
			Desc.setInitialState(nvrhi::ResourceStates::Common);
			break;
			
		case ENVRHIBufferUsage::Transient:
			Desc.setCpuAccess(nvrhi::CpuAccessMode::Write);
			Desc.setInitialState(nvrhi::ResourceStates::Common);
			break;
	}
	
	// Create buffer
	BufferHandle = Device->createBuffer(Desc);
	HLVM_ENSURE_MSG(BufferHandle, TXT("Failed to create buffer"));
	
	// Upload initial data if provided
	if (InitialData && SizeInBytes > 0)
	{
		if (Usage == ENVRHIBufferUsage::Dynamic)
		{
			// Map and copy for dynamic buffers
			void* MappedPtr = Map(nvrhi::CpuAccessMode::Write);
			if (MappedPtr)
			{
				std::memcpy(MappedPtr, InitialData, SizeInBytes);
				Unmap();
			}
		}
		else if (CommandList)
		{
			// Use command list for static buffers
			CommandList->open();
			CommandList->beginTrackingBufferState(BufferHandle, nvrhi::ResourceStates::CopyDest);
			CommandList->writeBuffer(BufferHandle, InitialData, SizeInBytes, 0);
			
			// Set final state based on usage
			if (Usage == ENVRHIBufferUsage::Static)
			{
				CommandList->setPermanentBufferState(BufferHandle, nvrhi::ResourceStates::ShaderResource);
			}
			
			CommandList->close();
			Device->executeCommandList(CommandList);
		}
	}
	
	return true;
}

void* FBufferNVRHI::Map(nvrhi::CpuAccessMode AccessMode)
{
	HLVM_ENSURE_MSG(BufferHandle, TXT("Buffer not initialized"));
	HLVM_ENSURE_MSG(Usage == ENVRHIBufferUsage::Dynamic || Usage == ENVRHIBufferUsage::Transient, 
		TXT("Map() only for dynamic/transient buffers"));
	HLVM_ENSURE_MSG(!bIsMapped, TXT("Buffer already mapped"));
	
	MappedData = Device->mapBuffer(BufferHandle, AccessMode, { 0, SizeInBytes });
	HLVM_ENSURE_MSG(MappedData, TXT("Failed to map buffer"));
	
	bIsMapped = true;
	return MappedData;
}

void FBufferNVRHI::Unmap()
{
	HLVM_ENSURE_MSG(bIsMapped, TXT("Buffer not mapped"));
	
	Device->unmapBuffer(BufferHandle);
	MappedData = nullptr;
	bIsMapped = false;
}

void FBufferNVRHI::Update(
	const void* Data,
	TUINT32 DataSize,
	TUINT32 Offset,
	bool bInitialUpdate,
	nvrhi::ICommandList* CommandList)
{
	HLVM_ENSURE_MSG(BufferHandle, TXT("Buffer not initialized"));
	HLVM_ENSURE_MSG(Data, TXT("Data is null"));
	HLVM_ENSURE_MSG(Offset + DataSize <= SizeInBytes, TXT("Update exceeds buffer size"));
	
	if (Usage == ENVRHIBufferUsage::Dynamic && bIsMapped)
	{
		// Direct copy if mapped
		std::memcpy(static_cast<byte*>(MappedData) + Offset, Data, DataSize);
	}
	else if (CommandList)
	{
		// Use command list for updates
		if (bInitialUpdate && Usage == ENVRHIBufferUsage::Static)
		{
			CommandList->beginTrackingBufferState(BufferHandle, nvrhi::ResourceStates::CopyDest);
		}
		
		CommandList->writeBuffer(BufferHandle, Data, DataSize, Offset);
		
		if (bInitialUpdate && Usage == ENVRHIBufferUsage::Static)
		{
			CommandList->setPermanentBufferState(BufferHandle, nvrhi::ResourceStates::ShaderResource);
		}
	}
	else
	{
		HLVM_LOG(LogRHI, err, TXT("FBufferNVRHI::Update - No command list provided for static buffer update"));
	}
}

void FBufferNVRHI::SetDebugName(const TCHAR* Name)
{
	DebugName = Name;
	if (BufferHandle)
	{
		Device->setObjectDebugName(BufferHandle, DebugName.c_str());
	}
}

/*-----------------------------------------------------------------------------
	FUniformBufferNVRHI Implementation
-----------------------------------------------------------------------------*/

bool FUniformBufferNVRHI::InitializeAligned(
	TUINT32 InSizeInBytes,
	TUINT32 Alignment,
	ENVRHIBufferUsage InUsage,
	nvrhi::IDevice* InDevice,
	const void* InitialData,
	nvrhi::ICommandList* CommandList)
{
	// Align size to uniform buffer requirements
	const TUINT32 AlignedSize = (InSizeInBytes + Alignment - 1) & ~(Alignment - 1);
	
	return Initialize(AlignedSize, InUsage, InDevice, InitialData, CommandList);
}
