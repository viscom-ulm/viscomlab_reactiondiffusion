/**
 * @file   HeightfieldRaycaster.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.com>
 * @date   2017.05.28
 *
 * @brief  Declaration of the heighfield raycaster renderer.
 */

#pragma once

#include "core/main.h"
#include "core/gfx/FrameBuffer.h"
#include "RDRenderer.h"

namespace viscom {
    class ApplicationNodeImplementation;
    class GPUProgram;
    class Texture;
    struct SimulationData;
}

namespace viscom::renderers {

    class HeightfieldRaycaster : public RDRenderer
    {
    public:
        HeightfieldRaycaster(ApplicationNodeImplementation* appNode);
        virtual ~HeightfieldRaycaster() override;

        virtual void ClearBuffers(FrameBuffer& fbo) override;
        virtual void UpdateFrame(double currentTime, double elapsedTime, const SimulationData& simData, const glm::vec2& nearPlaneSize) override;
        virtual void RenderRDResults(FrameBuffer& fbo, const SimulationData& simData, const glm::mat4& perspectiveMatrix, GLuint rdTexture) override;
        virtual void DrawOptionsGUI(SimulationData& simData) const override;

    private:
        /** The frame buffer objects for the simulation height field back. */
        std::vector<FrameBuffer> simulationBackFBOs_;

        /** Holds the shader program for raycasting the height field back side. */
        std::shared_ptr<GPUProgram> raycastBackProgram_;
        /** Holds the location of the VP matrix. */
        GLint raycastBackVPLoc_ = -1;
        /** Holds the location of the simulation quad size. */
        GLint raycastBackQuadSizeLoc_ = -1;
        /** Holds the location of the simulation quad distance. */
        GLint raycastBackDistanceLoc_ = -1;

        /** Holds the shader program for raycasting the height field. */
        std::shared_ptr<GPUProgram> raycastProgram_;
        /** Holds the location of the VP matrix. */
        GLint raycastVPLoc_ = -1;
        /** Holds the location of the simulation quad size. */
        GLint raycastQuadSizeLoc_ = -1;
        /** Holds the location of the simulation quad distance. */
        GLint raycastDistanceLoc_ = -1;
        /** Holds the location of the simulation height. */
        GLint raycastSimHeightLoc_ = -1;
        /** Holds the location of the camera position. */
        GLint raycastCamPosLoc_ = -1;
        /** Holds the location of index of refraction. */
        GLint raycastEtaLoc_ = -1;
        /** Holds the location of the absorption coefficient. */
        GLint raycastSigmaALoc_ = -1;
        /** Holds the location of the environment map. */
        GLint raycastEnvMapLoc_ = -1;
        /** Holds the location of the background texture. */
        GLint raycastBGTexLoc_ = -1;
        /** Holds the location of the height texture. */
        GLint raycastHeightTextureLoc_ = -1;
        /** Holds the location of the back position texture. */
        GLint raycastPositionBackTexLoc_ = -1;

        /** Holds the dummy VAO for the simulation quad. */
        GLuint simDummyVAO_ = 0;
        /** Holds the background texture for the simulation. */
        std::shared_ptr<Texture> backgroundTexture_;
        /** Holds the environment map texture. */
        std::shared_ptr<Texture> environmentMap_;
    };

}
