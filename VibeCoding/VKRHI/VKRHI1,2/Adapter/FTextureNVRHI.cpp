/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 * 
 * NVRHI Texture Implementation
 */

#include "Renderer/FTextureNVRHI.h"

#include <cstring>

/*-----------------------------------------------------------------------------
	Helper Functions
-----------------------------------------------------------------------------*/

static nvrhi::Format ConvertTextureFormat(ENVRHITextureFormat Format)
{
	switch (Format)
	{
		case ENVRHITextureFormat::R8:
			return nvrhi::Format::R8_UNORM;
		case ENVRHITextureFormat::RG8:
			return nvrhi::Format::RG8_UNORM;
		case ENVRHITextureFormat::RGBA8:
			return nvrhi::Format::RGBA8_UNORM;
		case ENVRHITextureFormat::SRGBA8:
			return nvrhi::Format::SRGBA8_UNORM;
			
		case ENVRHITextureFormat::D16:
			return nvrhi::Format::D16;
		case ENVRHITextureFormat::D24S8:
			return nvrhi::Format::D24S8;
		case ENVRHITextureFormat::D32:
			return nvrhi::Format::D32;
		case ENVRHITextureFormat::D32S8:
			return nvrhi::Format::D32S8;
			
		case ENVRHITextureFormat::BC1:
			return nvrhi::Format::BC1_UNORM;
		case ENVRHITextureFormat::BC3:
			return nvrhi::Format::BC3_UNORM;
		case ENVRHITextureFormat::BC6H:
			return nvrhi::Format::BC6H_UFLOAT;
		case ENVRHITextureFormat::BC7:
			return nvrhi::Format::BC7_UNORM;
			
		case ENVRHITextureFormat::R16F:
			return nvrhi::Format::R16_FLOAT;
		case ENVRHITextureFormat::RG16F:
			return nvrhi::Format::RG16_FLOAT;
		case ENVRHITextureFormat::RGBA16F:
			return nvrhi::Format::RGBA16_FLOAT;
		case ENVRHITextureFormat::R32F:
			return nvrhi::Format::R32_FLOAT;
		case ENVRHITextureFormat::RGBA32F:
			return nvrhi::Format::RGBA32_FLOAT;
			
		default:
			HLVM_LOG(LogRHI, err, TXT("FTextureNVRHI::ConvertTextureFormat - Unknown format"));
			return nvrhi::Format::UNKNOWN;
	}
}

static nvrhi::TextureDimension ConvertTextureDimension(ENVRHITextureDimension Dimension)
{
	switch (Dimension)
	{
		case ENVRHITextureDimension::Texture2D:
			return nvrhi::TextureDimension::Texture2D;
		case ENVRHITextureDimension::Texture2DArray:
			return nvrhi::TextureDimension::Texture2DArray;
		case ENVRHITextureDimension::Texture3D:
			return nvrhi::TextureDimension::Texture3D;
		case ENVRHITextureDimension::TextureCube:
			return nvrhi::TextureDimension::TextureCube;
		case ENVRHITextureDimension::TextureCubeArray:
			return nvrhi::TextureDimension::TextureCubeArray;
		default:
			return nvrhi::TextureDimension::Texture2D;
	}
}

/*-----------------------------------------------------------------------------
	FTextureNVRHI Implementation
-----------------------------------------------------------------------------*/

bool FTextureNVRHI::Initialize(
	TUINT32 InWidth,
	TUINT32 InHeight,
	TUINT32 InMipLevels,
	ENVRHITextureFormat InFormat,
	ENVRHITextureDimension InDimension,
	nvrhi::IDevice* InDevice,
	const void* InitialData,
	nvrhi::ICommandList* CommandList)
{
	HLVM_ENSURE_MSG(!TextureHandle, TXT("Texture already initialized"));
	HLVM_ENSURE_MSG(InWidth > 0 && InHeight > 0, TXT("Invalid texture dimensions"));
	HLVM_ENSURE_MSG(InDevice, TXT("Device is null"));
	
	Width = InWidth;
	Height = InHeight;
	MipLevels = InMipLevels > 0 ? InMipLevels : 1;
	ArraySize = 1;
	SampleCount = 1;
	Format = InFormat;
	Dimension = InDimension;
	Device = InDevice;
	
	// Create texture descriptor
	nvrhi::TextureDesc Desc;
	Desc.setDimension(ConvertTextureDimension(Dimension));
	Desc.setWidth(Width);
	Desc.setHeight(Height);
	Desc.setFormat(ConvertTextureFormat(Format));
	Desc.setMipLevels(MipLevels);
	Desc.setArraySize(ArraySize);
	Desc.setSampleCount(SampleCount);
	Desc.debugName = DebugName.empty() ? "Texture" : DebugName.c_str();
	
	// Set usage flags
	Desc.setInitialState(nvrhi::ResourceStates::ShaderResource);
	Desc.setKeepInitialState(InitialData == nullptr);
	
	// Create texture
	TextureHandle = Device->createTexture(Desc);
	HLVM_ENSURE_MSG(TextureHandle, TXT("Failed to create texture"));
	
	// Create views
	CreateViews();
	
	// Upload initial data if provided
	if (InitialData)
	{
		Update(InitialData, Width * Height * 4, 0, 0, CommandList);
	}
	
	return true;
}

