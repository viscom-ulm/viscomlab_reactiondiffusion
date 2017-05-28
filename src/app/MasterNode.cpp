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
#include "renderers/RDRenderer.h"

namespace viscom {

    MasterNode::MasterNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode }
    {
    }

    MasterNode::~MasterNode() = default;

    void MasterNode::InitOpenGL()
    {
        ApplicationNodeImplementation::InitOpenGL();

        for (const auto& renderer : GetRenderers()) {
            rendererNames_.push_back(renderer->GetName());
        }

        for (const auto& rName : rendererNames_) {
            rendererNamesCStr_.push_back(rName.c_str());
        }
    }

    void MasterNode::PreSync()
    {
        ApplicationNodeImplementation::PreSync();
#ifdef VISCOM_USE_SGCT
        sharedData_.setVal(GetSimulationData());
        sharedSeedPoints_.setVal(GetSeedPoints());

        auto syncPoint = syncedTimestamp_.getVal();
#else
        auto syncPoint = GetCurrentLocalIterationCount();
#endif
        // iterate GetSeedPoints, delete all seed points before syncPoint
        auto lastDel = GetSeedPoints().begin();
        for (; lastDel != GetSeedPoints().end() && lastDel->first < syncPoint; ++lastDel);
        if (lastDel != GetSeedPoints().begin()) {
            GetSeedPoints().erase(GetSeedPoints().begin(), lastDel);
        }
    }

    void MasterNode::UpdateFrame(double currentTime, double elapsedTime)
    {
        auto seedIterationCount = GetSimulationData().currentGlobalIterationCount_ + 1;
        GetSimulationData().currentGlobalIterationCount_ += ApplicationNodeImplementation::FRAME_ITERATIONS_INC;

        auto& seed_points = GetSeedPoints();
        if (currentMouseButton_ == GLFW_MOUSE_BUTTON_1 && currentMouseAction_ == GLFW_PRESS) {
            const float x = currentMouseCursorPosition_.x;
            const float y = currentMouseCursorPosition_.y;
            seed_points.emplace_back(seedIterationCount, glm::vec2(x, 1.0f - y));
        } else if (currentMouseButton_ == GLFW_MOUSE_BUTTON_2 && currentMouseAction_ == GLFW_PRESS) {
            SimulationData& sim_data = GetSimulationData();
            sim_data.resetFrameIdx_ = seedIterationCount;
        }

        for (const auto& tpos : tuioCursorPositions_) {
            seed_points.emplace_back(seedIterationCount, glm::vec2(tpos.second.x, 1.0f - tpos.second.y));
        }

        ApplicationNodeImplementation::UpdateFrame(currentTime, elapsedTime);
    }

    void MasterNode::Draw2D(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([this]() {
            ImGui::SetNextWindowSize(ImVec2(480.0f, 200.0f), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(1920.0f - 480.0f - 10.0f, 1080.0f - 200.0f - 10.0f), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Simulation Parameters");
            {
                SimulationData& simData = GetSimulationData();

                ImGui::Combo("Select Renderer", &simData.currentRenderer_, rendererNamesCStr_.data(), static_cast<int>(rendererNamesCStr_.size()));

                if (ImGui::TreeNode("Rendering Parameters")) {
                    GetRenderers()[simData.currentRenderer_]->DrawOptionsGUI(simData);
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

    bool MasterNode::MouseButtonCallback(int button, int action)
    {
        if (!ApplicationNodeImplementation::MouseButtonCallback(button, action)) {
            currentMouseAction_ = action;
            currentMouseButton_ = button;
        }
        return true;
    }

    bool MasterNode::MousePosCallback(double x, double y)
    {
        if (!ApplicationNodeImplementation::MousePosCallback(x, y)) {
            currentMouseCursorPosition_ = glm::vec2{x, y};
        }
        return true;
    }

#ifdef WITH_TUIO
    bool MasterNode::AddTuioCursor(TUIO::TuioCursor* tcur)
    {
        for (int i = 0; i < tuioCursorPositions_.size(); ++i) {
            if (tuioCursorPositions_[i].first == tcur->getCursorID()) {
                LOG(WARNING) << "TUIO cursor (" << tcur->getCursorID() << ") added while already present.";
                tuioCursorPositions_[i].second = glm::vec2(tcur->getX(), tcur->getY());
                return false;
            }
        }

        tuioCursorPositions_.emplace_back(tcur->getCursorID(), glm::vec2(tcur->getX(), tcur->getY()));
        return true;
    }

    bool MasterNode::UpdateTuioCursor(TUIO::TuioCursor* tcur)
    {
        for (int i = 0; i < tuioCursorPositions_.size(); ++i) {
            if (tuioCursorPositions_[i].first == tcur->getCursorID()) {
                tuioCursorPositions_[i].second = glm::vec2(tcur->getX(), tcur->getY());
                return true;
            }
        }

        LOG(WARNING) << "TUIO cursor (" << tcur->getCursorID() << ") updated but not present.";
        return false;
    }

    bool MasterNode::RemoveTuioCursor(TUIO::TuioCursor* tcur)
    {
        int localId = -1;
        for (int i = 0; i < tuioCursorPositions_.size(); ++i) {
            if (tuioCursorPositions_[i].first == tcur->getCursorID()) {
                localId = i;
                break;
            }
        }

        if (localId == -1) {
            LOG(WARNING) << "TUIO cursor (" << tcur->getCursorID() << ") deleted but not present.";
            return false;
        }
        else {
            tuioCursorPositions_.erase(tuioCursorPositions_.begin() + localId);
            return true;
        }
    }
#endif

#ifdef VISCOM_USE_SGCT
    void MasterNode::EncodeData()
    {
        ApplicationNodeImplementation::EncodeData();
        sgct::SharedData::instance()->writeObj(&sharedData_);
        sgct::SharedData::instance()->writeVector(&sharedSeedPoints_);
        syncedTimestamp_.setVal(sharedData_.getVal().currentGlobalIterationCount_);
    }

    void MasterNode::DecodeData()
    {
        ApplicationNodeImplementation::DecodeData();
        sgct::SharedData::instance()->readObj(&sharedData_);
        sgct::SharedData::instance()->readVector(&sharedSeedPoints_);
    }
#endif
}
