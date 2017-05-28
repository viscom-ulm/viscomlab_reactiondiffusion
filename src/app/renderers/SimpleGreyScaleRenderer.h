/**
 * @file   SimpleGreyScaleRenderer.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.com>
 * @date   2017.05.28
 *
 * @brief  Declaration of the simple greyscale renderer.
 */

#pragma once

#include "core/main.h"
#include "core/gfx/FrameBuffer.h"
#include "RDRenderer.h"

namespace viscom {
    class ApplicationNodeImplementation;
    class GPUProgram;
    struct SimulationData;
}

namespace viscom::renderers {

    class SimpleGreyScaleRenderer : public RDRenderer
    {
    public:
        SimpleGreyScaleRenderer(ApplicationNodeImplementation* appNode);
        virtual ~SimpleGreyScaleRenderer() override;

        virtual void ClearBuffers(FrameBuffer& fbo) override;
        virtual void UpdateFrame(double currentTime, double elapsedTime, const SimulationData& simData, const glm::vec2& nearPlaneSize) override;
        virtual void RenderRDResults(FrameBuffer& fbo, const SimulationData& simData, const glm::mat4& perspectiveMatrix, GLuint rdTexture) override;
        virtual void DrawOptionsGUI(SimulationData& simData) const override;

    private:
        /** Output size of the simulation. */
        glm::vec2 simulationOutputSize_;

        /** Holds the shader program for raycasting the height field back side. */
        std::shared_ptr<GPUProgram> drawGSProgram_;
        /** Holds the location of the VP matrix. */
        GLint drawGSVPLoc_ = -1;
        /** Holds the location of the simulation quad size. */
        GLint drawGSQuadSizeLoc_ = -1;
        /** Holds the location of the simulation quad distance. */
        GLint drawGSDistanceLoc_ = -1;
        /** Holds the location of the height texture. */
        GLint drawGSHeightTextureLoc_ = -1;

        /** Holds the dummy VAO for the simulation quad. */
        GLuint simDummyVAO_ = 0;
    };

}
