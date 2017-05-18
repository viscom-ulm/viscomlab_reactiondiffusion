/**
 * @file   ApplicationNodeImplementation.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Declaration of the application node implementation common for master and slave nodes.
 */

#pragma once

#include <sgct/Engine.h>
#include "core/ApplicationNode.h"

namespace viscom {

    class MeshRenderable;

    class ApplicationNodeImplementation
    {
    public:
        explicit ApplicationNodeImplementation(ApplicationNode* appNode);
        ApplicationNodeImplementation(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation(ApplicationNodeImplementation&&) = delete;
        ApplicationNodeImplementation& operator=(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation& operator=(ApplicationNodeImplementation&&) = delete;
        virtual ~ApplicationNodeImplementation();

        virtual void PreWindow();
        virtual void InitOpenGL();
        virtual void PreSync();
        virtual void UpdateSyncedInfo();
        virtual void UpdateFrame(double currentTime, double elapsedTime);
        virtual void ClearBuffer(FrameBuffer& fbo);
        virtual void DrawFrame(FrameBuffer& fbo);
        virtual void Draw2D(FrameBuffer& fbo);
        virtual void PostDraw();
        virtual void CleanUp();

        virtual void KeyboardCallback(int key, int scancode, int action, int mods);
        virtual void CharCallback(unsigned int character, int mods);
        virtual void MouseButtonCallback(int button, int action);
        virtual void MousePosCallback(double x, double y);
        virtual void MouseScrollCallback(double xoffset, double yoffset);

        virtual void EncodeData();
        virtual void DecodeData();

    protected:
        sgct::Engine* GetEngine() const { return appNode_->GetEngine(); }
        const FWConfiguration& GetConfig() const { return appNode_->GetConfig(); }
        ApplicationNode* GetApplication() const { return appNode_; }

        unsigned int GetGlobalProjectorId(int nodeId, int windowId) const { return appNode_->GetGlobalProjectorId(nodeId, windowId); }

        const Viewport& GetViewportScreen(size_t windowId) const { return appNode_->GetViewportScreen(windowId); }
        Viewport& GetViewportScreen(size_t windowId) { return appNode_->GetViewportScreen(windowId); }
        const glm::ivec2& GetViewportQuadSize(size_t windowId) const { return appNode_->GetViewportQuadSize(windowId); }
        glm::ivec2& GetViewportQuadSize(size_t windowId) { return appNode_->GetViewportQuadSize(windowId); }
        const glm::vec2& GetViewportScaling(size_t windowId) const { return appNode_->GetViewportScaling(windowId); }
        glm::vec2& GetViewportScaling(size_t windowId) { return appNode_->GetViewportScaling(windowId); }

        double GetCurrentAppTime() const { return appNode_->GetCurrentAppTime(); }
        double GetElapsedTime() const { return appNode_->GetElapsedTime(); }

        std::uint64_t& GetCurrentLocalIterationCount() { return currentLocalIterationCount_; }
        std::uint64_t& GetGlobalIterationCount() { return currentGlobalIterationCount_; }
        sgct::SharedUInt64& GetGlobalIterationCountShared() { return currentGlobalIterationCountShared_; }

        /** The maximum iteration count per frame. */
        static constexpr std::uint64_t MAX_FRAME_ITERATIONS = 10;
        /** The simulation frame buffer size (x). */
        static constexpr unsigned int SIMULATION_SIZE_X = 1920;
        /** The simulation frame buffer size (y). */
        static constexpr unsigned int SIMULATION_SIZE_Y = 1080;
        /** The distance the simulation will be drawn at. */
        static constexpr float SIMULATION_DRAW_DISTANCE = 10.0f;
        /** The simulation height field height. */
        static constexpr float SIMULATION_HEIGHT = 1.0f;

    private:
        /** Holds the application node. */
        ApplicationNode* appNode_;

        /** The current local iteration count. */
        std::uint64_t currentLocalIterationCount_;
        /** The current global iteration count. */
        std::uint64_t currentGlobalIterationCount_;
        /** The current global iteration count (shared). */
        sgct::SharedUInt64 currentGlobalIterationCountShared_;
        /** Toggle switch for iteration step */
        bool iteration_toggle_;

        /** The frame buffer object for the simulation. */
        std::unique_ptr<FrameBuffer> reactDiffuseFBO_;
        /** The frame buffer objects for the simulation height field back. */
        std::vector<FrameBuffer> simulationBackFBOs_;

        /** Holds the shader program for drawing the background. */
        std::shared_ptr<GPUProgram> backgroundProgram_;
        /** Holds the location of the MVP matrix. */
        GLint backgroundMVPLoc_ = -1;

        /** Holds the shader program for drawing the foreground triangle. */
        std::shared_ptr<GPUProgram> triangleProgram_;
        /** Holds the location of the MVP matrix. */
        GLint triangleMVPLoc_ = -1;

        /** Holds the shader program for drawing the foreground teapot. */
        std::shared_ptr<GPUProgram> teapotProgram_;
        /** Holds the location of the VP matrix. */
        GLint teapotVPLoc_ = -1;

        /** Holds the shader program for raycasting the height field back side. */
        std::shared_ptr<GPUProgram> raycastBackProgram_;
        /** Holds the location of the VP matrix. */
        GLint raycastBackVPLoc_ = -1;
        /** Holds the location of the simulation quad size. */
        GLint raycastBackQuadSizeLoc_ = -1;

        /** Holds the shader program for raycasting the height field. */
        std::shared_ptr<GPUProgram> raycastProgram_;
        /** Holds the location of the VP matrix. */
        GLint raycastVPLoc_ = -1;
        /** Holds the location of the simulation quad size. */
        GLint raycastQuadSizeLoc_ = -1;
        /** Holds the location of the simulation height. */
        GLint raycastSimHeightLoc_ = -1;
        /** Holds the location of the environment map. */
        GLint raycastEnvMapLoc_ = -1;
        /** Holds the location of the background texture. */
        GLint raycastBGTexLoc_ = -1;
        /** Holds the location of the back position texture. */
        GLint raycastPositionBackTexLoc_ = -1;

        /** Holds the dummy VAO for the simulation quad. */
        GLuint simDummyVAO_ = 0;
        /** Holds the background texture for the simulation. */
        std::shared_ptr<Texture> backgroundTexture_;
        /** Holds the environment map texture. */
        std::shared_ptr<Texture> environmentMap_;

        /** Holds the number of vertices of the background grid. */
        unsigned int numBackgroundVertices_ = 0;
        /** Holds the vertex buffer for the background grid. */
        GLuint vboBackgroundGrid_ = 0;
        /** Holds the vertex array object for the background grid. */
        GLuint vaoBackgroundGrid_ = 0;

        /** Holds the teapot mesh. */
        std::shared_ptr<Mesh> teapotMesh_;
        /** Holds the teapot mesh renderable. */
        std::unique_ptr<MeshRenderable> teapotRenderable_;

        glm::mat4 triangleModelMatrix_;
        glm::mat4 teapotModelMatrix_;
    };
}
