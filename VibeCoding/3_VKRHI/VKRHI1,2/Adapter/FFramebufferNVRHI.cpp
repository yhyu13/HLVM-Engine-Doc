/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 * 
 * NVRHI Framebuffer Implementation
 */

#include "Renderer/FFramebufferNVRHI.h"

/*-----------------------------------------------------------------------------
	FFramebufferNVRHI Implementation
-----------------------------------------------------------------------------*/

bool FFramebufferNVRHI::Initialize(nvrhi::IDevice* InDevice)
{
	HLVM_ENSURE_MSG(!FramebufferHandle, TXT("Framebuffer already initialized"));
	HLVM_ENSURE_MSG(InDevice, TXT("Device is null"));
	
	Device = InDevice;
	return true;
}

void FFramebufferNVRHI::AddColorAttachment(const FFramebufferAttachment& Attachment)
{
	HLVM_ENSURE_MSG(Attachment.Texture, TXT("Attachment texture is null"));
	
	ColorAttachments.Add(Attachment);
	
	// Update dimensions from first attachment
	if (ColorAttachments.Num() == 1 && Attachment.Texture)
	{
		const auto Info = Attachment.Texture->getDesc();
		Width = info.width;
		Height = info.height;
	}
}

void FFramebufferNVRHI::SetDepthAttachment(const FFramebufferAttachment& Attachment)
{
	HLVM_ENSURE_MSG(Attachment.Texture, TXT("Depth attachment texture is null"));
	
	DepthAttachment = Attachment;
	
	// Update dimensions if not set
	if (Width == 0 || Height == 0)
	{
		const auto Info = Attachment.Texture->getDesc();
		Width = info.width;
		Height = info.height;
	}
}

bool FFramebufferNVRHI::CreateFramebuffer()
{
	HLVM_ENSURE_MSG(Device, TXT("Device not initialized"));
	HLVM_ENSURE_MSG(ColorAttachments.Num() > 0, TXT("No color attachments"));
	
	// Build framebuffer descriptor
	nvrhi::FramebufferDesc Desc;
	Desc.debugName = DebugName.empty() ? "Framebuffer" : DebugName.c_str();
	
	// Add color attachments
	for (const auto& Attachment : ColorAttachments)
	{
		nvrhi::FramebufferAttachment ColorAttach;
		ColorAttach.setTexture(Attachment.Texture.Get());
		ColorAttach.setMipLevel(Attachment.MipLevel);
		ColorAttach.setArraySlice(Attachment.ArraySlice);
		
		Desc.addColorAttachment(ColorAttach);
	}
	
	// Add depth attachment if present
	if (DepthAttachment.Texture)
	{
		nvrhi::FramebufferAttachment DepthAttach;
		DepthAttach.setTexture(DepthAttachment.Texture.Get());
		DepthAttach.setMipLevel(DepthAttachment.MipLevel);
		DepthAttach.setArraySlice(DepthAttachment.ArraySlice);
		
		Desc.setDepthAttachment(DepthAttach);
	}
	
	// Create framebuffer
	FramebufferHandle = Device->createFramebuffer(Desc);
	HLVM_ENSURE_MSG(FramebufferHandle, TXT("Failed to create framebuffer"));
	
	// Set default viewport and scissor
	SetViewport(0, 0, static_cast<TFLOAT>(Width), static_cast<TFLOAT>(Height));
	SetScissor(0, 0, Width, Height);
	
	return true;
}

void FFramebufferNVRHI::SetViewport(TFLOAT X, TFLOAT Y, TFLOAT InWidth, TFLOAT InHeight, TFLOAT MinDepth, TFLOAT MaxDepth)
{
	Viewport.minX = X;
	Viewport.minY = Y;
	Viewport.maxX = X + InWidth;
	Viewport.maxY = Y + InHeight;
	Viewport.minZ = MinDepth;
	Viewport.maxZ = MaxDepth;
}

void FFramebufferNVRHI::SetScissor(TINT32 X, TINT32 Y, TUINT32 InWidth, TUINT32 InHeight)
{
	Scissor.x0 = X;
	Scissor.y0 = Y;
	Scissor.x1 = X + static_cast<TINT32>(InWidth);
	Scissor.y1 = Y + static_cast<TINT32>(InHeight);
}

void FFramebufferNVRHI::SetDebugName(const TCHAR* Name)
{
	DebugName = Name;
	if (FramebufferHandle)
	{
		Device->setObjectDebugName(FramebufferHandle, DebugName.c_str());
	}
}

/*-----------------------------------------------------------------------------
	FFramebufferManager Implementation
-----------------------------------------------------------------------------*/

void FFramebufferManager::Initialize(nvrhi::IDevice* InDevice)
{
	HLVM_ENSURE_MSG(InDevice, TXT("Device is null"));
	Device = InDevice;
}

FFramebufferNVRHI* FFramebufferManager::CreateFramebuffer(const TCHAR* Name)
{
	HLVM_ENSURE_MSG(Device, TXT("Manager not initialized"));
	HLVM_ENSURE_MSG(Name, TXT("Name is null"));
	
	// Check if already exists
	if (FramebufferPool.Contains(Name))
	{
		HLVM_LOG(LogRHI, warn, TXT("FFramebufferManager::CreateFramebuffer - Framebuffer '%s' already exists"), Name);
		return FramebufferPool[Name].Get();
	}
	
	// Create new framebuffer
	auto Framebuffer = TUniquePtr<FFramebufferNVRHI>(new FFramebufferNVRHI());
	Framebuffer->Initialize(Device);
	Framebuffer->SetDebugName(Name);
	
	// Add to pool
	FFramebufferNVRHI* RawPtr = Framebuffer.Get();
	FramebufferPool.Add(Name, MoveTemp(Framebuffer));
	
	return RawPtr;
}

FFramebufferNVRHI* FFramebufferManager::FindFramebuffer(const TCHAR* Name)
{
	HLVM_ENSURE_MSG(Name, TXT("Name is null"));
	
	if (FramebufferPool.Contains(Name))
	{
		return FramebufferPool[Name].Get();
	}
	
	return nullptr;
}

void FFramebufferManager::RemoveFramebuffer(const TCHAR* Name)
{
	HLVM_ENSURE_MSG(Name, TXT("Name is null"));
	
	FramebufferPool.Remove(Name);
}

void FFramebufferManager::RemoveAllFramebuffers()
{
	FramebufferPool.Empty();
}

FFramebufferManager::~FFramebufferManager()
{
	RemoveAllFramebuffers();
}
