/**
 * @file   ApplicationNodeImplementation.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Implementation of the application node class.
 */

#include "core/open_gl.h"
#include "ApplicationNodeImplementation.h"
#include "Vertices.h"
#include <imgui.h>
#include "core/gfx/mesh/MeshRenderable.h"
#include "core/gfx/FullscreenQuad.h"
#include <iostream>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "app/renderers/HeightfieldRaycaster.h"
#include "app/renderers/SimpleGreyScaleRenderer.h"


#include <iostream>

namespace viscom {

    ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNodeInternal* appNode) :
        ApplicationNodeBase{ appNode }
    {
        InitialiseVR();
        if (!InitialiseDisplayVR()) {
            CalibrateVR(ovr::CalibrateMethod::CALIBRATE_BY_POINTING);
        }


    void ApplicationNodeImplementation::InitOpenGL()
    {
        FrameBufferDescriptor reactDiffuseFBDesc;
        reactDiffuseFBDesc.texDesc_.emplace_back(GL_RG32F, GL_TEXTURE_2D);
        reactDiffuseFBDesc.texDesc_.emplace_back(GL_RG32F, GL_TEXTURE_2D);
        reactDiffuseFBDesc.texDesc_.emplace_back(GL_R32F, GL_TEXTURE_2D);
        reactDiffuseFBO_ = std::make_unique<FrameBuffer>(SIMULATION_SIZE_X, SIMULATION_SIZE_Y, reactDiffuseFBDesc);

        renderers_.push_back(std::make_unique<renderers::HeightfieldRaycaster>(this));
        renderers_.push_back(std::make_unique<renderers::SimpleGreyScaleRenderer>(this));

        reactionDiffusionFullScreenQuad_ = CreateFullscreenQuad("reactionDiffusionSimulation.frag");
        const auto rdGpuProgram = reactionDiffusionFullScreenQuad_->GetGPUProgram();
        rdPrevIterationTextureLoc_ = rdGpuProgram->getUniformLocation("texture_0");
        rdDiffusionRateALoc_ = rdGpuProgram->getUniformLocation("diffusion_rate_A");
        rdDiffusionRateBLoc_ = rdGpuProgram->getUniformLocation("diffusion_rate_B");
        rdFeedRateLoc_ = rdGpuProgram->getUniformLocation("feed_rate");
        rdKillRateLoc_ = rdGpuProgram->getUniformLocation("kill_rate");
        rdDtLoc_ = rdGpuProgram->getUniformLocation("dt");
        rdSeedPointRadiusLoc_ = rdGpuProgram->getUniformLocation("seed_point_radius");
        rdNumSeedPointsLoc_ = rdGpuProgram->getUniformLocation("num_seed_points");
        rdSeedPointsLoc_ = rdGpuProgram->getUniformLocation("seed_points");
        rdUseManhattanDistanceLoc_ = rdGpuProgram->getUniformLocation("use_manhattan_distance");

        seed_points_.clear();
        ResetSimulation();
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double elapsedTime)
    {
        static const std::vector<std::size_t> drawBuffers0{{0, 2}};
        static const std::vector<std::size_t> drawBuffers1{{1, 2}};

        if (currentLocalIterationCount_ < simData_.currentGlobalIterationCount_) {
            const auto iterations = glm::min(simData_.currentGlobalIterationCount_ - currentLocalIterationCount_, MAX_FRAME_ITERATIONS);

            for (std::uint64_t i = 0; i < iterations; ++i) {
                if (currentLocalIterationCount_ + i == simData_.resetFrameIdx_) {
                    ResetSimulation();
                }

                const std::vector<std::size_t>* currentDrawBuffers{nullptr};
                glActiveTexture(GL_TEXTURE0);
                if (iterationToggle_) {
                    currentDrawBuffers = &drawBuffers0;
                    glBindTexture(GL_TEXTURE_2D, reactDiffuseFBO_->GetTextures()[1]);
                } else {
                    currentDrawBuffers = &drawBuffers1;
                    glBindTexture(GL_TEXTURE_2D, reactDiffuseFBO_->GetTextures()[0]);
                }
                iterationToggle_ = !iterationToggle_;

                std::vector<glm::vec2> actual_seed_points;
                for (const auto& seed_point : seed_points_) {
                    if (currentLocalIterationCount_ + i == seed_point.first) actual_seed_points.push_back(seed_point.second);
                }

                const auto rdGpuProgram = reactionDiffusionFullScreenQuad_->GetGPUProgram();
                glUseProgram(rdGpuProgram->getProgramId());
                glUniform1i(rdPrevIterationTextureLoc_, 0);
                glUniform1f(rdDiffusionRateALoc_, simData_.diffusion_rate_a_);
                glUniform1f(rdDiffusionRateBLoc_, simData_.diffusion_rate_b_);
                glUniform1f(rdFeedRateLoc_, simData_.feed_rate_);
                glUniform1f(rdKillRateLoc_, simData_.kill_rate_);
                glUniform1f(rdDtLoc_, simData_.dt_);
                glUniform1f(rdSeedPointRadiusLoc_, simData_.seed_point_radius_);
                glUniform1ui(rdNumSeedPointsLoc_, static_cast<GLuint>(actual_seed_points.size()));
                glUniform2fv(rdSeedPointsLoc_, static_cast<GLsizei>(actual_seed_points.size()), reinterpret_cast<const GLfloat*>(actual_seed_points.data()));
                glUniform1i(rdUseManhattanDistanceLoc_, simData_.use_manhattan_distance_);

                // simulate
                reactDiffuseFBO_->DrawToFBO(*currentDrawBuffers, [this]() {
                    reactionDiffusionFullScreenQuad_->Draw();
                });
            }
            currentLocalIterationCount_ += iterations;
        }

        float userDistance = (GetCamera()->GetPosition() + GetCamera()->GetUserPosition()).z;
        // TODO: maybe calculate the correct center? (ray through userPosition, (0,0,0) -> hits z=simulationDrawDistance_) [5/27/2017 Sebastian Maisch]
        simulationOutputSize_ = GetConfig().nearPlaneSize_ * (userDistance + simData_.simulationDrawDistance_) / userDistance;

        simPlane_.position_ = glm::vec3(0.0f, 0.0f, -simData_.simulationDrawDistance_);
        simPlane_.right_ = glm::vec3(simulationOutputSize_.x, 0.0f, -simData_.simulationDrawDistance_);
        simPlane_.up_ = glm::vec3(0.0f, simulationOutputSize_.y, -simData_.simulationDrawDistance_);

        renderers_[simData_.currentRenderer_]->UpdateFrame(currentTime, elapsedTime, simData_, GetConfig().nearPlaneSize_);
    }

    void ApplicationNodeImplementation::ResetSimulation() const
    {
        // clear A and B, {0, 1}
        reactDiffuseFBO_->DrawToFBO(std::vector<std::size_t>{0, 1}, []() {
            glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        });

        // clear mixed result, {2}
        reactDiffuseFBO_->DrawToFBO(std::vector<std::size_t>{2}, []() {
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        });
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        renderers_[simData_.currentRenderer_]->ClearBuffers(fbo);
    }

    void ApplicationNodeImplementation::DrawFrame(FrameBuffer& fbo)
    {
        auto perspectiveMatrix = GetCamera()->GetViewPerspectiveMatrix();
        renderers_[simData_.currentRenderer_]->RenderRDResults(fbo, simData_, perspectiveMatrix, reactDiffuseFBO_->GetTextures()[2]);
    }

    void ApplicationNodeImplementation::CleanUp()
    {
        renderers_.clear();
    }
}
