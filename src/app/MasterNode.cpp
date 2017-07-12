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
#include <fstream>

namespace viscom {

    MasterNode::MasterNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode }
    {
    }

    MasterNode::~MasterNode() = default;

    void MasterNode::InitOpenGL()
    {
        ApplicationNodeImplementation::InitOpenGL();

        LoadPresetList();

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
            seed_points.emplace_back(seedIterationCount, FindIntersectionWithPlane(GetCamera()->GetPickRay(currentMouseCursorPosition_)));
        } else if (currentMouseButton_ == GLFW_MOUSE_BUTTON_2 && currentMouseAction_ == GLFW_PRESS) {
            SimulationData& sim_data = GetSimulationData();
            sim_data.resetFrameIdx_ = seedIterationCount;
        }

        for (const auto& tpos : tuioCursorPositions_) {
            seed_points.emplace_back(seedIterationCount, FindIntersectionWithPlane(GetCamera()->GetPickRay(tpos.second)));
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

                if (presetNames_.size() > 1) {
                    int selectedPreset = 0;
                    if (ImGui::Combo("Select Preset", &selectedPreset, presetNamesCStr_.data(), static_cast<int>(presetNamesCStr_.size()))) LoadPreset(selectedPreset);
                }
                static std::string presetName;
                presetName.resize(255);
                ImGui::InputText("Preset Name", presetName.data(), static_cast<int>(presetName.size()));
                if (ImGui::Button("Save Preset")) SavePreset(presetName.c_str());

                ImGui::Combo("Select Renderer", &simData.currentRenderer_, rendererNamesCStr_.data(), static_cast<int>(rendererNamesCStr_.size()));

                if (ImGui::TreeNode("Plane Parameters")) {
                    ImGui::SliderFloat("Draw Distance", &simData.simulationDrawDistance_, 5.0f, 20.0f);
                    ImGui::TreePop();
                }

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
        for (auto& tuioCursorPosition : tuioCursorPositions_) {
            if (tuioCursorPosition.first == tcur->getCursorID()) {
                LOG(WARNING) << "TUIO cursor (" << tcur->getCursorID() << ") added while already present.";
                tuioCursorPosition.second = glm::vec2(tcur->getX(), tcur->getY());
                return false;
            }
        }

        tuioCursorPositions_.emplace_back(tcur->getCursorID(), glm::vec2(tcur->getX(), tcur->getY()));
        return true;
    }

    bool MasterNode::UpdateTuioCursor(TUIO::TuioCursor* tcur)
    {
        for (auto& tuioCursorPosition : tuioCursorPositions_) {
            if (tuioCursorPosition.first == tcur->getCursorID()) {
                tuioCursorPosition.second = glm::vec2(tcur->getX(), tcur->getY());
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

    void MasterNode::LoadPresetList()
    {
        presetNames_.emplace_back("None", "");

        std::string presetListFile = GetConfig().resourceSearchPaths_.back() + "/presetList.txt";
        if (!utils::file_exists(presetListFile)) return;

        std::ifstream ifs(presetListFile);
        std::string str;
        while (ifs >> str && ifs.good()) {
            std::string presetName = str;
            std::string presetFile;
            ifs >> presetFile;
            presetNames_.emplace_back(presetName, presetFile);
        }

        UpdatePresetNames();
    }

    void MasterNode::UpdatePresetNames()
    {
        presetNamesCStr_.clear();
        for (const auto& pName : presetNames_) {
            presetNamesCStr_.push_back(pName.first.c_str());
        }
    }

    void MasterNode::LoadPreset(int preset)
    {
        if (preset == 0) return;

        std::string presetFile = GetConfig().resourceSearchPaths_.back() + "/" + presetNames_[preset].second;
        if (!utils::file_exists(presetFile)) return;

        std::ifstream ifs(presetFile);
        std::string str;

        while (ifs >> str && ifs.good()) {
            if (str == "simulationDrawDistance=") ifs >> GetSimulationData().simulationDrawDistance_;
            else if (str == "simulationHeight=") ifs >> GetSimulationData().simulationHeight_;
            else if (str == "eta=") ifs >> GetSimulationData().eta_;
            else if (str == "sigma_a.r=") ifs >> GetSimulationData().sigma_a_.r;
            else if (str == "sigma_a.g=") ifs >> GetSimulationData().sigma_a_.g;
            else if (str == "sigma_a.b=") ifs >> GetSimulationData().sigma_a_.b;
            else if (str == "diffusion_rate_a=") ifs >> GetSimulationData().diffusion_rate_a_;
            else if (str == "diffusion_rate_b=") ifs >> GetSimulationData().diffusion_rate_b_;
            else if (str == "feed_rate=") ifs >> GetSimulationData().feed_rate_;
            else if (str == "kill_rate=") ifs >> GetSimulationData().kill_rate_;
            else if (str == "dt=") ifs >> GetSimulationData().dt_;
            else if (str == "seed_point_radius=") ifs >> GetSimulationData().seed_point_radius_;
            else if (str == "use_manhattan_distance=") ifs >> GetSimulationData().use_manhattan_distance_;
            else if (str == "currentRenderer=") ifs >> GetSimulationData().currentRenderer_;
        }
    }

    void MasterNode::SavePreset(const std::string& presetName)
    {
        std::string presetFile = GetConfig().resourceSearchPaths_.back() + "/" + presetName + ".txt";
        std::ofstream ofs(presetFile, std::ofstream::trunc);

        ofs << "simulationDrawDistance= " << GetSimulationData().simulationDrawDistance_ << std::endl;
        ofs << "simulationHeight= " << GetSimulationData().simulationHeight_ << std::endl;
        ofs << "eta= " << GetSimulationData().eta_ << std::endl;
        ofs << "sigma_a.r= " << GetSimulationData().sigma_a_.r << std::endl;
        ofs << "sigma_a.g= " << GetSimulationData().sigma_a_.g << std::endl;
        ofs << "sigma_a.b= " << GetSimulationData().sigma_a_.b << std::endl;
        ofs << "diffusion_rate_a= " << GetSimulationData().diffusion_rate_a_ << std::endl;
        ofs << "diffusion_rate_b= " << GetSimulationData().diffusion_rate_b_ << std::endl;
        ofs << "feed_rate= " << GetSimulationData().feed_rate_ << std::endl;
        ofs << "kill_rate= " << GetSimulationData().kill_rate_ << std::endl;
        ofs << "dt= " << GetSimulationData().dt_ << std::endl;
        ofs << "seed_point_radius= " << GetSimulationData().seed_point_radius_ << std::endl;
        ofs << "use_manhattan_distance= " << GetSimulationData().use_manhattan_distance_ << std::endl;
        ofs << "currentRenderer= " << GetSimulationData().currentRenderer_ << std::endl;

        presetNames_.emplace_back(presetName, presetName + ".pst");
        UpdatePresetNames();

        std::string presetListFile = GetConfig().resourceSearchPaths_.back() + "/presetList.txt";

        std::ofstream ofsList(presetListFile, std::ofstream::app);
        if (ofsList.good()) ofsList << presetName << " " << presetName + ".txt" << std::endl;
    }

    glm::vec2 MasterNode::FindIntersectionWithPlane(const math::Line3<float>& ray) const
    {
        glm::mat3 m{ 0.0f };
        m[0] = ray[0] - ray[1];
        m[1] = GetSimPlane().right_ - GetSimPlane().position_;
        m[2] = GetSimPlane().up_ - GetSimPlane().position_;

        auto intersection = glm::inverse(m) * (ray[0] - GetSimPlane().position_);
        return glm::vec2(0.5f) + glm::vec2(intersection.y, intersection.z) / 2.0f;
    }
}
