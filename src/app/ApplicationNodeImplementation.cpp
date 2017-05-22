/**
 * @file   ApplicationNodeImplementation.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Implementation of the application node class.
 */

#include "ApplicationNodeImplementation.h"
#include "Vertices.h"
#include <imgui.h>
#include "core/gfx/mesh/MeshRenderable.h"
#include "core/imgui/imgui_impl_glfw_gl3.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>

#undef max

namespace viscom {

    ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNodeInternal* appNode) :
        ApplicationNodeBase{ appNode }
    {
    }

    ApplicationNodeImplementation::~ApplicationNodeImplementation() = default;

    void ApplicationNodeImplementation::InitOpenGL()
    {
        FrameBufferDescriptor reactDiffuseFBDesc;
        reactDiffuseFBDesc.texDesc_.emplace_back(GL_RG32F, GL_TEXTURE_2D);
        reactDiffuseFBDesc.texDesc_.emplace_back(GL_RG32F, GL_TEXTURE_2D);
        reactDiffuseFBDesc.texDesc_.emplace_back(GL_R32F, GL_TEXTURE_2D);
        reactDiffuseFBO_ = std::make_unique<FrameBuffer>(SIMULATION_SIZE_X, SIMULATION_SIZE_Y, reactDiffuseFBDesc);


        FrameBufferDescriptor simulationBackFBDesc;
        simulationBackFBDesc.texDesc_.emplace_back(GL_RG32F, GL_TEXTURE_2D);
        simulationBackFBDesc.rbDesc_.emplace_back(GL_DEPTH_COMPONENT32);

        auto numWindows = sgct_core::ClusterManager::instance()->getThisNodePtr()->getNumberOfWindows();
        for (auto i = 0U; i < numWindows; ++i) {
            auto fboSize = GetViewportQuadSize(i);
            simulationBackFBOs_.emplace_back(fboSize.x, fboSize.y, simulationBackFBDesc);
        }


        raycastBackProgram_ = GetGPUProgramManager().GetResource("raycastHeightfieldBack", std::initializer_list<std::string>{ "raycastHeightfield.vert", "raycastHeightfieldBack.frag" });
        raycastBackVPLoc_ = raycastBackProgram_->getUniformLocation("viewProjectionMatrix");
        raycastBackQuadSizeLoc_ = raycastBackProgram_->getUniformLocation("quadSize");
        raycastBackDistanceLoc_ = raycastBackProgram_->getUniformLocation("distance");
        raycastProgram_ = GetGPUProgramManager().GetResource("raycastHeightfield", std::initializer_list<std::string>{ "raycastHeightfield.vert", "raycastHeightfield.frag" });
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
        backgroundTexture_ = GetTextureManager().GetResource("models/teapot/default.png");
        environmentMap_ = GetTextureManager().GetResource("textures/grace_probe.hdr");
    }


    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double)
    {
        static const std::vector<unsigned int> drawBuffers0{{0, 2}};
        static const std::vector<unsigned int> drawBuffers1{{1, 2}};

        if (currentLocalIterationCount_ < simData_.currentGlobalIterationCount_) {
            auto iterations = glm::max(simData_.currentGlobalIterationCount_ - currentLocalIterationCount_, MAX_FRAME_ITERATIONS);
            for (std::uint64_t i = 0; i < iterations; ++i) {
                const std::vector<unsigned int>* currentDrawBuffers{nullptr};
                glActiveTexture(GL_TEXTURE0);
                if (iteration_toggle_) {
                    currentDrawBuffers = &drawBuffers0;
                    glBindTexture(GL_TEXTURE0, reactDiffuseFBO_->GetTextures()[1]);
                } else {
                    currentDrawBuffers = &drawBuffers1;
                    glBindTexture(GL_TEXTURE0, reactDiffuseFBO_->GetTextures()[0]);
                }
                iteration_toggle_ = !iteration_toggle_;
                reactDiffuseFBO_->DrawToFBO(*currentDrawBuffers, []() {

                });
            }
            currentLocalIterationCount_ += iterations;
        }

        auto perspectiveMatrix = GetCamera()->GetCentralPerspectiveMatrix();
        simulationOutputSize_ = glm::vec2(simData_.simulationDrawDistance_) / glm::vec2(perspectiveMatrix[0][0], perspectiveMatrix[1][1]);
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        SelectOffscreenBuffer(simulationBackFBOs_)->DrawToFBO([]() {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        });

        fbo.DrawToFBO([]() {
            auto colorPtr = sgct::Engine::instance()->getClearColor();
            glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        });
    }

    void ApplicationNodeImplementation::DrawFrame(FrameBuffer& fbo)
    {
        auto perspectiveMatrix = GetCamera()->GetViewPerspectiveMatrix();

        SelectOffscreenBuffer(simulationBackFBOs_)->DrawToFBO([this, &perspectiveMatrix]() {
            glBindVertexArray(simDummyVAO_);
            glUseProgram(raycastBackProgram_->getProgramId());
            glUniformMatrix4fv(raycastBackVPLoc_, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
            glUniform2fv(raycastBackQuadSizeLoc_, 1, glm::value_ptr(simulationOutputSize_));
            glUniform1f(raycastBackDistanceLoc_, simData_.simulationDrawDistance_);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        });

        fbo.DrawToFBO([this, &perspectiveMatrix]() {
            {
                glm::vec3 camPos(0.0f);
                glBindVertexArray(simDummyVAO_);
                glUseProgram(raycastProgram_->getProgramId());
                glUniformMatrix4fv(raycastVPLoc_, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
                glUniform2fv(raycastQuadSizeLoc_, 1, glm::value_ptr(simulationOutputSize_));
                glUniform1f(raycastDistanceLoc_, simData_.simulationDrawDistance_ - simData_.simulationHeight_);
                glUniform1f(raycastSimHeightLoc_, simData_.simulationHeight_);
                glUniform3fv(raycastCamPosLoc_, 1, glm::value_ptr(camPos));
                glUniform1f(raycastEtaLoc_, simData_.eta_);
                glUniform1f(raycastSigmaALoc_, simData_.sigma_a_);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, environmentMap_->getTextureId());
                glUniform1i(raycastEnvMapLoc_, 0);

                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_2D, backgroundTexture_->getTextureId());
                glUniform1i(raycastBGTexLoc_, 1);

                glActiveTexture(GL_TEXTURE0 + 2);
                glBindTexture(GL_TEXTURE_2D, backgroundTexture_->getTextureId());
                glUniform1i(raycastHeightTextureLoc_, 2);

                glBindImageTexture(0, SelectOffscreenBuffer(simulationBackFBOs_)->GetTextures()[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
                glUniform1i(raycastPositionBackTexLoc_, 0);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }


            /*glBindVertexArray(vaoBackgroundGrid_);
            glBindBuffer(GL_ARRAY_BUFFER, vboBackgroundGrid_);

            auto MVP = GetCamera()->GetViewPerspectiveMatrix();
            {
                glUseProgram(backgroundProgram_->getProgramId());
                glUniformMatrix4fv(backgroundMVPLoc_, 1, GL_FALSE, glm::value_ptr(MVP));
                glDrawArrays(GL_TRIANGLES, 0, numBackgroundVertices_);
            }

            {
                glDisable(GL_CULL_FACE);
                auto triangleMVP = MVP * triangleModelMatrix_;
                glUseProgram(triangleProgram_->getProgramId());
                glUniformMatrix4fv(triangleMVPLoc_, 1, GL_FALSE, glm::value_ptr(triangleMVP));
                glDrawArrays(GL_TRIANGLES, numBackgroundVertices_, 3);
                glEnable(GL_CULL_FACE);
            }

            {
                glUseProgram(teapotProgram_->getProgramId());
                auto normalMatrix = glm::inverseTranspose(glm::mat3(teapotModelMatrix_));
                glUniformMatrix4fv(teapotModelMLoc_, 1, GL_FALSE, glm::value_ptr(teapotModelMatrix_));
                glUniformMatrix4fv(teapotNormalMLoc_, 1, GL_FALSE, glm::value_ptr(normalMatrix));
                glUniformMatrix4fv(teapotVPLoc_, 1, GL_FALSE, glm::value_ptr(MVP));
                teapotRenderable_->Draw(teapotModelMatrix_);
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glUseProgram(0);*/
        });
    }

    void ApplicationNodeImplementation::CleanUp()
    {
        if (simDummyVAO_ != 0) glDeleteVertexArrays(1, &simDummyVAO_);
        simDummyVAO_ = 0;
    }

}
