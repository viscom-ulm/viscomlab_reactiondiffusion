/**
 * @file   MasterNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the master node.
 */

#pragma once

#include "../app/ApplicationNodeImplementation.h"

namespace viscom {

    class MasterNode final : public ApplicationNodeImplementation
    {
    public:
        explicit MasterNode(ApplicationNodeInternal* appNode);
        virtual ~MasterNode() override;

        void InitOpenGL() override;
        void PreSync() override;
        virtual void UpdateFrame(double currentTime, double elapsedTime) override;
        void DrawFrame(FrameBuffer& fbo) override;
        void Draw2D(FrameBuffer& fbo) override;
        void CleanUp() override;

        bool KeyboardCallback(int key, int scancode, int action, int mods) override;
        bool CharCallback(unsigned int character, int mods) override;
        bool MouseButtonCallback(int button, int action) override;
        bool MousePosCallback(double x, double y) override;
        bool MouseScrollCallback(double xoffset, double yoffset) override;

        virtual void EncodeData() override;
        virtual void DecodeData() override;

    private:
        /** Holds the data the master shares. */
        sgct::SharedObject<SimulationData> sharedData_;
        sgct::SharedVector<glm::vec2> sharedSeedPoints_;
        sgct::SharedUInt64 syncedTimestamp_;

        /** store mouse button state */
        int currentMouseAction_ = -1;
        int currentMouseButton_ = -1;
        /** store mouse position for seed point generation */
        glm::vec2 currentCursorPosition_ = glm::vec2{0.0f};
    };
}