bool FTextureNVRHI::InitializeRenderTarget(
	TUINT32 InWidth,
	TUINT32 InHeight,
	ENVRHITextureFormat InFormat,
	nvrhi::IDevice* InDevice,
	TUINT32 SampleCount)
{
	HLVM_ENSURE_MSG(!TextureHandle, TXT("Texture already initialized"));
	HLVM_ENSURE_MSG(InWidth > 0 && InHeight > 0, TXT("Invalid texture dimensions"));
	HLVM_ENSURE_MSG(InDevice, TXT("Device is null"));
	
	Width = InWidth;
	Height = InHeight;
	MipLevels = 1;
	ArraySize = 1;
	SampleCount = SampleCount > 0 ? SampleCount : 1;
	Format = InFormat;
	Dimension = ENVRHITextureDimension::Texture2D;
	Device = InDevice;
	
	// Create texture descriptor for render target
	nvrhi::TextureDesc Desc;
	Desc.setDimension(nvrhi::TextureDimension::Texture2D);
	Desc.setWidth(Width);
	Desc.setHeight(Height);
	Desc.setFormat(ConvertTextureFormat(Format));
	Desc.setMipLevels(1);
	Desc.setArraySize(1);
	Desc.setSampleCount(SampleCount);
	Desc.setIsRenderTarget(true);
	Desc.setInitialState(nvrhi::ResourceStates::RenderTarget);
	Desc.setKeepInitialState(true);
	Desc.debugName = DebugName.empty() ? "RenderTarget" : DebugName.c_str();
	
	// Create texture
	TextureHandle = Device->createTexture(Desc);
	HLVM_ENSURE_MSG(TextureHandle, TXT("Failed to create render target texture"));
	
	// Create RTV
	nvrhi::TextureDesc RTVDesc = Desc;
	RTVDesc.setInitialState(nvrhi::ResourceStates::RenderTarget);
	TextureRTV = Device->createTexture(RTVDesc);
	
	return true;
}

void FTextureNVRHI::CreateViews()
{
	if (!TextureHandle)
	{
		return;
	}
	
	// Create SRV if not a depth format
	const bool bIsDepth = (Format == ENVRHITextureFormat::D16 || 
						   Format == ENVRHITextureFormat::D24S8 ||
						   Format == ENVRHITextureFormat::D32 ||
						   Format == ENVRHITextureFormat::D32S8);
	
	if (!bIsDepth)
	{
		TextureSRV = TextureHandle; // Use same handle for SRV
	}
	
	// RTV and DSV would be created here if needed
	// For now, we use the main handle
}

nvrhi::SamplerHandle FTextureNVRHI::GetSampler(ENVRHITextureFilter Filter) const
{
	// Check cache first
	if (SamplerCache.Contains(Filter))
	{
		return SamplerCache[Filter];
	}
	
	// Create new sampler
	nvrhi::SamplerDesc Desc;
	Desc.setDebugName(DebugName.c_str());
	
	// Set filter
	switch (Filter)
	{
		case ENVRHITextureFilter::Nearest:
			Desc.setMinFilter(false).setMagFilter(false).setMipFilter(false);
			break;
		case ENVRHITextureFilter::Linear:
			Desc.setMinFilter(true).setMagFilter(true).setMipFilter(false);
			break;
		case ENVRHITextureFilter::NearestMipmapNearest:
			Desc.setMinFilter(false).setMagFilter(false).setMipFilter(false);
			break;
		case ENVRHITextureFilter::NearestMipmapLinear:
			Desc.setMinFilter(false).setMagFilter(false).setMipFilter(true);
			break;
		case ENVRHITextureFilter::LinearMipmapNearest:
			Desc.setMinFilter(true).setMagFilter(true).setMipFilter(false);
			break;
		case ENVRHITextureFilter::LinearMipmapLinear:
			Desc.setMinFilter(true).setMagFilter(true).setMipFilter(true);
			break;
		case ENVRHITextureFilter::Anisotropic:
			Desc.setMinFilter(true).setMagFilter(true).setMipFilter(true);
			Desc.setMaxAnisotropy(16.0f);
			break;
	}
	
	// Default address modes
	Desc.setAddressU(nvrhi::SamplerAddressMode::ClampToEdge);
	Desc.setAddressV(nvrhi::SamplerAddressMode::ClampToEdge);
	Desc.setAddressW(nvrhi::SamplerAddressMode::ClampToEdge);
	
	// Create sampler
	nvrhi::SamplerHandle Sampler = Device->createSampler(Desc);
	if (Sampler)
	{
		SamplerCache.Add(Filter, Sampler);
	}
	
	return Sampler;
}

