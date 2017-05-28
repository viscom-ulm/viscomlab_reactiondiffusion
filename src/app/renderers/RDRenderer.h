/**
 * @file   HeightfieldRaycaster.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.com>
 * @date   2017.05.28
 *
 * @brief  Declaration of the base reaction diffusion renderer.
 */

#pragma once

#include "core/main.h"
#include "core/gfx/FrameBuffer.h"

namespace viscom {
    class ApplicationNodeImplementation;
    struct SimulationData;
}

namespace viscom::renderers {

    class RDRenderer
    {
    public:
        RDRenderer(const std::string& name, ApplicationNodeImplementation* appNode);
        virtual ~RDRenderer();

        std::string GetName() const { return name_; }
        virtual void ClearBuffers(FrameBuffer& fbo) = 0;
        virtual void UpdateFrame(double currentTime, double elapsedTime, const SimulationData& simData, const glm::vec2& nearPlaneSize) = 0;
        virtual void RenderRDResults(FrameBuffer& fbo, const SimulationData& simData, const glm::mat4& perspectiveMatrix, GLuint rdTexture) = 0;
        virtual void DrawOptionsGUI(SimulationData& simData) const = 0;

    protected:
        /** Holds the application node. */
        ApplicationNodeImplementation* appNode_;

    private:
        /** Holds the implementations name. */
        std::string name_;
    };

}
