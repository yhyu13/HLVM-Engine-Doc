/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 * 
 * NVRHI Device Manager
 * 
 * Manages NVRHI device initialization, resource creation, and frame rendering.
 * Borrowed from RBDOOM-3-BFG's DeviceManager with HLVM coding style.
 */

#pragma once

#include "Core/Log.h"
#include "Template/UniquePtr.h"
#include "Template/SharedPtr.h"
#include "Renderer/Window/WindowDefinition.h"
#include "Renderer/RHI/Common.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/vulkan.h>

#if HLVM_WINDOW_USE_VULKAN
	#include <vulkan/vulkan.hpp>
#endif

/*-----------------------------------------------------------------------------
	Forward Declarations
-----------------------------------------------------------------------------*/

class FBufferNVRHI;
class FTextureNVRHI;
class FFramebufferNVRHI;

/*-----------------------------------------------------------------------------
	Device Creation Parameters
-----------------------------------------------------------------------------*/

/**
 * Parameters for NVRHI device and swapchain creation
 */
struct FNVRHIDeviceCreationParameters
{
	// Window configuration
	TUINT32		BackBufferWidth = 1280;
	TUINT32		BackBufferHeight = 720;
	TUINT32		SwapChainBufferCount = RHI::MAX_FRAMES_IN_FLIGHT;
	nvrhi::Format SwapChainFormat = nvrhi::Format::RGBA8_UNORM;
	TINT32		VSyncMode = 0; // 0=off, 1=on, 2=adaptive
	
	// Feature flags
	bool		bEnableRayTracingExtensions = false;
	bool		bEnableComputeQueue = false;
	bool		bEnableCopyQueue = false;
	
	// Debug and validation
	bool		bEnableDebugRuntime = false;
	bool		bEnableNVRHIValidationLayer = false;
	
#if HLVM_WINDOW_USE_VULKAN
	// Vulkan-specific extensions and layers
	TVector<std::string>	RequiredVulkanInstanceExtensions;
	TVector<std::string>	RequiredVulkanDeviceExtensions;
	TVector<std::string>	RequiredVulkanLayers;
	TVector<std::string>	OptionalVulkanInstanceExtensions;
	TVector<std::string>	OptionalVulkanDeviceExtensions;
	TVector<std::string>	OptionalVulkanLayers;
#endif
};

/*-----------------------------------------------------------------------------
	NVRHI Message Callback
-----------------------------------------------------------------------------*/

/**
 * Default message callback for NVRHI debug output
 */
struct FNVRHIMessageCallback : public nvrhi::IMessageCallback
{
	static FNVRHIMessageCallback& GetInstance();
	void message(nvrhi::MessageSeverity Severity, const char* MessageText) override;
};

/*-----------------------------------------------------------------------------
	NVRHI Frame Data
-----------------------------------------------------------------------------*/

/**
 * Per-frame flight data for NVRHI rendering
 */
struct FNVRHIFrameData
{
	nvrhi::EventQueryHandle	FrameWaitQuery;
	nvrhi::CommandListHandle	CommandList;
};

/*-----------------------------------------------------------------------------
	Main NVRHI Device Manager
-----------------------------------------------------------------------------*/

/**
 * Manages NVRHI device, swapchain, and frame rendering
 * 
 * Key features:
 * - Vulkan instance and device creation via NVRHI
 * - Swapchain management with present mode selection
 * - Frame synchronization with queries and semaphores
 * - Resource garbage collection
 * 
 * Usage:
 * 1. Create via FNVRHIDeviceManager::Create()
 * 2. Call CreateWindowDeviceAndSwapChain()
 * 3. Use GetDevice() for NVRHI operations
 * 4. Call BeginFrame(), render, EndFrame(), Present() each frame
 * 5. Call Shutdown() for cleanup
 */
class FNVRHIDeviceManager
{
public:
	NOCOPYMOVE(FNVRHIDeviceManager)
	
	// Factory method
	static TUniquePtr<FNVRHIDeviceManager> Create(nvrhi::GraphicsAPI Api = nvrhi::GraphicsAPI::VULKAN);
	
	// Lifecycle
	virtual bool CreateWindowDeviceAndSwapChain(const IWindow::Properties& Params);
	virtual void Shutdown();
	
	// Frame rendering
	virtual void BeginFrame();
	virtual void EndFrame();
	virtual void Present();
	
	// Device access
	[[nodiscard]] nvrhi::IDevice* GetDevice() const { return NvrhiDevice; }
	[[nodiscard]] nvrhi::GraphicsAPI GetGraphicsAPI() const { return GraphicsAPI; }
	[[nodiscard]] const char* GetRendererString() const { return RendererString.c_str(); }
	
	// Swapchain access
	[[nodiscard]] nvrhi::ITexture* GetCurrentBackBuffer() const;
	[[nodiscard]] nvrhi::ITexture* GetBackBuffer(TUINT32 Index) const;
	[[nodiscard]] TUINT32 GetCurrentBackBufferIndex() const { return SwapChainIndex; }
	[[nodiscard]] TUINT32 GetBackBufferCount() const { return static_cast<TUINT32>(SwapChainImages.size()); }
	
	// Configuration
	const FNVRHIDeviceCreationParameters& GetDeviceParams() const { return DeviceParams; }
	void SetVSyncMode(TINT32 VSyncMode);
	
	// Utility
	[[nodiscard]] TUINT32 GetFrameIndex() const { return FrameIndex; }
	
protected:
	FNVRHIDeviceManager();
	virtual ~FNVRHIDeviceManager();
	
	// Platform-specific implementations
	virtual bool CreateDeviceAndSwapChain() = 0;
	virtual void DestroyDeviceAndSwapChain() = 0;
	virtual void ResizeSwapChain() = 0;
	
	// Friends
	friend class FBufferNVRHI;
	friend class FTextureNVRHI;
	friend class FFramebufferNVRHI;
	
	// Protected members
	TSharedPtr<IWindow>		WindowHandle;
	FNVRHIDeviceCreationParameters DeviceParams;
	nvrhi::GraphicsAPI		GraphicsAPI;
	std::string				RendererString;
	
	// NVRHI device
	nvrhi::DeviceHandle		NvrhiDevice;
	nvrhi::DeviceHandle		ValidationLayer;
	
	// Swapchain
	TVector<nvrhi::TextureHandle>	SwapChainImages;
	TUINT32							SwapChainIndex = INVALID_INDEX_UINT32;
	
	// Frame synchronization
	TVector<FNVRHIFrameData>	FrameData;
	TUINT32						FrameIndex = 0;
	
	// Present semaphore
	nvrhi::EventQueryHandle		FrameWaitQuery;
};

/*-----------------------------------------------------------------------------
	Inline Implementations
-----------------------------------------------------------------------------*/

FORCEINLINE nvrhi::ITexture* FNVRHIDeviceManager::GetCurrentBackBuffer() const
{
	if (SwapChainIndex < SwapChainImages.Num())
	{
		return SwapChainImages[SwapChainIndex].Get();
	}
	return nullptr;
}

FORCEINLINE nvrhi::ITexture* FNVRHIDeviceManager::GetBackBuffer(TUINT32 Index) const
{
	if (Index < SwapChainImages.Num())
	{
		return SwapChainImages[Index].Get();
	}
	return nullptr;
}
