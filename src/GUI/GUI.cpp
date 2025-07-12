#include "PCH.h"
#include "GUI.h"
#include "Menu.h"
#include "Resources/Ubuntu.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "Tools/stb_image.h"
#include "resource.h"

namespace GUI {
    bool dragging = false;
    POINT dragStartMousePos = { 0, 0 };
    int dragStartWindowX = 0;
    int dragStartWindowY = 0;
    GLFWwindow* window = nullptr;
    std::atomic<bool> render_running = true;
    std::thread render_thread;

    void HandleWindowDrag(GLFWwindow* window, const ImVec2& dragAreaPos, const ImVec2& dragAreaSize) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 windowPos = ImGui::GetWindowPos();

        ImVec2 relMousePos(mousePos.x - windowPos.x, mousePos.y - windowPos.y);

        bool hoveringDragArea =
            relMousePos.x >= dragAreaPos.x && relMousePos.x <= dragAreaPos.x + dragAreaSize.x &&
            relMousePos.y >= dragAreaPos.y && relMousePos.y <= dragAreaPos.y + dragAreaSize.y;

        if (hoveringDragArea && ImGui::IsMouseClicked(0)) {
            dragging = true;
            GetCursorPos(&dragStartMousePos);
            glfwGetWindowPos(window, &dragStartWindowX, &dragStartWindowY);
        }

        if (!ImGui::IsMouseDown(0)) {
            dragging = false;
        }

