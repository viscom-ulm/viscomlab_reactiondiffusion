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

namespace viscom {

    ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNode* appNode) :
        appNode_{ appNode }
    {
    }

    ApplicationNodeImplementation::~ApplicationNodeImplementation() = default;

    void ApplicationNodeImplementation::PreWindow()
    {
    }

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


        backgroundProgram_ = appNode_->GetGPUProgramManager().GetResource("backgroundGrid", std::initializer_list<std::string>{ "backgroundGrid.vert", "backgroundGrid.frag" });
        backgroundMVPLoc_ = backgroundProgram_->getUniformLocation("MVP");

        triangleProgram_ = appNode_->GetGPUProgramManager().GetResource("foregroundTriangle", std::initializer_list<std::string>{ "foregroundTriangle.vert", "foregroundTriangle.frag" });
        triangleMVPLoc_ = triangleProgram_->getUniformLocation("MVP");

        teapotProgram_ = appNode_->GetGPUProgramManager().GetResource("foregroundMesh", std::initializer_list<std::string>{ "foregroundMesh.vert", "foregroundMesh.frag" });
        teapotVPLoc_ = teapotProgram_->getUniformLocation("viewProjectionMatrix");

        raycastBackProgram_ = appNode_->GetGPUProgramManager().GetResource("raycastHeightfieldBack", std::initializer_list<std::string>{ "raycastHeightfield.vert", "raycastHeightfieldBack.frag" });
        raycastBackVPLoc_ = raycastBackProgram_->getUniformLocation("viewProjectionMatrix");
        raycastBackQuadSizeLoc_ = raycastBackProgram_->getUniformLocation("quadSize");
        raycastBackDistanceLoc_ = raycastBackProgram_->getUniformLocation("distance");
        raycastProgram_ = appNode_->GetGPUProgramManager().GetResource("raycastHeightfield", std::initializer_list<std::string>{ "raycastHeightfield.vert", "raycastHeightfield.frag" });
        raycastVPLoc_ = raycastProgram_->getUniformLocation("viewProjectionMatrix");
        raycastQuadSizeLoc_ = raycastProgram_->getUniformLocation("quadSize");
        raycastDistanceLoc_ = raycastProgram_->getUniformLocation("distance");
        raycastSimHeightLoc_ = raycastProgram_->getUniformLocation("simulationHeight");
        raycastEnvMapLoc_ = raycastProgram_->getUniformLocation("environment");
        raycastBGTexLoc_ = raycastProgram_->getUniformLocation("backgroundTexture");
        raycastPositionBackTexLoc_ = raycastProgram_->getUniformLocation("backPositionTexture");

        glGenVertexArrays(1, &simDummyVAO_);
        backgroundTexture_ = appNode_->GetTextureManager().GetResource("models/teapot/default.png");
        environmentMap_ = appNode_->GetTextureManager().GetResource("textures/grace.hdr");

        std::vector<GridVertex> gridVertices;

        auto delta = 0.125f;
        for (auto x = -1.0f; x < 1.0f; x += delta) {
            auto green = (x + 1.0f) / 2.0f;

            for (float y = -1.0; y < 1.0; y += delta) {
                auto red = (y + 1.0f) / 2.0f;

                auto dx = 0.004f;
                auto dy = 0.004f;

                gridVertices.emplace_back(glm::vec3(x + dx, y + dy, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//right top
                gridVertices.emplace_back(glm::vec3(x - dx + delta, y + dy, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//left top
                gridVertices.emplace_back(glm::vec3(x - dx + delta, y - dy + delta, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//left bottom

                gridVertices.emplace_back(glm::vec3(x - dx + delta, y - dy + delta, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//left bottom
                gridVertices.emplace_back(glm::vec3(x + dx, y - dy + delta, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//right bottom
                gridVertices.emplace_back(glm::vec3(x + dx, y + dy, -1.0f), glm::vec4(red, green, 0.0f, 1.0f));//right top
            }
        }

        numBackgroundVertices_ = static_cast<unsigned int>(gridVertices.size());

        gridVertices.emplace_back(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        gridVertices.emplace_back(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
        gridVertices.emplace_back(glm::vec3(0.5f, -0.5f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

        glGenBuffers(1, &vboBackgroundGrid_);
        glBindBuffer(GL_ARRAY_BUFFER, vboBackgroundGrid_);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(GridVertex), gridVertices.data(), GL_STATIC_DRAW);

        glGenVertexArrays(1, &vaoBackgroundGrid_);
        glBindVertexArray(vaoBackgroundGrid_);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridVertex), reinterpret_cast<GLvoid*>(offsetof(GridVertex, position_)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(GridVertex), reinterpret_cast<GLvoid*>(offsetof(GridVertex, color_)));
        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        teapotMesh_ = appNode_->GetMeshManager().GetResource("/models/teapot/teapot.obj");
        teapotRenderable_ = MeshRenderable::create<SimpleMeshVertex>(teapotMesh_.get(), teapotProgram_.get());
    }

    void ApplicationNodeImplementation::PreSync()
    {
    }

    void ApplicationNodeImplementation::UpdateSyncedInfo()
    {
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double)
    {
        static const std::vector<unsigned int> drawBuffers0{{0, 2}};
        static const std::vector<unsigned int> drawBuffers1{{1, 2}};

        if (currentLocalIterationCount_ < currentGlobalIterationCount_) {
            auto iterations = glm::max(currentGlobalIterationCount_ - currentLocalIterationCount_, MAX_FRAME_ITERATIONS);
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
        triangleModelMatrix_ = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)), static_cast<float>(currentTime), glm::vec3(0.0f, 1.0f, 0.0f));
        teapotModelMatrix_ = glm::scale(glm::rotate(glm::translate(glm::mat4(0.01f), glm::vec3(-3.0f, 0.0f, -5.0f)), static_cast<float>(currentTime), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.01f));
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        auto windowId = GetEngine()->getCurrentWindowPtr()->getId();
        simulationBackFBOs_[windowId].DrawToFBO([]() {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClearDepth(1.0f);
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
        auto perspectiveMatrix = GetEngine()->getCurrentProjectionMatrix();
        glm::vec2 simulationSize(SIMULATION_DRAW_DISTANCE);
        simulationSize /= glm::vec2(perspectiveMatrix[0][0], perspectiveMatrix[1][1]);

        auto windowId = GetEngine()->getCurrentWindowPtr()->getId();
        simulationBackFBOs_[windowId].DrawToFBO([this, &perspectiveMatrix, &simulationSize]() {
            glBindVertexArray(simDummyVAO_);
            glUseProgram(raycastBackProgram_->getProgramId());
            glUniformMatrix4fv(raycastBackVPLoc_, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
            glUniform2fv(raycastBackQuadSizeLoc_, 1, glm::value_ptr(simulationSize));
            glUniform1f(raycastBackDistanceLoc_, SIMULATION_DRAW_DISTANCE);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        });

        fbo.DrawToFBO([this, &perspectiveMatrix, &simulationSize, windowId]() {
            {
                glBindVertexArray(simDummyVAO_);
                glUseProgram(raycastProgram_->getProgramId());
                glUniformMatrix4fv(raycastVPLoc_, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
                glUniform2fv(raycastQuadSizeLoc_, 1, glm::value_ptr(simulationSize));
                glUniform1f(raycastDistanceLoc_, SIMULATION_DRAW_DISTANCE - SIMULATION_HEIGHT);
                glUniform1f(raycastSimHeightLoc_, SIMULATION_HEIGHT);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE0, environmentMap_->getTextureId());
                glUniform1i(raycastEnvMapLoc_, 0);

                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE0 + 1, backgroundTexture_->getTextureId());
                glUniform1i(raycastBGTexLoc_, 1);

                glActiveTexture(GL_TEXTURE0 + 2);
                glBindTexture(GL_TEXTURE0 + 2, simulationBackFBOs_[windowId].GetTextures()[0]);
                glUniform1i(raycastPositionBackTexLoc_, 2);

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }


            glBindVertexArray(vaoBackgroundGrid_);
            glBindBuffer(GL_ARRAY_BUFFER, vboBackgroundGrid_);

            auto MVP = GetEngine()->getCurrentModelViewProjectionMatrix();
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
                glUniformMatrix4fv(teapotVPLoc_, 1, GL_FALSE, glm::value_ptr(MVP));
                teapotRenderable_->Draw(teapotModelMatrix_);
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glUseProgram(0);
        });
    }

    void ApplicationNodeImplementation::Draw2D(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([]() {
#ifdef VISCOM_CLIENTGUI
            ImGui::ShowTestWindow();
#endif
        });
    }

    void ApplicationNodeImplementation::PostDraw()
    {
    }

    void ApplicationNodeImplementation::CleanUp()
    {
        if (simDummyVAO_ != 0) glDeleteVertexArrays(1, &simDummyVAO_);
        simDummyVAO_ = 0;

        if (vaoBackgroundGrid_ != 0) glDeleteVertexArrays(1, &vaoBackgroundGrid_);
        vaoBackgroundGrid_ = 0;
        if (vboBackgroundGrid_ != 0) glDeleteBuffers(1, &vboBackgroundGrid_);
        vboBackgroundGrid_ = 0;
    }

    // ReSharper disable CppParameterNeverUsed
    void ApplicationNodeImplementation::KeyboardCallback(int key, int scancode, int action, int mods)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_KeyCallback(key, scancode, action, mods);
#endif
    }

    void ApplicationNodeImplementation::CharCallback(unsigned int character, int mods)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_CharCallback(character);
#endif
    }

    void ApplicationNodeImplementation::MouseButtonCallback(int button, int action)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
#endif
    }

    void ApplicationNodeImplementation::MousePosCallback(double x, double y)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
#endif
    }

    void ApplicationNodeImplementation::MouseScrollCallback(double xoffset, double yoffset)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_ScrollCallback(xoffset, yoffset);
#endif
    }
    // ReSharper restore CppParameterNeverUsed

    void ApplicationNodeImplementation::EncodeData()
    {
    }

    void ApplicationNodeImplementation::DecodeData()
    {
    }
}
