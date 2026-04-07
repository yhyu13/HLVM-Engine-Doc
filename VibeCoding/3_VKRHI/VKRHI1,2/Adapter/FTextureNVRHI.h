/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 * 
 * NVRHI Texture Objects
 * 
 * Texture/Image management using NVRHI.
 * Borrowed from RBDOOM-3-BFG's Image_NVRHI with HLVM coding style.
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
	Texture Dimension Types
-----------------------------------------------------------------------------*/

enum class ENVRHITextureDimension : TUINT8
{
	Texture2D,
	Texture2DArray,
	Texture3D,
	TextureCube,
	TextureCubeArray,
};

/*-----------------------------------------------------------------------------
	Texture Format
-----------------------------------------------------------------------------*/

enum class ENVRHITextureFormat : TUINT8
{
	// Color formats
	R8,
	RG8,
	RGBA8,
	SRGBA8,
	
	// Depth formats
	D16,
	D24S8,
	D32,
	D32S8,
	
	// Compressed formats
	BC1,
	BC3,
	BC6H,
	BC7,
	
	// Float formats
	R16F,
	RG16F,
	RGBA16F,
	R32F,
	RGBA32F,
};

/*-----------------------------------------------------------------------------
	Texture Filter Modes
-----------------------------------------------------------------------------*/

enum class ENVRHITextureFilter : TUINT8
{
	Nearest,
	Linear,
	NearestMipmapNearest,
	NearestMipmapLinear,
	LinearMipmapNearest,
	LinearMipmapLinear,
	Anisotropic,
};

/*-----------------------------------------------------------------------------
	Texture Address Modes
-----------------------------------------------------------------------------*/

enum class ENVRHITextureAddress : TUINT8
{
	Wrap,
	Mirror,
	Clamp,
	Border,
	MirrorOnce,
};

/*-----------------------------------------------------------------------------
	FTextureNVRHI - Main Texture Class
-----------------------------------------------------------------------------*/

/**
 * Texture resource for NVRHI rendering
 * 
 * Manages:
 * - NVRHI texture handle and views
 * - Sampler state
 * - Mipmap generation
 * - Texture uploads
 * 
 * Usage:
 * 1. Create instance
 * 2. Call Initialize() or InitializeRenderTarget()
 * 3. Use GetTextureHandle() for shader bindings
 * 4. Use GetSampler() for sampler bindings
 */
class FTextureNVRHI
{
public:
	NOCOPYMOVE(FTextureNVRHI)
	
	FTextureNVRHI();
	virtual ~FTextureNVRHI();
	
	// Initialization
	bool Initialize(
		TUINT32 Width,
		TUINT32 Height,
		TUINT32 MipLevels,
		ENVRHITextureFormat Format,
		ENVRHITextureDimension Dimension,
		nvrhi::IDevice* Device,
		const void* InitialData = nullptr,
		nvrhi::ICommandList* CommandList = nullptr);
	
	// Render target initialization
	bool InitializeRenderTarget(
		TUINT32 Width,
		TUINT32 Height,
		ENVRHITextureFormat Format,
		nvrhi::IDevice* Device,
		TUINT32 SampleCount = 1);
	
	// Resource access
	[[nodiscard]] nvrhi::TextureHandle GetTextureHandle() const { return TextureHandle; }
	[[nodiscard]] nvrhi::TextureHandle GetTextureRTV() const { return TextureRTV; }
	[[nodiscard]] nvrhi::TextureHandle GetTextureDSV() const { return TextureDSV; }
	[[nodiscard]] nvrhi::TextureHandle GetTextureSRV() const { return TextureSRV; }
	[[nodiscard]] nvrhi::TextureHandle GetTextureUAV() const { return TextureUAV; }
	
	// Sampler access
	[[nodiscard]] nvrhi::SamplerHandle GetSampler(ENVRHITextureFilter Filter = ENVRHITextureFilter::Linear) const;
	