        if (dragging) {
            POINT currentPos;
            GetCursorPos(&currentPos);

            int dx = currentPos.x - dragStartMousePos.x;
            int dy = currentPos.y - dragStartMousePos.y;

            glfwSetWindowPos(window, dragStartWindowX + dx, dragStartWindowY + dy);
        }
    }

    void CenterWindow(GLFWwindow* window) {
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        int cursorX, cursorY;
        glfwGetCursorPos(window, (double*)&cursorX, (double*)&cursorY);
        GLFWmonitor* launchMonitor = glfwGetPrimaryMonitor();
        int monitorX, monitorY;

        glfwGetMonitorPos(launchMonitor, &monitorX, &monitorY);
        int monitorWidth    = glfwGetVideoMode(launchMonitor)->width;
        int monitorHeight   = glfwGetVideoMode(launchMonitor)->height;

        int posX = monitorX + (monitorWidth - windowWidth) / 2;
        int posY = monitorY + (monitorHeight - windowHeight) / 2;
        glfwSetWindowPos(window, posX, posY);
    }

    float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    enum ResizeEdge {
        NONE = 0,
        LEFT = 1 << 0,
        RIGHT = 1 << 1,
        TOP = 1 << 2,
        BOTTOM = 1 << 3
    };

    void HandleManualResize(GLFWwindow* window) {
        const int BORDER = 6;
        const float LERP_SPEED = 0.25f;

        static int resizeEdge = ResizeEdge::NONE;
        static bool resizing = false;
        static POINT startMouse = {};
        static int startWidth = 0, startHeight = 0;
        static int startX = 0, startY = 0;
        static int targetW = 0, targetH = 0;
        static int targetX = 0, targetY = 0;

        int winW, winH, winX, winY;
        glfwGetWindowSize(window, &winW, &winH);
        glfwGetWindowPos(window, &winX, &winY);

        static bool initialized = false;
        if (!initialized) {
            targetW = winW;
            targetH = winH;
            targetX = winX;
            targetY = winY;
            initialized = true;
        }

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        int edge = ResizeEdge::NONE;

        if (mouseX < BORDER)
            edge |= ResizeEdge::LEFT;
        else if (mouseX > winW - BORDER)
            edge |= ResizeEdge::RIGHT;

        if (mouseY < BORDER)
            edge |= ResizeEdge::TOP;
        else if (mouseY > winH - BORDER)
            edge |= ResizeEdge::BOTTOM;

        if (!resizing) {
            if (edge == (LEFT | TOP) || edge == (RIGHT | BOTTOM))
                glfwSetCursor(window, glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR));
            else if (edge == (RIGHT | TOP) || edge == (LEFT | BOTTOM))
                glfwSetCursor(window, glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR));
            else if (edge & (LEFT | RIGHT))
                glfwSetCursor(window, glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR));
            else if (edge & (TOP | BOTTOM))
                glfwSetCursor(window, glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR));
            else
                glfwSetCursor(window, nullptr);
        }

        if (!resizing && edge != ResizeEdge::NONE && ImGui::IsMouseClicked(0)) {
            resizing = true;
            resizeEdge = edge;
            GetCursorPos(&startMouse);
            glfwGetWindowSize(window, &startWidth, &startHeight);
            glfwGetWindowPos(window, &startX, &startY);
            targetW = startWidth;
            targetH = startHeight;
            targetX = startX;
            targetY = startY;
        }

        if (resizing) {
            if (!ImGui::IsMouseDown(0)) {
                resizing = false;
                resizeEdge = ResizeEdge::NONE;
                return;
            }

            POINT curMouse;
            GetCursorPos(&curMouse);
            int dx = curMouse.x - startMouse.x;
            int dy = curMouse.y - startMouse.y;

            targetX = startX;
            targetY = startY;
            targetW = startWidth;
            targetH = startHeight;

            if (resizeEdge & ResizeEdge::RIGHT)
                targetW = max(100, startWidth + dx);
            if (resizeEdge & ResizeEdge::BOTTOM)
                targetH = max(100, startHeight + dy);
            if (resizeEdge & ResizeEdge::LEFT) {
                targetW = max(100, startWidth - dx);
                targetX = startX + dx;
            }
            if (resizeEdge & ResizeEdge::TOP) {
                targetH = max(100, startHeight - dy);
                targetY = startY + dy;
            }
        }
        else {
            targetX = winX;
            targetY = winY;
            targetW = winW;
            targetH = winH;
        }

        if (resizing) {
            int newX = static_cast<int>(Lerp(winX, targetX, LERP_SPEED));
            int newY = static_cast<int>(Lerp(winY, targetY, LERP_SPEED));
            int newW = static_cast<int>(Lerp(winW, targetW, LERP_SPEED));
            int newH = static_cast<int>(Lerp(winH, targetH, LERP_SPEED));

            glfwSetWindowPos(window, newX, newY);
            glfwSetWindowSize(window, newW, newH);
        }
    }

    void RenderLoop() {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(IDB_PNG1), "PNG");
        HGLOBAL hData = LoadResource(nullptr, hRes);
        void* pData = LockResource(hData);
        DWORD pDataSize = SizeofResource(nullptr, hRes);
        GLFWimage icons[1];
        icons[0].pixels = stbi_load_from_memory((stbi_uc*)pData, pDataSize, &icons[0].width, &icons[0].height, 0, 4);
        glfwSetWindowIcon(window, 1, icons);
        stbi_image_free(icons[0].pixels);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        ImGui::GetStyle().FontSizeBase = 16.0f;

        ImFontConfig FontCfg;
        FontCfg.FontDataOwnedByAtlas = false;
        FontCfg.FontLoaderFlags |= ImGuiFreeTypeBuilderFlags_ForceAutoHint;
        io.Fonts->AddFontFromMemoryCompressedBase85TTF(ubuntu_compressed_data_base85, 72.f, &FontCfg, io.Fonts->GetGlyphRangesCyrillic());

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        while (render_running && !glfwWindowShouldClose(window)) {
            ImGuiIO& io = ImGui::GetIO();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            HandleManualResize(window);

            Menu::DrawMainWindow(window);

            ImGui::Render();
            glViewport(0, 0, display_w, display_h);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void Init() {
        if (!glfwInit()) return;

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

        window = glfwCreateWindow(1280, 720, "Growl", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            return;
        }

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        CenterWindow(window);

        render_thread = std::thread(RenderLoop);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }

        render_running = false;
        render_thread.join();

        glfwDestroyWindow(window);
        glfwTerminate();

        if (std::filesystem::exists(std::filesystem::u8path(Utils::GetAppDirectory()) / "growl_tmpfiles")) {
            std::filesystem::remove_all(std::filesystem::u8path(Utils::GetAppDirectory()) / "growl_tmpfiles");
        }
    }
}