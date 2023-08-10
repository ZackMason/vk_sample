// cmake --build . --config Release 

#include "imgui.h"
#include "../bindings/imgui_impl_glfw.h"
#include "../bindings/imgui_impl_opengl2.h"
#include <stdio.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

#include <thread>
#include <iostream>
#include <vector>
#include <string>
#include <span>

void AlignForWidth(float width, float alignment = 0.5f)
{
    float avail = ImGui::GetContentRegionAvail().x;
    float off = (avail - width) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

void AlignForButtons(std::span<const char*> button_text, float alignment = 0.5f) {
    ImGuiStyle& style = ImGui::GetStyle();
    float width = 0.0f;
    for (auto text: button_text) {
        width += ImGui::CalcTextSize(text).x;
        width += style.ItemSpacing.x*2;
    }
    width -= style.ItemSpacing.x;
    AlignForWidth(width, alignment);
}

int main() { 
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(1280, 720, "ZYY Launcher", NULL, NULL);

    const auto pressed=[&window](int k) {
        return glfwGetKey(window, k) == GLFW_PRESS;
    };

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.WindowPadding = ImVec2(0, 0);
    style.FramePadding = ImVec2(15, 10);
    style.ItemSpacing = ImVec2(10, 5);
    style.Colors[ImGuiCol_Button] = ImColor(0x2a, 0x1d, 0x1f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    // state
    std::vector<std::string> messages;
    messages.reserve(100);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (pressed(GLFW_KEY_ESCAPE)) {
            break;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::Begin("##Launcher", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

            // ImGui::SetCursorPosX(ImGui::GetIO().DisplaySize.x * 0.5f); // Adjust Y position for the same line
            ImGui::SetCursorPosX(0.0f);

            const char* buttons[]{
                "Build Game",
                "Build Platform",
                "Build Physics",
            };
            AlignForButtons(buttons);
            if (ImGui::Button("Build Game")) {
                std::thread([](){
                    std::system("build g game");
                }).detach();
            }
            ImGui::SameLine();
            if (ImGui::Button("Build Platform")) {
                std::thread([](){
                    std::system("build win32");
                }).detach();
            }
            ImGui::SameLine();
            if (ImGui::Button("Build Physics")) {
                std::thread([](){
                    std::system("build physics");
                }).detach();
            }
            
            ImGui::Indent(16.0f);
            ImGui::SetCursorPosY(ImGui::GetIO().DisplaySize.y * 0.9f);
            if (ImGui::Button("Launch")) {
                std::thread([](){
                    std::system("run");
                }).detach();
            }
            ImGui::SameLine();
            if (ImGui::Button("Tests")) {
                std::thread([](){
                    std::system("./build/tests.exe");
                }).detach();
            }
            


            ImGui::End();
        }
    
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}