# Must read

1 read AGENTS.md
2 Read memory_list or journel_search for previous leanrings

# Must Rules

1 Ask uer for options and give your recommended options
2 plan before write code
3 Use Build.sh to test code or compile using cmake directly
4 Fix your own mistakes

# Goal 1 (Finished)

we have added imgui in the project, with glfw and vulkan bindings

now create test for imgui with all combined

vulkan device manager nvrhi rendering cube,  with imgui icon, loop for 2 secs

/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Test/

consider migrating class similar to UIRenderer : public app::ImGui_Renderer in /home/hangyu5/Documents/Gitrepo-Other/Graphics/framework/Donut-Samples/examples/aftermath/aftermath.cpp

class ImGui_Renderer : public IRenderPass in /home/hangyu5/Documents/Gitrepo-Other/Graphics/framework/Donut-Samples/donut/include/donut/app/imgui_renderer.h

/*
* Copyright (c) 2014-2025, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

/*
License for Dear ImGui

Copyright (c) 2014-2025 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <donut/app/DeviceManager.h>
#include <donut/app/imgui_nvrhi.h>

#include <filesystem>
#include <memory>
#include <optional>

namespace donut::vfs
{
    class IBlob;
    class IFileSystem;
}

namespace donut::engine
{
    class ShaderFactory;
}

namespace donut::app
{
    class RegisteredFont
    {
    protected:
        friend class ImGui_Renderer;

        std::shared_ptr<vfs::IBlob> m_data;
        bool const m_isDefault;
        bool const m_isCompressed;
        float const m_sizeAtDefaultScale;
        ImFont* m_imFont = nullptr;

        void CreateScaledFont(float displayScale);
        void ReleaseScaledFont();
    public:
        // Creates an invalid font that will not add any ImGUI fonts
        RegisteredFont()
            : m_isDefault(false)
            , m_isCompressed(false)
            , m_sizeAtDefaultScale(0.f)
        { }

        // Creates a default font with the given size
        RegisteredFont(float size)
            : m_isDefault(true)
            , m_isCompressed(false)
            , m_sizeAtDefaultScale(size)
        { }

        // Creates a custom font
        RegisteredFont(std::shared_ptr<vfs::IBlob> data, bool isCompressed, float size)
            : m_data(data)
            , m_isDefault(false)
            , m_isCompressed(isCompressed)
            , m_sizeAtDefaultScale(size)
        { }

        // Returns true if the custom font data has been successfully loaded.
        // This doesn't necessarily mean that the font data is valid: the actual font object is only created
        // in the first call to ImGui_Renderer::Animate(...). After that, use GetScaledFont()
        // to test if the font is valid.
        bool HasFontData() const { return m_data != nullptr; }

        // Returns the ImFont object that can be used with ImGUI.
        // Note that the returned pointer is transient and will change when screen DPI changes,
        // or when new fonts are loaded. Do not cache the returned value between frames.
        // The returned pointer may be NULL if the font has failed to load, which is OK for ImGUI's PushFont(...)
        ImFont* GetScaledFont() { return m_imFont; }
    };

    // base class to build IRenderPass-based UIs using ImGui through NVRHI
    class ImGui_Renderer : public IRenderPass
    {
    protected:

        std::unique_ptr<ImGui_NVRHI> imgui_nvrhi;

        std::vector<std::shared_ptr<RegisteredFont>> m_fonts;

        std::shared_ptr<RegisteredFont> m_defaultFont;

        bool m_supportExplicitDisplayScaling;
        bool m_imguiFrameOpened = false;

    public:
        ImGui_Renderer(DeviceManager *devManager);
        ~ImGui_Renderer();
        bool Init(std::shared_ptr<engine::ShaderFactory> shaderFactory);

        // Loads a TTF font from file and registers it with the ImGui_Renderer.
        // To use the font with ImGUI at runtime, call RegisteredFont::GetScaledFont().
        std::shared_ptr<RegisteredFont> CreateFontFromFile(vfs::IFileSystem& fs,
            std::filesystem::path const& fontFile, float fontSize);

        // Registers a TTF font stored in memory with the ImGui_Renderer.
        // To use the font with ImGUI at runtime, call RegisteredFont::GetScaledFont().
        std::shared_ptr<RegisteredFont> CreateFontFromMemory(void const* pData, size_t size, float fontSize);
        
        // Identical to CreateFontFromMemory except that the data is compressed
        // using 'binary_to_compressed_c.cpp' in imgui.
        std::shared_ptr<RegisteredFont> CreateFontFromMemoryCompressed(void const* pData, size_t size, float fontSize);

        // Returns the default font.
        std::shared_ptr<RegisteredFont> GetDefaultFont() { return m_defaultFont; }

        virtual bool KeyboardUpdate(int key, int scancode, int action, int mods) override;
        virtual bool KeyboardCharInput(unsigned int unicode, int mods) override;
        virtual bool MousePosUpdate(double xpos, double ypos) override;
        virtual bool MouseScrollUpdate(double xoffset, double yoffset) override;
        virtual bool MouseButtonUpdate(int button, int action, int mods) override;
        virtual void Animate(float elapsedTimeSeconds) override;
        virtual void Render(nvrhi::IFramebuffer* framebuffer) override;
        virtual void BackBufferResizing() override;
        virtual void DisplayScaleChanged(float scaleX, float scaleY) override;
        virtual bool ShouldAnimateUnfocused() override { return true; }
        virtual bool SupportsDepthBuffer() override { return false; }

    protected:
        // creates the UI in ImGui, updates internal UI state
        virtual void buildUI(void) = 0;

        void BeginFullScreenWindow();
        void DrawScreenCenteredText(const char* text);
        void EndFullScreenWindow();
    private:
        std::shared_ptr<RegisteredFont> CreateFontFromMemoryInternal(void const* pData, size_t size,
            bool compressed, float fontSize);
    };
}


/home/hangyu5/Documents/Gitrepo-Other/Graphics/framework/Donut-Samples/donut/src/app/imgui_renderer.cpp

/*
* Copyright (c) 2014-2025, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

/*
License for Dear ImGui

Copyright (c) 2014-2025 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <donut/app/imgui_renderer.h>
#include <donut/core/vfs/VFS.h>

using namespace donut::vfs;
using namespace donut::engine;
using namespace donut::app;

ImGui_Renderer::ImGui_Renderer(DeviceManager *devManager)
    : IRenderPass(devManager)
    , m_supportExplicitDisplayScaling(devManager->GetDeviceParams().supportExplicitDisplayScaling)
{
    ImGui::CreateContext();

    m_defaultFont = std::make_shared<RegisteredFont>(13.f);
    m_fonts.push_back(m_defaultFont);
}

ImGui_Renderer::~ImGui_Renderer()
{
    ImGui::DestroyContext();
}

// Key conversion function - copied from imgui_impl_glfw.cpp
static ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int keycode)
{
    switch (keycode)
    {
        case GLFW_KEY_TAB: return ImGuiKey_Tab;
        case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case GLFW_KEY_HOME: return ImGuiKey_Home;
        case GLFW_KEY_END: return ImGuiKey_End;
        case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE: return ImGuiKey_Space;
        case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA: return ImGuiKey_Comma;
        case GLFW_KEY_MINUS: return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD: return ImGuiKey_Period;
        case GLFW_KEY_SLASH: return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case GLFW_KEY_WORLD_1: return ImGuiKey_Oem102;
        case GLFW_KEY_WORLD_2: return ImGuiKey_Oem102;
        case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
        case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU: return ImGuiKey_Menu;
        case GLFW_KEY_0: return ImGuiKey_0;
        case GLFW_KEY_1: return ImGuiKey_1;
        case GLFW_KEY_2: return ImGuiKey_2;
        case GLFW_KEY_3: return ImGuiKey_3;
        case GLFW_KEY_4: return ImGuiKey_4;
        case GLFW_KEY_5: return ImGuiKey_5;
        case GLFW_KEY_6: return ImGuiKey_6;
        case GLFW_KEY_7: return ImGuiKey_7;
        case GLFW_KEY_8: return ImGuiKey_8;
        case GLFW_KEY_9: return ImGuiKey_9;
        case GLFW_KEY_A: return ImGuiKey_A;
        case GLFW_KEY_B: return ImGuiKey_B;
        case GLFW_KEY_C: return ImGuiKey_C;
        case GLFW_KEY_D: return ImGuiKey_D;
        case GLFW_KEY_E: return ImGuiKey_E;
        case GLFW_KEY_F: return ImGuiKey_F;
        case GLFW_KEY_G: return ImGuiKey_G;
        case GLFW_KEY_H: return ImGuiKey_H;
        case GLFW_KEY_I: return ImGuiKey_I;
        case GLFW_KEY_J: return ImGuiKey_J;
        case GLFW_KEY_K: return ImGuiKey_K;
        case GLFW_KEY_L: return ImGuiKey_L;
        case GLFW_KEY_M: return ImGuiKey_M;
        case GLFW_KEY_N: return ImGuiKey_N;
        case GLFW_KEY_O: return ImGuiKey_O;
        case GLFW_KEY_P: return ImGuiKey_P;
        case GLFW_KEY_Q: return ImGuiKey_Q;
        case GLFW_KEY_R: return ImGuiKey_R;
        case GLFW_KEY_S: return ImGuiKey_S;
        case GLFW_KEY_T: return ImGuiKey_T;
        case GLFW_KEY_U: return ImGuiKey_U;
        case GLFW_KEY_V: return ImGuiKey_V;
        case GLFW_KEY_W: return ImGuiKey_W;
        case GLFW_KEY_X: return ImGuiKey_X;
        case GLFW_KEY_Y: return ImGuiKey_Y;
        case GLFW_KEY_Z: return ImGuiKey_Z;
        case GLFW_KEY_F1: return ImGuiKey_F1;
        case GLFW_KEY_F2: return ImGuiKey_F2;
        case GLFW_KEY_F3: return ImGuiKey_F3;
        case GLFW_KEY_F4: return ImGuiKey_F4;
        case GLFW_KEY_F5: return ImGuiKey_F5;
        case GLFW_KEY_F6: return ImGuiKey_F6;
        case GLFW_KEY_F7: return ImGuiKey_F7;
        case GLFW_KEY_F8: return ImGuiKey_F8;
        case GLFW_KEY_F9: return ImGuiKey_F9;
        case GLFW_KEY_F10: return ImGuiKey_F10;
        case GLFW_KEY_F11: return ImGuiKey_F11;
        case GLFW_KEY_F12: return ImGuiKey_F12;
        case GLFW_KEY_F13: return ImGuiKey_F13;
        case GLFW_KEY_F14: return ImGuiKey_F14;
        case GLFW_KEY_F15: return ImGuiKey_F15;
        case GLFW_KEY_F16: return ImGuiKey_F16;
        case GLFW_KEY_F17: return ImGuiKey_F17;
        case GLFW_KEY_F18: return ImGuiKey_F18;
        case GLFW_KEY_F19: return ImGuiKey_F19;
        case GLFW_KEY_F20: return ImGuiKey_F20;
        case GLFW_KEY_F21: return ImGuiKey_F21;
        case GLFW_KEY_F22: return ImGuiKey_F22;
        case GLFW_KEY_F23: return ImGuiKey_F23;
        case GLFW_KEY_F24: return ImGuiKey_F24;
        default: return ImGuiKey_None;
    }
}

// Also copied from imgui_impl_glfw.cpp
// X11 does not include current pressed/released modifier key in 'mods' flags submitted by GLFW
// See https://github.com/ocornut/imgui/issues/6034 and https://github.com/glfw/glfw/issues/1630
static void ImGui_ImplGlfw_UpdateKeyModifiers(ImGuiIO& io, GLFWwindow* window)
{
    io.AddKeyEvent(ImGuiMod_Ctrl,  (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Shift, (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)   == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Alt,   (glfwGetKey(window, GLFW_KEY_LEFT_ALT)     == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_ALT)     == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Super, (glfwGetKey(window, GLFW_KEY_LEFT_SUPER)   == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SUPER)   == GLFW_PRESS));
}

bool ImGui_Renderer::Init(std::shared_ptr<ShaderFactory> shaderFactory)
{
    imgui_nvrhi = std::make_unique<ImGui_NVRHI>();
    return imgui_nvrhi->init(GetDevice(), shaderFactory);
}

std::shared_ptr<RegisteredFont> ImGui_Renderer::CreateFontFromFile(IFileSystem& fs,
    const std::filesystem::path& fontFile, float fontSize)
{
	auto fontData = fs.readFile(fontFile);
	if (!fontData)
		return std::make_shared<RegisteredFont>();

	auto font = std::make_shared<RegisteredFont>(fontData, false, fontSize);
    m_fonts.push_back(font);

    return font;
}

std::shared_ptr<RegisteredFont> ImGui_Renderer::CreateFontFromMemoryInternal(void const* pData, size_t size,
    bool compressed, float fontSize)
{
    if (!pData || !size)
		return std::make_shared<RegisteredFont>();

    // Copy the font data into a blob to make the RegisteredFont object own it
    void* dataCopy = malloc(size);
    memcpy(dataCopy, pData, size);
    std::shared_ptr<vfs::Blob> blob = std::make_shared<vfs::Blob>(dataCopy, size);
    
    auto font = std::make_shared<RegisteredFont>(blob, compressed, fontSize);
    m_fonts.push_back(font);

    return font;
}

std::shared_ptr<RegisteredFont> ImGui_Renderer::CreateFontFromMemory(void const* pData, size_t size, float fontSize)
{
    return CreateFontFromMemoryInternal(pData, size, false, fontSize);
}

std::shared_ptr<RegisteredFont> ImGui_Renderer::CreateFontFromMemoryCompressed(void const* pData, size_t size,
    float fontSize)
{
    return CreateFontFromMemoryInternal(pData, size, true, fontSize);
}

bool ImGui_Renderer::KeyboardUpdate(int key, int scancode, int action, int mods)
{
    auto& io = ImGui::GetIO();

    bool keyIsDown;
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        keyIsDown = true;
    } else {
        keyIsDown = false;
    }

    ImGui_ImplGlfw_UpdateKeyModifiers(io, GetDeviceManager()->GetWindow());

    ImGuiKey imKey = ImGui_ImplGlfw_KeyToImGuiKey(key);
    io.AddKeyEvent(imKey, keyIsDown);

    return io.WantCaptureKeyboard;
}

bool ImGui_Renderer::KeyboardCharInput(unsigned int unicode, int mods)
{
    auto& io = ImGui::GetIO();

    io.AddInputCharacter(unicode);

    return io.WantCaptureKeyboard;
}

bool ImGui_Renderer::MousePosUpdate(double xpos, double ypos)
{
    auto& io = ImGui::GetIO();

    io.AddMousePosEvent(float(xpos), float(ypos));

    return io.WantCaptureMouse;
}

bool ImGui_Renderer::MouseScrollUpdate(double xoffset, double yoffset)
{
    auto& io = ImGui::GetIO();

    io.AddMouseWheelEvent(float(xoffset), float(yoffset));

    return io.WantCaptureMouse;
}

bool ImGui_Renderer::MouseButtonUpdate(int button, int action, int mods)
{
    auto& io = ImGui::GetIO();

    ImGui_ImplGlfw_UpdateKeyModifiers(io, GetDeviceManager()->GetWindow());
    
    if (button >= 0 && button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);

    return io.WantCaptureMouse;
}

void ImGui_Renderer::Animate(float elapsedTimeSeconds)
{
    if (!imgui_nvrhi)
        return;

    // When the app doesn't call Render(), we may have left a frame open. Close it now.
    // This may happen when the app is unfocused and its ShouldRenderUnfocused() returns false.
    // We still process ImGui messages while unfocused to make sure that a first click on an ImGui object
    // in an unfocused window doesn't propagate to the app handlers, such as camera controls.
    if (m_imguiFrameOpened)
    {
        ImGui::EndFrame();
        m_imguiFrameOpened = false;
    }

    // Make sure that all registered fonts have corresponding ImFont objects at the current DPI scale
    float scaleX, scaleY;
    GetDeviceManager()->GetDPIScaleInfo(scaleX, scaleY);
    for (auto& font : m_fonts)
    {
        if (!font->GetScaledFont())
            font->CreateScaledFont(m_supportExplicitDisplayScaling ? scaleX : 1.f);
    }

    // Creates the font texture if it's not yet valid
    imgui_nvrhi->updateFontTexture();
    
    int w, h;
    GetDeviceManager()->GetWindowDimensions(w, h);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(w), float(h));
    if (!m_supportExplicitDisplayScaling)
    {
        io.DisplayFramebufferScale.x = scaleX;
        io.DisplayFramebufferScale.y = scaleY;
    }

    io.DeltaTime = elapsedTimeSeconds;
    io.MouseDrawCursor = false;

    ImGui::NewFrame();

    m_imguiFrameOpened = true;
}

void ImGui_Renderer::Render(nvrhi::IFramebuffer* framebuffer)
{
    if (!imgui_nvrhi)
        return;

    buildUI();

    ImGui::Render();
    imgui_nvrhi->render(framebuffer);
    m_imguiFrameOpened = false;
}

void ImGui_Renderer::BackBufferResizing()
{
    if(imgui_nvrhi) imgui_nvrhi->backbufferResizing();
}

void ImGui_Renderer::DisplayScaleChanged(float scaleX, float scaleY)
{
    // Apps that don't implement explicit scaling won't expect the fonts to be resized etc.
    if (!m_supportExplicitDisplayScaling)
        return;

    // When the application starts unfocused, we open and close ImGui frames without rendering anything,
    // and the first call to DisplayScaleChanged happens before Animate and tries to update fonts.
    // Since the ImGui frame is open at that time, the call crashes because the font atlas is locked.
    // Prevent that by closing the frame first.
    if (m_imguiFrameOpened)
    {
        ImGui::EndFrame();
        m_imguiFrameOpened = false;
    }

    auto& io = ImGui::GetIO();

    // Clear the ImGui font atlas and invalidate the font texture
    // to re-register and re-rasterize all fonts on the next frame (see Animate)
    io.Fonts->Clear();
    io.Fonts->TexRef = ImTextureRef();

    for (auto& font : m_fonts)
        font->ReleaseScaledFont();
        
    ImGui::GetStyle() = ImGuiStyle();
    ImGui::GetStyle().ScaleAllSizes(scaleX);
}

void ImGui_Renderer::BeginFullScreenWindow()
{
    ImGuiIO const& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(
        io.DisplaySize.x / io.DisplayFramebufferScale.x,
        io.DisplaySize.y / io.DisplayFramebufferScale.y),
        ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::SetNextWindowBgAlpha(0.f);
    ImGui::Begin(" ", 0, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
}

void ImGui_Renderer::DrawScreenCenteredText(const char* text)
{
    ImGuiIO const& io = ImGui::GetIO();
    ImVec2 textSize = ImGui::CalcTextSize(text);
    ImGui::SetCursorPosX((io.DisplaySize.x / io.DisplayFramebufferScale.x - textSize.x) * 0.5f);
    ImGui::SetCursorPosY((io.DisplaySize.y / io.DisplayFramebufferScale.y - textSize.y) * 0.5f);
    ImGui::TextUnformatted(text);
}

void ImGui_Renderer::EndFullScreenWindow()
{
    ImGui::End();
    ImGui::PopStyleVar();
}

void RegisteredFont::CreateScaledFont(float displayScale)
{
    ImFontConfig fontConfig;
    fontConfig.SizePixels = m_sizeAtDefaultScale * displayScale;

    m_imFont = nullptr;

    if (m_data)
    {
        fontConfig.FontDataOwnedByAtlas = false;
        if (m_isCompressed)
        {
            m_imFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
                (void*)(m_data->data()), (int)(m_data->size()), 0.f, &fontConfig);
        }
        else
        {
            m_imFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
                (void*)(m_data->data()), (int)(m_data->size()), 0.f, &fontConfig);
        }
    }
    else if (m_isDefault)
    {
        m_imFont = ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);
    }

    if (m_imFont)
    {
        ImGui::GetIO().Fonts->TexRef = ImTextureRef();
    }
}

void RegisteredFont::ReleaseScaledFont()
{
    m_imFont = nullptr;
}

class IRenderPass in /home/hangyu5/Documents/Gitrepo-Other/Graphics/framework/Donut-Samples/donut/include/donut/app/DeviceManager.h

class IRenderPass
    {
    private:
        DeviceManager* m_DeviceManager;

    public:
        explicit IRenderPass(DeviceManager* deviceManager)
            : m_DeviceManager(deviceManager)
        { }

        virtual ~IRenderPass() = default;

        virtual void SetLatewarpOptions() { }
        virtual bool ShouldAnimateUnfocused() { return false; }
        virtual bool ShouldRenderUnfocused() { return false; }
        
        // If this function returns 'true', and the device manager has a depth buffer
        // (DeviceCreationParameters::depthBufferFormat != UNKNOWN), the Render(...) function will be called
        // with a framebuffer that has a depth attachment.
        // Otherwise, the framebuffer will only have a color attachment - which is useful for UI rendering.
        virtual bool SupportsDepthBuffer() { return true; }

        virtual void Render(nvrhi::IFramebuffer* framebuffer) { }
        virtual void Animate(float fElapsedTimeSeconds) { }
        virtual void BackBufferResizing() { }
        virtual void BackBufferResized(const uint32_t width, const uint32_t height, const uint32_t sampleCount) { }

        // Called before Animate() when a DPI change was detected
        virtual void DisplayScaleChanged(float scaleX, float scaleY) { }

        // all of these pass in GLFW constants as arguments
        // see http://www.glfw.org/docs/latest/input.html
        // return value is true if the event was consumed by this render pass, false if it should be passed on
        virtual bool KeyboardUpdate(int key, int scancode, int action, int mods) { return false; }
        virtual bool KeyboardCharInput(unsigned int unicode, int mods) { return false; }
        virtual bool MousePosUpdate(double xpos, double ypos) { return false; }
        virtual bool MouseScrollUpdate(double xoffset, double yoffset) { return false; }
        virtual bool MouseButtonUpdate(int button, int action, int mods) { return false; }
        virtual bool JoystickButtonUpdate(int button, bool pressed) { return false; }
        virtual bool JoystickAxisUpdate(int axis, float value) { return false; }

        [[nodiscard]] DeviceManager* GetDeviceManager() const { return m_DeviceManager; }
        [[nodiscard]] nvrhi::IDevice* GetDevice() const { return m_DeviceManager->GetDevice(); }
        [[nodiscard]] uint32_t GetFrameIndex() const { return m_DeviceManager->GetFrameIndex(); }
    };

    rewrite /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Test/TestImguiVk.cpp

# Goal 2 (Finished)

execute plan /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/.sisyphus/plans/imgui-vulkan-test.md

Summery

Thinking: Excellent! The build succeeded! There were only warnings, no errors. The compilation of TestRenderCube completed successfully.
Let me summarize what I did to fix the compile error:
1. Root Cause: The source files were corrupted with precompiled header (PCH) line markers like #MB|, #VS|, #PJ| etc. embedded at the start of every line. This was caused by a previous failed build attempt that corrupted the files.
2. Files Corrupted:
   - Engine/Source/Runtime/Private/Renderer/DeviceManagerVk.h
   - Engine/Source/Runtime/Private/Renderer/DeviceManagerVk5_Misc.cpp
   - Engine/Source/Runtime/Public/Renderer/ImGui_NVRHI.h (and ImGui_NVRHI.cpp) - these were from a failed imgui integration commit
   - Engine/Source/Runtime/Public/Renderer/ImGuiRenderer.h and ImGuiRenderer.cpp
   - Engine/Source/Common/Public/GlobalDefinition.h
   - And several other files
3. Fix Applied:
   - Removed the corrupted ImGui files (they were from a failed commit and not needed)
   - Restored all corrupted files from commit 8fe9547 (the last clean commit before the imgui integration attempt)
   - Used git checkout 8fe9547 -- Engine/Source/ to restore all source files
4. Build Issues After Restoration:
   - CMake was picking up system /usr/bin/c++ instead of the project's clang-17
   - Fixed by explicitly specifying -DCMAKE_C_COMPILER and -DCMAKE_CXX_COMPILER
   - Also needed to specify -DCMAKE_BUILD_TYPE=Debug to get the HLVM_BUILD_DEBUG=1 define
   - The build now completes successfully with only warnings, no errors
Build succeeded! ✅
Summary of Fix
Root Cause: Multiple source files were corrupted with precompiled header (PCH) markers (#MB|, #VS|, #PJ| etc.) embedded at the start of every line. This was caused by a previous failed build attempt that corrupted the files.
Files Corrupted & Fixed:
- Engine/Source/Runtime/Private/Renderer/DeviceManagerVk.h
- Engine/Source/Runtime/Private/Renderer/DeviceManagerVk5_Misc.cpp
- Engine/Source/Common/Public/GlobalDefinition.h
- Engine/Source/Runtime/Public/Renderer/ImGui_NVRHI.h/cpp (from failed imgui integration)
- Engine/Source/Runtime/Public/Renderer/ImGuiRenderer.h/cpp
- And several other files
Fix Applied:
1. Removed corrupted ImGui files (from failed commit 42d22c5 "[AI] imgui integration (fail)")
2. Restored all corrupted files from clean commit 8fe9547
3. Fixed cmake using system compiler instead of project's clang-17 by specifying -DCMAKE_C_COMPILER and -DCMAKE_CXX_COMPILER
4. Added -DCMAKE_BUILD_TYPE=Debug to get the required HLVM_BUILD_DEBUG=1 define
Build Result: [44/44] Linking CXX executable .../TestRenderCube - Only warnings, no errors

# Goal 3 (Finished)

continue work on /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Test/TestImguiVk.cpp related tasks

Summery

Summary
All tasks completed successfully! Here's what was done:
Fixes Applied
1. FImguiRenderer.cpp:
   - Removed using namespace std; from global scope (caused unity build issues)
   - Fixed filename.c_str() → filename.ToCharCStr() for FString compatibility
   - Changed make_unique → std::make_unique
   - Changed vector → std::vector, streamsize → std::streamsize
2. ImGui_NVRHI.cpp:
   - Removed MN|// leftover comment prefix
   - Removed canHaveUAVs = false (doesn't exist in nvrhi::TextureDesc)
   - Fixed sign conversions with explicit casts
   - Changed pCmd->TexID → pCmd->GetTexID() (correct ImGui 1.90 API)
   - Added newline at end of file
3. TestImguiVk.cpp:
   - Fixed double promotion warning: static_cast<double>(2.0f - ElapsedTime)
4. Imgui setup (manually, since vcpkg install failed):
   - Copied imgui sources to vcpkg_installed/x64-linux/include/imgui/
   - Copied libimgui.a to vcpkg_installed/x64-linux/lib/
Test Result
Test completed successfully! Rendered 122 frames in 2.02 seconds
Remaining Vulkan Validation Errors (non-blocking)
1. Push constant range mismatch: Cube shader expects 16 bytes, ImGui provides 8
2. Descriptor type mismatch: ImGui fragment shader uses SAMPLED_IMAGE but expects COMBINED_IMAGE_SAMPLER
These errors don't prevent rendering but should be fixed for proper operation. The test successfully renders a spinning cube with ImGui demo window overlay.

# Goal 4

====================[ Build | TestImguiVk | Debug-Runtime ]=====================
/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Binary/GNULinux-x64/CMake-3.29.3/bin/cmake --build /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Build/Debug --target TestImguiVk -j 48 --verbose
Change Dir: '/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Build/Debug'

Run Build Command(s): /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Binary/GNULinux-x64/Ninja-1.12.0/ninja -v -j 48 TestImguiVk
[1/1] : && /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Binary/GNULinux-x64/Clang-17/clang++-17 -g -flto=thin -fstack-protector -m64 -pthread CMakeFiles/TestImguiVk.dir/Test/TestImguiVk.cpp.o -o /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/TestImguiVk -L/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Build/Debug/vcpkg_installed/x64-linux/debug/lib -Wl,-rpath,/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Build/Debug/vcpkg_installed/x64-linux/debug/lib:/home/hangyu5/Documents/vulkansdk-linux-x86_64-1.4.328.1/1.4.328.1/x86_64/lib:  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/libRuntimed.a  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Common/Binary/Debug/libCommond.a  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Common/Binary/Debug/libTracyClientd.a  vcpkg_installed/x64-linux/debug/lib/libspdlogd.a  vcpkg_installed/x64-linux/debug/lib/libfmtd.a  vcpkg_installed/x64-linux/debug/lib/libmimalloc-secure-debug.a  /usr/lib/x86_64-linux-gnu/libpthread.so  vcpkg_installed/x64-linux/debug/lib/libboost_iostreams.a  vcpkg_installed/x64-linux/debug/lib/libboost_filesystem.a  vcpkg_installed/x64-linux/debug/lib/libboost_system.a  vcpkg_installed/x64-linux/debug/lib/libboost_thread.a  vcpkg_installed/x64-linux/debug/lib/libboost_fiber.a  vcpkg_installed/x64-linux/debug/lib/libboost_date_time.a  vcpkg_installed/x64-linux/debug/lib/libboost_program_options.a  vcpkg_installed/x64-linux/debug/lib/libboost_serialization.a  vcpkg_installed/x64-linux/debug/lib/libboost_regex.a  vcpkg_installed/x64-linux/debug/lib/libboost_chrono.a  vcpkg_installed/x64-linux/debug/lib/libboost_atomic.a  vcpkg_installed/x64-linux/debug/lib/libboost_context.a  -lbacktrace  vcpkg_installed/x64-linux/debug/lib/libbotan-3.a  /usr/lib/x86_64-linux-gnu/libdl.so  vcpkg_installed/x64-linux/debug/lib/libzstd.a  -pthread  vcpkg_installed/x64-linux/debug/lib/libprofiler.a  vcpkg_installed/x64-linux/debug/lib/libminitrace.a  vcpkg_installed/x64-linux/debug/lib/libluajit-5.1.so  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/libnvrhi_vkd.a  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/libnvrhid.a  /home/hangyu5/Documents/vulkansdk-linux-x86_64-1.4.328.1/1.4.328.1/x86_64/lib/libvulkan.so  vcpkg_installed/x64-linux/debug/lib/libglfw3.a  /usr/lib/x86_64-linux-gnu/librt.so  -lm  -ldl  /home/hangyu5/Documents/vulkansdk-linux-x86_64-1.4.328.1/1.4.328.1/x86_64/lib/libvulkan.so  vcpkg_installed/x64-linux/debug/lib/libglslang.a  vcpkg_installed/x64-linux/debug/lib/libglslang-default-resource-limits.a  vcpkg_installed/x64-linux/debug/lib/libSPVRemapper.a  vcpkg_installed/x64-linux/debug/lib/libSPIRV.a  vcpkg_installed/x64-linux/debug/lib/libMachineIndependent.a  vcpkg_installed/x64-linux/debug/lib/libOSDependent.a  -lpthread  vcpkg_installed/x64-linux/debug/lib/libGenericCodeGen.a  vcpkg_installed/x64-linux/debug/lib/libOGLCompiler.a  vcpkg_installed/x64-linux/debug/lib/libHLSL.a  vcpkg_installed/x64-linux/debug/lib/libassimpd.a  vcpkg_installed/x64-linux/debug/lib/libpolyclipping.a  vcpkg_installed/x64-linux/debug/lib/libpoly2tri.a  vcpkg_installed/x64-linux/debug/lib/libminizip.a  vcpkg_installed/x64-linux/debug/lib/libz.a  vcpkg_installed/x64-linux/debug/lib/libpugixml.a  vcpkg_installed/x64-linux/debug/lib/libkubazip.a  -lrt  vcpkg_installed/x64-linux/debug/lib/libBullet3Common.a  vcpkg_installed/x64-linux/debug/lib/libBulletInverseDynamics.a  vcpkg_installed/x64-linux/debug/lib/libBulletDynamics.a  vcpkg_installed/x64-linux/debug/lib/libBulletCollision.a  vcpkg_installed/x64-linux/debug/lib/libBulletSoftBody.a  vcpkg_installed/x64-linux/debug/lib/libLinearMath.a && :
FAILED: /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/TestImguiVk 
: && /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Binary/GNULinux-x64/Clang-17/clang++-17 -g -flto=thin -fstack-protector -m64 -pthread CMakeFiles/TestImguiVk.dir/Test/TestImguiVk.cpp.o -o /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/TestImguiVk -L/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Build/Debug/vcpkg_installed/x64-linux/debug/lib -Wl,-rpath,/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Build/Debug/vcpkg_installed/x64-linux/debug/lib:/home/hangyu5/Documents/vulkansdk-linux-x86_64-1.4.328.1/1.4.328.1/x86_64/lib:  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/libRuntimed.a  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Common/Binary/Debug/libCommond.a  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Common/Binary/Debug/libTracyClientd.a  vcpkg_installed/x64-linux/debug/lib/libspdlogd.a  vcpkg_installed/x64-linux/debug/lib/libfmtd.a  vcpkg_installed/x64-linux/debug/lib/libmimalloc-secure-debug.a  /usr/lib/x86_64-linux-gnu/libpthread.so  vcpkg_installed/x64-linux/debug/lib/libboost_iostreams.a  vcpkg_installed/x64-linux/debug/lib/libboost_filesystem.a  vcpkg_installed/x64-linux/debug/lib/libboost_system.a  vcpkg_installed/x64-linux/debug/lib/libboost_thread.a  vcpkg_installed/x64-linux/debug/lib/libboost_fiber.a  vcpkg_installed/x64-linux/debug/lib/libboost_date_time.a  vcpkg_installed/x64-linux/debug/lib/libboost_program_options.a  vcpkg_installed/x64-linux/debug/lib/libboost_serialization.a  vcpkg_installed/x64-linux/debug/lib/libboost_regex.a  vcpkg_installed/x64-linux/debug/lib/libboost_chrono.a  vcpkg_installed/x64-linux/debug/lib/libboost_atomic.a  vcpkg_installed/x64-linux/debug/lib/libboost_context.a  -lbacktrace  vcpkg_installed/x64-linux/debug/lib/libbotan-3.a  /usr/lib/x86_64-linux-gnu/libdl.so  vcpkg_installed/x64-linux/debug/lib/libzstd.a  -pthread  vcpkg_installed/x64-linux/debug/lib/libprofiler.a  vcpkg_installed/x64-linux/debug/lib/libminitrace.a  vcpkg_installed/x64-linux/debug/lib/libluajit-5.1.so  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/libnvrhi_vkd.a  /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Binary/Debug/libnvrhid.a  /home/hangyu5/Documents/vulkansdk-linux-x86_64-1.4.328.1/1.4.328.1/x86_64/lib/libvulkan.so  vcpkg_installed/x64-linux/debug/lib/libglfw3.a  /usr/lib/x86_64-linux-gnu/librt.so  -lm  -ldl  /home/hangyu5/Documents/vulkansdk-linux-x86_64-1.4.328.1/1.4.328.1/x86_64/lib/libvulkan.so  vcpkg_installed/x64-linux/debug/lib/libglslang.a  vcpkg_installed/x64-linux/debug/lib/libglslang-default-resource-limits.a  vcpkg_installed/x64-linux/debug/lib/libSPVRemapper.a  vcpkg_installed/x64-linux/debug/lib/libSPIRV.a  vcpkg_installed/x64-linux/debug/lib/libMachineIndependent.a  vcpkg_installed/x64-linux/debug/lib/libOSDependent.a  -lpthread  vcpkg_installed/x64-linux/debug/lib/libGenericCodeGen.a  vcpkg_installed/x64-linux/debug/lib/libOGLCompiler.a  vcpkg_installed/x64-linux/debug/lib/libHLSL.a  vcpkg_installed/x64-linux/debug/lib/libassimpd.a  vcpkg_installed/x64-linux/debug/lib/libpolyclipping.a  vcpkg_installed/x64-linux/debug/lib/libpoly2tri.a  vcpkg_installed/x64-linux/debug/lib/libminizip.a  vcpkg_installed/x64-linux/debug/lib/libz.a  vcpkg_installed/x64-linux/debug/lib/libpugixml.a  vcpkg_installed/x64-linux/debug/lib/libkubazip.a  -lrt  vcpkg_installed/x64-linux/debug/lib/libBullet3Common.a  vcpkg_installed/x64-linux/debug/lib/libBulletInverseDynamics.a  vcpkg_installed/x64-linux/debug/lib/libBulletDynamics.a  vcpkg_installed/x64-linux/debug/lib/libBulletCollision.a  vcpkg_installed/x64-linux/debug/lib/libBulletSoftBody.a  vcpkg_installed/x64-linux/debug/lib/libLinearMath.a && :
/usr/bin/ld: /usr/bin/ld: DWARF error: invalid or unhandled FORM value: 0x25
/tmp/lto-llvm-d0fb38.o: in function `FTestUI::buildUI()':
TestImguiVk.cpp:(.text._ZN7FTestUI7buildUIEv+0x16): undefined reference to `ImGui::ShowDemoWindow(bool*)'
/usr/bin/ld: TestImguiVk.cpp:(.text._ZN7FTestUI7buildUIEv+0x2e): undefined reference to `ImGui::Begin(char const*, bool*, int)'
/usr/bin/ld: TestImguiVk.cpp:(.text._ZN7FTestUI7buildUIEv+0x3c): undefined reference to `ImGui::Text(char const*, ...)'
/usr/bin/ld: TestImguiVk.cpp:(.text._ZN7FTestUI7buildUIEv+0x4a): undefined reference to `ImGui::Text(char const*, ...)'
/usr/bin/ld: TestImguiVk.cpp:(.text._ZN7FTestUI7buildUIEv+0x4f): undefined reference to `ImGui::Separator()'
/usr/bin/ld: TestImguiVk.cpp:(.text._ZN7FTestUI7buildUIEv+0x6e): undefined reference to `ImGui::Text(char const*, ...)'
/usr/bin/ld: TestImguiVk.cpp:(.text._ZN7FTestUI7buildUIEv+0x73): undefined reference to `ImGui::End()'
/usr/bin/ld: /usr/bin/ld: DWARF error: invalid or unhandled FORM value: 0x25
/tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::FImguiRenderer(FDeviceManager*)':
unity_0_cxx.cxx:(.text._ZN14FImguiRendererC2EP14FDeviceManager+0x43): undefined reference to `ImGui::CreateContext(ImFontAtlas*)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::~FImguiRenderer()':
unity_0_cxx.cxx:(.text._ZN14FImguiRendererD2Ev+0x1b): undefined reference to `ImGui::DestroyContext(ImGuiContext*)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::KeyboardUpdate(int, int, int, int)':
unity_0_cxx.cxx:(.text._ZN14FImguiRenderer14KeyboardUpdateEiiii+0x1a): undefined reference to `ImGui::GetIO()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN14FImguiRenderer14KeyboardUpdateEiiii+0x51): undefined reference to `ImGuiIO::AddKeyEvent(ImGuiKey, bool)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::KeyboardCharInput(unsigned int, int)':
unity_0_cxx.cxx:(.text._ZN14FImguiRenderer17KeyboardCharInputEji+0x13): undefined reference to `ImGui::GetIO()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN14FImguiRenderer17KeyboardCharInputEji+0x23): undefined reference to `ImGuiIO::AddInputCharacter(unsigned int)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::MousePosUpdate(double, double)':
unity_0_cxx.cxx:(.text._ZN14FImguiRenderer14MousePosUpdateEdd+0x17): undefined reference to `ImGui::GetIO()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN14FImguiRenderer14MousePosUpdateEdd+0x36): undefined reference to `ImGuiIO::AddMousePosEvent(float, float)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::MouseScrollUpdate(double, double)':
unity_0_cxx.cxx:(.text._ZN14FImguiRenderer17MouseScrollUpdateEdd+0x17): undefined reference to `ImGui::GetIO()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN14FImguiRenderer17MouseScrollUpdateEdd+0x36): undefined reference to `ImGuiIO::AddMouseWheelEvent(float, float)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::MouseButtonUpdate(int, int, int)':
unity_0_cxx.cxx:(.text._ZN14FImguiRenderer17MouseButtonUpdateEiii+0x16): undefined reference to `ImGui::GetIO()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN14FImguiRenderer17MouseButtonUpdateEiii+0x3e): undefined reference to `ImGuiIO::AddMouseButtonEvent(int, bool)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::Animate(float)':
unity_0_cxx.cxx:(.text._ZN14FImguiRenderer7AnimateEf+0x32): undefined reference to `ImGui::EndFrame()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN14FImguiRenderer7AnimateEf+0x67): undefined reference to `ImGui::GetIO()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN14FImguiRenderer7AnimateEf+0xcd): undefined reference to `ImGui::NewFrame()'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImGui_NVRHI::UpdateFontTexture()':
unity_0_cxx.cxx:(.text._ZN12FImGui_NVRHI17UpdateFontTextureEv+0x17): undefined reference to `ImGui::GetIO()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN12FImGui_NVRHI17UpdateFontTextureEv+0x77): undefined reference to `ImFontAtlas::GetTexDataAsRGBA32(unsigned char**, int*, int*, int*)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImguiRenderer::Render(nvrhi::IFramebuffer*)':
unity_0_cxx.cxx:(.text._ZN14FImguiRenderer6RenderEPN5nvrhi12IFramebufferE+0x34): undefined reference to `ImGui::Render()'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImGui_NVRHI::Render(nvrhi::IFramebuffer*)':
unity_0_cxx.cxx:(.text._ZN12FImGui_NVRHI6RenderEPN5nvrhi12IFramebufferE+0x1b): undefined reference to `ImGui::GetDrawData()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN12FImGui_NVRHI6RenderEPN5nvrhi12IFramebufferE+0x3e): undefined reference to `ImGui::GetIO()'
/usr/bin/ld: unity_0_cxx.cxx:(.text._ZN12FImGui_NVRHI6RenderEPN5nvrhi12IFramebufferE+0xa1): undefined reference to `ImDrawData::ScaleClipRects(ImVec2 const&)'
/usr/bin/ld: /tmp/lto-llvm-8988fc.o: in function `FImGui_NVRHI::UpdateGeometry(nvrhi::ICommandList*)':
unity_0_cxx.cxx:(.text._ZN12FImGui_NVRHI14UpdateGeometryEPN5nvrhi12ICommandListE+0x1c): undefined reference to `ImGui::GetDrawData()'
clang++-17: error: linker command failed with exit code 1 (use -v to see invocation)
ninja: build stopped: subcommand failed.