void FTextureNVRHI::Update(
	const void* Data,
	TUINT32 DataSize,
	TUINT32 MipLevel,
	TUINT32 ArraySlice,
	nvrhi::ICommandList* CommandList)
{
	HLVM_ENSURE_MSG(TextureHandle, TXT("Texture not initialized"));
	HLVM_ENSURE_MSG(Data, TXT("Data is null"));
	HLVM_ENSURE_MSG(CommandList, TXT("CommandList is null"));
	
	CommandList->open();
	CommandList->beginTrackingTextureState(TextureHandle, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
	
	nvrhi::TextureSubresourceSubset Subresources(MipLevel, MipLevel + 1, ArraySlice, ArraySlice + 1);
	CommandList->writeTexture(TextureHandle, Data, DataSize, Subresources);
	
	CommandList->setPermanentTextureState(TextureHandle, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
	CommandList->close();
	
	Device->executeCommandList(CommandList);
}

void FTextureNVRHI::GenerateMipmaps(nvrhi::ICommandList* CommandList)
{
	HLVM_ENSURE_MSG(TextureHandle, TXT("Texture not initialized"));
	HLVM_ENSURE_MSG(MipLevels > 1, TXT("Texture has only 1 mip level"));
	
	// NVRHI doesn't have built-in mipmap generation
	// This would require a compute shader or blit commands
	// For now, just log a message
	HLVM_LOG(LogRHI, warn, TXT("FTextureNVRHI::GenerateMipmaps - Not implemented, requires compute shader"));
}

void FTextureNVRHI::SetDebugName(const TCHAR* Name)
{
	DebugName = Name;
	if (TextureHandle)
	{
		Device->setObjectDebugName(TextureHandle, DebugName.c_str());
	}
}

/*-----------------------------------------------------------------------------
	FSamplerNVRHI Implementation
-----------------------------------------------------------------------------*/

bool FSamplerNVRHI::Initialize(
	ENVRHITextureFilter Filter,
	ENVRHITextureAddress AddressU,
	ENVRHITextureAddress AddressV,
	ENVRHITextureAddress AddressW,
	nvrhi::IDevice* InDevice,
	TFLOAT MaxAnisotropy)
{
	HLVM_ENSURE_MSG(!SamplerHandle, TXT("Sampler already initialized"));
	HLVM_ENSURE_MSG(InDevice, TXT("Device is null"));
	
	Device = InDevice;
	
	// Create sampler descriptor
	nvrhi::SamplerDesc Desc;
	Desc.setDebugName("Sampler");
	
	// Set filter
	switch (Filter)
	{
		case ENVRHITextureFilter::Nearest:
			Desc.setMinFilter(false).setMagFilter(false).setMipFilter(false);
			break;
		case ENVRHITextureFilter::Linear:
			Desc.setMinFilter(true).setMagFilter(true).setMipFilter(false);
			break;
		case ENVRHITextureFilter::NearestMipmapNearest:
			Desc.setMinFilter(false).setMagFilter(false).setMipFilter(false);
			break;
		case ENVRHITextureFilter::NearestMipmapLinear:
			Desc.setMinFilter(false).setMagFilter(false).setMipFilter(true);
			break;
		case ENVRHITextureFilter::LinearMipmapNearest:
			Desc.setMinFilter(true).setMagFilter(true).setMipFilter(false);
			break;
		case ENVRHITextureFilter::LinearMipmapLinear:
			Desc.setMinFilter(true).setMagFilter(true).setMipFilter(true);
			break;
		case ENVRHITextureFilter::Anisotropic:
			Desc.setMinFilter(true).setMagFilter(true).setMipFilter(true);
			Desc.setMaxAnisotropy(MaxAnisotropy);
			break;
	}
	
	// Set address modes
	auto ConvertAddress = [](ENVRHITextureAddress Address) -> nvrhi::SamplerAddressMode
	{
		switch (Address)
		{
			case ENVRHITextureAddress::Wrap:
				return nvrhi::SamplerAddressMode::Repeat;
			case ENVRHITextureAddress::Mirror:
				return nvrhi::SamplerAddressMode::Mirror;
			case ENVRHITextureAddress::Clamp:
				return nvrhi::SamplerAddressMode::ClampToEdge;
			case ENVRHITextureAddress::Border:
				return nvrhi::SamplerAddressMode::ClampToBorder;
			case ENVRHITextureAddress::MirrorOnce:
				return nvrhi::SamplerAddressMode::MirrorOnce;
			default:
				return nvrhi::SamplerAddressMode::ClampToEdge;
		}
	};
	
	Desc.setAddressU(ConvertAddress(AddressU));
	Desc.setAddressV(ConvertAddress(AddressV));
	Desc.setAddressW(ConvertAddress(AddressW));
	
	// Create sampler
	SamplerHandle = Device->createSampler(Desc);
	HLVM_ENSURE_MSG(SamplerHandle, TXT("Failed to create sampler"));
	
	return true;
}
