/**
 * @file   MasterNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the master node.
 */

#pragma once

#include "../app/ApplicationNodeImplementation.h"
#ifdef WITH_TUIO
#include "core/TuioInputWrapper.h"
#endif

class TuioInputWrapper;

namespace viscom {

    class MasterNode final : public ApplicationNodeImplementation
    {
    public:
        explicit MasterNode(ApplicationNodeInternal* appNode);
        virtual ~MasterNode() override;

        virtual void PreSync() override;
        virtual void UpdateFrame(double currentTime, double elapsedTime) override;
        virtual void Draw2D(FrameBuffer& fbo) override;
        virtual bool MouseButtonCallback(int button, int action) override;
        virtual bool MousePosCallback(double x, double y) override;

#ifdef WITH_TUIO
        virtual bool AddTuioCursor(TUIO::TuioCursor *tcur) override;
        virtual bool UpdateTuioCursor(TUIO::TuioCursor *tcur) override;
        virtual bool RemoveTuioCursor(TUIO::TuioCursor *tcur) override;
#endif

        virtual void EncodeData() override;
        virtual void DecodeData() override;

    private:
        /** Holds the data the master shares. */
        sgct::SharedObject<SimulationData> sharedData_;
        sgct::SharedVector<SeedPoint> sharedSeedPoints_;
        sgct::SharedUInt64 syncedTimestamp_;

        /** store mouse button state */
        int currentMouseAction_ = -1;
        int currentMouseButton_ = -1;
        /** store mouse position for seed point generation */
        glm::vec2 currentMouseCursorPosition_ = glm::vec2{0.0f};
        /** Store tuio cursor positions. */
        std::vector<std::pair<int, glm::vec2>> tuioCursorPositions_;
    };
}
