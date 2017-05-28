/**
 * @file   HeightfieldRaycaster.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.05.28
 *
 * @brief  Implementation of the heighfield raycaster renderer.
 */

#include "HeightfieldRaycaster.h"
#include "app/ApplicationNodeImplementation.h"
#include <imgui.h>

namespace viscom::renderers {

    HeightfieldRaycaster::HeightfieldRaycaster(ApplicationNodeImplementation* appNode) :
        RDRenderer{ "HeightfieldRaycaster", appNode }
    {
        FrameBufferDescriptor simulationBackFBDesc;
        simulationBackFBDesc.texDesc_.emplace_back(GL_RG32F, GL_TEXTURE_2D);
        simulationBackFBDesc.rbDesc_.emplace_back(GL_DEPTH_COMPONENT32);
        simulationBackFBOs_ = appNode_->CreateOffscreenBuffers(simulationBackFBDesc);

        raycastBackProgram_ = appNode_->GetGPUProgramManager().GetResource("raycastHeightfieldBack", std::initializer_list<std::string>{ "raycastHeightfield.vert", "raycastHeightfieldBack.frag" });
        raycastBackVPLoc_ = raycastBackProgram_->getUniformLocation("viewProjectionMatrix");
        raycastBackQuadSizeLoc_ = raycastBackProgram_->getUniformLocation("quadSize");
        raycastBackDistanceLoc_ = raycastBackProgram_->getUniformLocation("distance");
        raycastProgram_ = appNode_->GetGPUProgramManager().GetResource("raycastHeightfield", std::initializer_list<std::string>{ "raycastHeightfield.vert", "raycastHeightfield.frag" });
        raycastVPLoc_ = raycastProgram_->getUniformLocation("viewProjectionMatrix");
        raycastQuadSizeLoc_ = raycastProgram_->getUniformLocation("quadSize");
        raycastDistanceLoc_ = raycastProgram_->getUniformLocation("distance");
        raycastSimHeightLoc_ = raycastProgram_->getUniformLocation("simulationHeight");
        raycastCamPosLoc_ = raycastProgram_->getUniformLocation("cameraPosition");
        raycastEtaLoc_ = raycastProgram_->getUniformLocation("eta");
        raycastSigmaALoc_ = raycastProgram_->getUniformLocation("sigma_a");
        raycastEnvMapLoc_ = raycastProgram_->getUniformLocation("environment");
        raycastBGTexLoc_ = raycastProgram_->getUniformLocation("backgroundTexture");
        raycastHeightTextureLoc_ = raycastProgram_->getUniformLocation("heightTexture");
        raycastPositionBackTexLoc_ = raycastProgram_->getUniformLocation("backPositionTexture");

        glGenVertexArrays(1, &simDummyVAO_);
        backgroundTexture_ = appNode_->GetTextureManager().GetResource("models/teapot/default.png");
        environmentMap_ = appNode_->GetTextureManager().GetResource("textures/grace_probe.hdr");
    }

    HeightfieldRaycaster::~HeightfieldRaycaster()
    {
        if (simDummyVAO_ != 0) glDeleteVertexArrays(1, &simDummyVAO_);
        simDummyVAO_ = 0;
    }

    void HeightfieldRaycaster::ClearBuffers(FrameBuffer& fbo)
    {
        appNode_->SelectOffscreenBuffer(simulationBackFBOs_)->DrawToFBO([]() {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        });

        fbo.DrawToFBO([]() {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        });
    }

    void HeightfieldRaycaster::UpdateFrame(double, double, const SimulationData& simData, const glm::vec2& nearPlaneSize)
    {
        float userDistance = appNode_->GetCamera()->GetUserPosition().z;
        // TODO: maybe calculate the correct center? (ray through userPosition, (0,0,0) -> hits z=simulationDrawDistance_) [5/27/2017 Sebastian Maisch]
        simulationOutputSize_ = nearPlaneSize * (userDistance + simData.simulationDrawDistance_) / userDistance;
    }

    void HeightfieldRaycaster::RenderRDResults(FrameBuffer& fbo, const SimulationData& simData, const glm::mat4& perspectiveMatrix, GLuint rdTexture)
    {
        appNode_->SelectOffscreenBuffer(simulationBackFBOs_)->DrawToFBO([this, &perspectiveMatrix, &simData]() {
            glBindVertexArray(simDummyVAO_);
            glUseProgram(raycastBackProgram_->getProgramId());
            glUniformMatrix4fv(raycastBackVPLoc_, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
            glUniform2fv(raycastBackQuadSizeLoc_, 1, glm::value_ptr(simulationOutputSize_));
            glUniform1f(raycastBackDistanceLoc_, simData.simulationDrawDistance_);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        });

        fbo.DrawToFBO([this, &perspectiveMatrix, &simData, rdTexture]() {
            glm::vec3 camPos = appNode_->GetCamera()->GetPosition();
            glBindVertexArray(simDummyVAO_);
            glUseProgram(raycastProgram_->getProgramId());
            glUniformMatrix4fv(raycastVPLoc_, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
            glUniform2fv(raycastQuadSizeLoc_, 1, glm::value_ptr(simulationOutputSize_));
            glUniform1f(raycastDistanceLoc_, simData.simulationDrawDistance_ - simData.simulationHeight_);
            glUniform1f(raycastSimHeightLoc_, simData.simulationHeight_);
            glUniform3fv(raycastCamPosLoc_, 1, glm::value_ptr(camPos));
            glUniform1f(raycastEtaLoc_, simData.eta_);
            glUniform3fv(raycastSigmaALoc_, 1, glm::value_ptr(simData.sigma_a_));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, environmentMap_->getTextureId());
            glUniform1i(raycastEnvMapLoc_, 0);

            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, backgroundTexture_->getTextureId());
            glUniform1i(raycastBGTexLoc_, 1);

            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D, rdTexture);
            glUniform1i(raycastHeightTextureLoc_, 2);

            glBindImageTexture(0, appNode_->SelectOffscreenBuffer(simulationBackFBOs_)->GetTextures()[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
            glUniform1i(raycastPositionBackTexLoc_, 0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        });
    }

    void HeightfieldRaycaster::DrawOptionsGUI(SimulationData& simData) const
    {
        ImGui::SliderFloat("Draw Distance", &simData.simulationDrawDistance_, 5.0f, 20.0f);
        ImGui::SliderFloat("Height", &simData.simulationHeight_, 0.02f, 0.5f);
        ImGui::SliderFloat("Eta", &simData.eta_, 1.0f, 5.0f);
        ImGui::SliderFloat("Absorption Red", &simData.sigma_a_.r, 0.0f, 100.0f);
        ImGui::SliderFloat("Absorption Green", &simData.sigma_a_.g, 0.0f, 100.0f);
        ImGui::SliderFloat("Absorption Blue", &simData.sigma_a_.b, 0.0f, 100.0f);
    }
}
