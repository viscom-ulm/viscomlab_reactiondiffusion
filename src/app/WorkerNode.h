/**
 * @file   WorkerNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the worker node.
 */

#pragma once

#include "app/ApplicationNodeImplementation.h"

namespace viscom {

    class WorkerNode final : public ApplicationNodeImplementation
    {
    public:
        explicit WorkerNode(ApplicationNodeInternal* appNode);
        virtual ~WorkerNode() override;

        virtual void UpdateSyncedInfo() override;

#ifdef VISCOM_USE_SGCT
        virtual void EncodeData() override;
        virtual void DecodeData() override;

    private:
        /** Holds the data shared by the master. */
        sgct::SharedObject<SimulationData> sharedData_;
        sgct::SharedVector<SeedPoint> sharedSeedPoints_;
#endif
    };
}
