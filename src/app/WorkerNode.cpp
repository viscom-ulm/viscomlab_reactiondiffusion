/**
 * @file   WorkerNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the worker application node.
 */

#include "WorkerNode.h"
#include <imgui.h>
#include "core/open_gl.h"

namespace viscom {

    WorkerNode::WorkerNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode }
    {
    }

    WorkerNode::~WorkerNode() = default;

    void WorkerNode::UpdateSyncedInfo()
    {
        ApplicationNodeImplementation::UpdateSyncedInfo();
#ifdef VISCOM_USE_SGCT
        GetSimulationData() = sharedData_.getVal();
        auto tmpSeedPoints = sharedSeedPoints_.getVal();
        for (const auto& tsp : tmpSeedPoints) GetSeedPoints().push_back(tsp);
#endif

        // iterate GetSeedPoints, delete all seed points before current time
        auto lastDel = GetSeedPoints().begin();
        for (; lastDel != GetSeedPoints().end() && lastDel->first < GetCurrentLocalIterationCount(); ++lastDel);
        if (lastDel != GetSeedPoints().begin()) {
            GetSeedPoints().erase(GetSeedPoints().begin(), lastDel);
        }
    }

#ifdef VISCOM_USE_SGCT
    void WorkerNode::EncodeData()
    {
        ApplicationNodeImplementation::EncodeData();
        sgct::SharedData::instance()->writeObj(&sharedData_);
        sgct::SharedData::instance()->writeVector(&sharedSeedPoints_);
    }

    void WorkerNode::DecodeData()
    {
        ApplicationNodeImplementation::DecodeData();
        sgct::SharedData::instance()->readObj(&sharedData_);
        sgct::SharedData::instance()->readVector(&sharedSeedPoints_);
    }
#endif

}
