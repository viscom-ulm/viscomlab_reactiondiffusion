/**
 * @file   ApplicationNodeImplementation.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Declaration of the application node implementation common for coordinator and worker nodes.
 */

#pragma once

#include "core/app/ApplicationNodeBase.h"

namespace viscom::renderers {
    class RDRenderer;
}

namespace viscom {

    class MeshRenderable;
    class FullscreenQuad;

    struct SimulationData {
        /** The distance the simulation will be drawn at. */
        float simulationDrawDistance_ = 15.0f;
        /** The simulation height field height. */
        float simulationHeight_ = 0.1f;
        /** The relative index of refraction used for raycasting. */
        float eta_ = 1.5f;
        /** The absorption coefficient. */
        glm::vec3 sigma_a_ = glm::vec3(2.0f);
        /** The current global iteration count. */
        std::uint64_t currentGlobalIterationCount_ = 0;
        /** frame at which the simulation should be reset */
        size_t resetFrameIdx_ = 0;

        /** reaction diffusion parameters */
        float diffusion_rate_a_ = 1.0f;
        float diffusion_rate_b_ = 0.5f;
        float feed_rate_ = 0.055f;
        float kill_rate_ = 0.062f;
        float dt_ = 1.0f;
        float seed_point_radius_ = 0.1f;
        bool use_manhattan_distance_ = true;

        int currentRenderer_ = 0;
    };

    struct SimulationPlane {
        glm::vec3 position_;
        glm::vec3 right_;
        glm::vec3 up_;
    };

    class ApplicationNodeImplementation : public ApplicationNodeBase
    {
    public:
        explicit ApplicationNodeImplementation(ApplicationNodeInternal* appNode);
        ApplicationNodeImplementation(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation(ApplicationNodeImplementation&&) = delete;
        ApplicationNodeImplementation& operator=(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation& operator=(ApplicationNodeImplementation&&) = delete;
        virtual ~ApplicationNodeImplementation() override;

        virtual void InitOpenGL() override;
        virtual void UpdateFrame(double currentTime, double elapsedTime) override;
        virtual void ClearBuffer(FrameBuffer& fbo) override;
        virtual void DrawFrame(FrameBuffer& fbo) override;
        virtual void CleanUp() override;

        using SeedPoint = std::pair<std::size_t, glm::vec2>;

        std::uint64_t& GetCurrentLocalIterationCount() { return currentLocalIterationCount_; }
        SimulationData& GetSimulationData() { return simData_; }
        std::vector<SeedPoint>& GetSeedPoints() { return seed_points_; }
        const std::vector<std::unique_ptr<renderers::RDRenderer>>& GetRenderers() const { return renderers_; }
        void ResetSimulation() const;

        const glm::vec2& GetSimulationOutputSize() const { return simulationOutputSize_; }

        /** The maximum iteration count per frame. */
        static constexpr std::uint64_t MAX_FRAME_ITERATIONS = 15;
        /** The increase in iteration count per frame. */
        static constexpr std::uint64_t FRAME_ITERATIONS_INC = 5;

        /** The simulation frame buffer size (x). */
        static constexpr unsigned int SIMULATION_SIZE_X = 1920 / 4;
        /** The simulation frame buffer size (y). */
        static constexpr unsigned int SIMULATION_SIZE_Y = 1080 / 4;

    protected:
        const SimulationPlane& GetSimPlane() const { return simPlane_; }

    private:
        /** The current local iteration count. */
        std::uint64_t currentLocalIterationCount_ = 0;
        /** Holds the simulation data. */
        SimulationData simData_;

        /** Toggle switch for iteration step */
        bool iterationToggle_ = true;
        /** stores seed points */
        std::vector<SeedPoint> seed_points_;

        /** Uniform Location for texture sampler of previous iteration step */
        GLint rdPrevIterationTextureLoc_ = -1;
        GLint rdDiffusionRateALoc_ = -1;
        GLint rdDiffusionRateBLoc_ = -1;
        GLint rdFeedRateLoc_ = -1;
        GLint rdKillRateLoc_ = -1;
        GLint rdDtLoc_ = -1;
        GLint rdSeedPointRadiusLoc_ = -1;
        GLint rdNumSeedPointsLoc_ = -1;
        GLint rdSeedPointsLoc_ = -1;
        GLint rdUseManhattanDistanceLoc_ = -1;

        /** Program to compute reaction diffusion step */
        std::unique_ptr<FullscreenQuad> reactionDiffusionFullScreenQuad_;
        /** The frame buffer object for the simulation. */
        std::unique_ptr<FrameBuffer> reactDiffuseFBO_;

        std::vector<std::unique_ptr<renderers::RDRenderer>> renderers_;

        /** Holds the simulation plane. */
        SimulationPlane simPlane_;
        /** Output size of the simulation. */
        glm::vec2 simulationOutputSize_;
    };
}
