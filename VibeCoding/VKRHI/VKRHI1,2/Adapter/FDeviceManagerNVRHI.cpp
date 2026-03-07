/**
 * Copyright (c) 2025. MIT License. All rights reserved.
 * 
 * NVRHI Device Manager Implementation
 */

#include "Renderer/FDeviceManagerNVRHI.h"
#include "Renderer/Window/WindowDefinition.h"

#if HLVM_WINDOW_USE_VULKAN
	#include "Renderer/Window/GLFW3/Vulkan/VulkanWindow.h"
	#include "Renderer/RHI/Vulkan/VulkanDefinition.h"
#endif

#include <nvrhi/validation.h>

/*-----------------------------------------------------------------------------
	FNVRHIMessageCallback Implementation
-----------------------------------------------------------------------------*/

FNVRHIMessageCallback& FNVRHIMessageCallback::GetInstance()
{
	static FNVRHIMessageCallback Instance;
	return Instance;
}

void FNVRHIMessageCallback::message(nvrhi::MessageSeverity Severity, const char* MessageText)
{
	switch (Severity)
	{
		case nvrhi::MessageSeverity::Info:
			HLVM_LOG(LogRHI, info, TXT("%s"), TO_TCHAR_CSTR(MessageText));
			break;
		case nvrhi::MessageSeverity::Warning:
			HLVM_LOG(LogRHI, warn, TXT("%s"), TO_TCHAR_CSTR(MessageText));
			break;
		case nvrhi::MessageSeverity::Error:
			HLVM_LOG(LogRHI, err, TXT("%s"), TO_TCHAR_CSTR(MessageText));
			break;
		case nvrhi::MessageSeverity::Fatal:
		default:
			HLVM_LOG(LogRHI, critical, TXT("%s"), TO_TCHAR_CSTR(MessageText));
			break;
	}
}

/*-----------------------------------------------------------------------------
	FNVRHIDeviceManager Implementation
-----------------------------------------------------------------------------*/

FNVRHIDeviceManager::FNVRHIDeviceManager()
	: GraphicsAPI(nvrhi::GraphicsAPI::VULKAN)
{
}

FNVRHIDeviceManager::~FNVRHIDeviceManager()
{
	Shutdown();
}

TUniquePtr<FNVRHIDeviceManager> FNVRHIDeviceManager::Create(nvrhi::GraphicsAPI Api)
{
#if HLVM_WINDOW_USE_VULKAN
	if (Api == nvrhi::GraphicsAPI::VULKAN)
	{
		// Will be implemented in platform-specific file
		// For now, return nullptr - actual implementation requires Vulkan backend
		HLVM_LOG(LogRHI, critical, TXT("FNVRHIDeviceManager::Create - Vulkan backend not yet implemented"));
		return nullptr;
	}
#endif
	
	HLVM_LOG(LogRHI, critical, TXT("FNVRHIDeviceManager::Create - Unsupported graphics API"));
	return nullptr;
}

bool FNVRHIDeviceManager::CreateWindowDeviceAndSwapChain(const IWindow::Properties& Params)
{
	// 1. Create window
	HLVM_LOG(LogRHI, debug, TXT("Creating window with properties:\n%s"), *Params.ToString());
	
#if HLVM_WINDOW_USE_VULKAN
	if (GraphicsAPI == nvrhi::GraphicsAPI::VULKAN)
	{
		TSharedPtr<FGLFW3VulkanWindow> glfwWindow;
		glfwWindow = MAKE_SHARED(FGLFW3VulkanWindow, Params);
		
		if (glfwWindow)
		{
			HLVM_LOG(LogRHI, debug, TXT("FGLFW3VulkanWindow created!"));
			WindowHandle = MoveTemp(glfwWindow);
		}
		else
		{
			HLVM_LOG(LogRHI, err, TXT("Failed to create window"));
			return false;
		}
	}
	else
#endif
	{
		HLVM_LOG(LogRHI, critical, TXT("Unsupported graphics API for window creation"));
		return false;
	}
	
	// 2. Create device and swapchain
	return CreateDeviceAndSwapChain();
}

void FNVRHIDeviceManager::Shutdown()
{
	DestroyDeviceAndSwapChain();
	
	WindowHandle.Reset();
	NvrhiDevice = nullptr;
	ValidationLayer = nullptr;
	SwapChainImages.Empty();
	FrameData.Empty();
}

void FNVRHIDeviceManager::BeginFrame()
{
	// Wait for previous frame
	if (FrameData.Num() > 0 && FrameData[FrameIndex].FrameWaitQuery)
	{
		NvrhiDevice->waitEventQuery(FrameData[FrameIndex].FrameWaitQuery);
		NvrhiDevice->resetEventQuery(FrameData[FrameIndex].FrameWaitQuery);
	}
	
	// Acquire next swapchain image (platform-specific)
	// This will be implemented in Vulkan backend
}

void FNVRHIDeviceManager::EndFrame()
{
	// Signal frame completion
	if (FrameData.Num() > 0)
	{
		NvrhiDevice->setEventQuery(FrameData[FrameIndex].FrameWaitQuery, nvrhi::CommandQueue::Graphics);
	}
}

void FNVRHIDeviceManager::Present()
{
	// Present swapchain (platform-specific)
	// This will be implemented in Vulkan backend
	
	// Advance frame index
	FrameIndex = (FrameIndex + 1) % FrameData.Num();
}

void FNVRHIDeviceManager::SetVSyncMode(TINT32 VSyncMode)
{
	DeviceParams.VSyncMode = VSyncMode;
	ResizeSwapChain();
}