	// Properties
	[[nodiscard]] TUINT32 GetWidth() const { return Width; }
	[[nodiscard]] TUINT32 GetHeight() const { return Height; }
	[[nodiscard]] TUINT32 GetMipLevels() const { return MipLevels; }
	[[nodiscard]] ENVRHITextureFormat GetFormat() const { return Format; }
	[[nodiscard]] ENVRHITextureDimension GetDimension() const { return Dimension; }
	
	// Texture upload
	void Update(
		const void* Data,
		TUINT32 DataSize,
		TUINT32 MipLevel,
		TUINT32 ArraySlice,
		nvrhi::ICommandList* CommandList);
	
	// Generate mipmaps
	void GenerateMipmaps(nvrhi::ICommandList* CommandList);
	
	// Debug name
	void SetDebugName(const TCHAR* Name);
	
protected:
	nvrhi::TextureHandle		TextureHandle;
	nvrhi::TextureHandle		TextureRTV;		// Render target view
	nvrhi::TextureHandle		TextureDSV;		// Depth stencil view
	nvrhi::TextureHandle		TextureSRV;		// Shader resource view
	nvrhi::TextureHandle		TextureUAV;		// Unordered access view
	
	TUINT32						Width;
	TUINT32						Height;
	TUINT32						MipLevels;
	TUINT32						ArraySize;
	TUINT32						SampleCount;
	ENVRHITextureFormat			Format;
	ENVRHITextureDimension		Dimension;
	
	nvrhi::IDevice*				Device;
	TString<64>					DebugName;
	
	mutable TMap<ENVRHITextureFilter, nvrhi::SamplerHandle>	SamplerCache;
};

/*-----------------------------------------------------------------------------
	FSamplerNVRHI - Standalone Sampler
-----------------------------------------------------------------------------*/

/**
 * Standalone sampler state object
 * 
 * Usage:
 * ```cpp
 * FSamplerNVRHI Sampler;
 * Sampler.Initialize(ENVRHITextureFilter::LinearMipmapLinear, ENVRHITextureAddress::Wrap, Device);
 * CommandList->bindSamplers(0, &Sampler.GetSamplerHandle().Get(), 1);
 * ```
 */
class FSamplerNVRHI
{
public:
	NOCOPYMOVE(FSamplerNVRHI)
	
	FSamplerNVRHI() = default;
	~FSamplerNVRHI();
	
	// Initialize sampler
	bool Initialize(
		ENVRHITextureFilter Filter,
		ENVRHITextureAddress AddressU,
		ENVRHITextureAddress AddressV,
		ENVRHITextureAddress AddressW,
		nvrhi::IDevice* Device,
		TFLOAT MaxAnisotropy = 16.0f);
	
	// Access
	[[nodiscard]] nvrhi::SamplerHandle GetSamplerHandle() const { return SamplerHandle; }
	
protected:
	nvrhi::SamplerHandle	SamplerHandle;
	nvrhi::IDevice*			Device = nullptr;
};

/*-----------------------------------------------------------------------------
	Inline Implementations
-----------------------------------------------------------------------------*/

FORCEINLINE FTextureNVRHI::FTextureNVRHI()
	: Width(0)
	, Height(0)
	, MipLevels(1)
	, ArraySize(1)
	, SampleCount(1)
	, Format(ENVRHITextureFormat::RGBA8)
	, Dimension(ENVRHITextureDimension::Texture2D)
	, Device(nullptr)
{
}

FORCEINLINE FTextureNVRHI::~FTextureNVRHI()
{
	TextureHandle.Reset();
	TextureRTV.Reset();
	TextureDSV.Reset();
	TextureSRV.Reset();
	TextureUAV.Reset();
	SamplerCache.Empty();
}

FORCEINLINE FSamplerNVRHI::~FSamplerNVRHI()
{
	SamplerHandle.Reset();
}
