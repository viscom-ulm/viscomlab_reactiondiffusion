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
                ImGui::SliderFloat("Draw Distance", &GetSimulationData().simulationDrawDistance_, 5.0f, 20.0f);
                ImGui::SliderFloat("Height", &GetSimulationData().simulationHeight_, 0.1f, 1.0f);
                ImGui::SliderFloat("Eta", &GetSimulationData().eta_, 1.0f, 5.0f);
                ImGui::SliderFloat("Absorption", &GetSimulationData().sigma_a_, 0.5f, 5.0f);
            }
            ImGui::End();
        });

        ApplicationNodeImplementation::Draw2D(fbo);
    }

    void MasterNode::CleanUp()
    {
        ApplicationNodeImplementation::CleanUp();
    }

    void MasterNode::KeyboardCallback(int key, int scancode, int action, int mods)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_KeyCallback(key, scancode, action, mods);
#endif
        ApplicationNodeImplementation::KeyboardCallback(key, scancode, action, mods);
    }

    void MasterNode::CharCallback(unsigned int character, int mods)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_CharCallback(character);
#endif
        ApplicationNodeImplementation::CharCallback(character, mods);
    }

    void MasterNode::MouseButtonCallback(int button, int action)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
#endif
        ApplicationNodeImplementation::MouseButtonCallback(button, action);
    }

    void MasterNode::MousePosCallback(double x, double y)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
#endif
        ApplicationNodeImplementation::MousePosCallback(x, y);
    }

    void MasterNode::MouseScrollCallback(double xoffset, double yoffset)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_ScrollCallback(xoffset, yoffset);
#endif
        ApplicationNodeImplementation::MouseScrollCallback(xoffset, yoffset);
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
