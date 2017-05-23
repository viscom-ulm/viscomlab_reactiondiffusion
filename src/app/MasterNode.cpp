/**
 * @file   MasterNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the master application node.
 */

#include "MasterNode.h"
#include <imgui.h>
#include "core/imgui/imgui_impl_glfw_gl3.h"

namespace viscom {

    MasterNode::MasterNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode }
    {
    }

    MasterNode::~MasterNode() = default;


    void MasterNode::InitOpenGL()
    {
        ApplicationNodeImplementation::InitOpenGL();
    }

    void MasterNode::PreSync()
    {
        ApplicationNodeImplementation::PreSync();
        sharedData_.setVal(GetSimulationData());
    }

    void MasterNode::UpdateFrame(double currentTime, double elapsedTime)
    {
        ApplicationNodeImplementation::UpdateFrame(currentTime, elapsedTime);
        GetSimulationData().currentGlobalIterationCount_ += ApplicationNodeImplementation::MAX_FRAME_ITERATIONS;
    }

    void MasterNode::DrawFrame(FrameBuffer& fbo)
    {
        ApplicationNodeImplementation::DrawFrame(fbo);
    }

    void MasterNode::Draw2D(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([this]() {
            ImGui::SetNextWindowSize(ImVec2(480.0f, 200.0f), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(1920.0f - 480.0f - 10.0f, 1080.0f - 200.0f - 10.0f), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Simulation Parameters");
            {
                SimulationData& simData = GetSimulationData();

                if (ImGui::TreeNode("Rendering Parameters")) {
                    ImGui::SliderFloat("Draw Distance", &simData.simulationDrawDistance_, 5.0f, 20.0f);
                    ImGui::SliderFloat("Height", &simData.simulationHeight_, 0.02f, 0.5f);
                    ImGui::SliderFloat("Eta", &simData.eta_, 1.0f, 5.0f);
                    ImGui::SliderFloat("Absorption Red", &simData.sigma_a_.r, 0.0f, 100.0f);
                    ImGui::SliderFloat("Absorption Green", &simData.sigma_a_.g, 0.0f, 100.0f);
                    ImGui::SliderFloat("Absorption Blue", &simData.sigma_a_.b, 0.0f, 100.0f);
                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Reaction Diffusion Parameters")) {
                    ImGui::SliderFloat("Diffusion Rate A", &simData.diffusion_rate_a_, 0.0f, 2.0f);
                    ImGui::SliderFloat("Diffusion Rate B", &simData.diffusion_rate_b_, 0.0f, 2.0f);
                    ImGui::SliderFloat("Feed Rate", &simData.feed_rate_, 0.0f, 0.2f);
                    ImGui::SliderFloat("Kill Rate", &simData.kill_rate_, 0.0f, 0.2f);
                    ImGui::SliderFloat("Dt", &simData.dt_, 0.0f, 5.0f);
                    ImGui::SliderFloat("Seed Point Radius", &simData.seed_point_radius_, 0.01f, 1.0f);
                    ImGui::Checkbox("Use Manhattan Distance", &simData.use_manhattan_distance_);
                    ImGui::TreePop();
                }
            }
            ImGui::End();
        });

        ApplicationNodeImplementation::Draw2D(fbo);
    }

    void MasterNode::CleanUp()
    {
        ApplicationNodeImplementation::CleanUp();
    }

    bool MasterNode::KeyboardCallback(int key, int scancode, int action, int mods)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_KeyCallback(key, scancode, action, mods);
        if (ImGui::GetIO().WantCaptureKeyboard) return true;
#endif
        return ApplicationNodeImplementation::KeyboardCallback(key, scancode, action, mods);
    }

    bool MasterNode::CharCallback(unsigned int character, int mods)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_CharCallback(character);
        if (ImGui::GetIO().WantCaptureKeyboard) return true;
#endif
        return ApplicationNodeImplementation::CharCallback(character, mods);
    }

    bool MasterNode::MouseButtonCallback(int button, int action)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
        if (ImGui::GetIO().WantCaptureMouse) return true;
#endif
        return ApplicationNodeImplementation::MouseButtonCallback(button, action);
    }

    bool MasterNode::MousePosCallback(double x, double y)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
        if (ImGui::GetIO().WantCaptureMouse) return true;
#endif
        return ApplicationNodeImplementation::MousePosCallback(x, y);
    }

    bool MasterNode::MouseScrollCallback(double xoffset, double yoffset)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_ScrollCallback(xoffset, yoffset);
        if (ImGui::GetIO().WantCaptureMouse) return true;
#endif
        return ApplicationNodeImplementation::MouseScrollCallback(xoffset, yoffset);
    }

    void MasterNode::EncodeData()
    {
        ApplicationNodeImplementation::EncodeData();
        sgct::SharedData::instance()->writeObj(&sharedData_);
    }

    void MasterNode::DecodeData()
    {
        ApplicationNodeImplementation::DecodeData();
        sgct::SharedData::instance()->readObj(&sharedData_);
    }

}
