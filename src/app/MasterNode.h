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

        virtual void InitOpenGL() override;
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

#ifdef VISCOM_USE_SGCT
        virtual void EncodeData() override;
        virtual void DecodeData() override;
#endif

    private:
        math::Line3<float> GetPickRay(const glm::vec2& globalClickCoords);
        glm::vec2 FindIntersectionWithPlane(const math::Line3<float>& ray) const;

#ifdef VISCOM_USE_SGCT
        /** Holds the data the master shares. */
        sgct::SharedObject<SimulationData> sharedData_;
        sgct::SharedVector<SeedPoint> sharedSeedPoints_;
        sgct::SharedUInt64 syncedTimestamp_;
#endif

        /** store mouse button state */
        int currentMouseAction_ = -1;
        int currentMouseButton_ = -1;
        /** store mouse position for seed point generation */
        glm::vec2 currentMouseCursorPosition_ = glm::vec2{0.0f};
        /** Store tuio cursor positions. */
        std::vector<std::pair<int, glm::vec2>> tuioCursorPositions_;

        void LoadPresetList();
        void UpdatePresetNames();
        void LoadPreset(int preset);
        void SavePreset(const std::string& presetName);

        /** The list of preset names. */
        std::vector<std::pair<std::string, std::string>> presetNames_;
        /** The list of preset names (as c strings for imgui). */
        std::vector<const char*> presetNamesCStr_;
        /** The list of renderer names. */
        std::vector<std::string> rendererNames_;
        /** The list of renderer names (as c strings for imgui). */
        std::vector<const char*> rendererNamesCStr_;
    };
}
