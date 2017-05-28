/**
 * @file   RDRenderer.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2017.05.28
 *
 * @brief  Implementation of the base reaction diffusion renderer.
 */

#include "RDRenderer.h"

namespace viscom::renderers {

    RDRenderer::RDRenderer(const std::string& name, ApplicationNodeImplementation* appNode) :
        name_{ name },
        appNode_{ appNode }
    {
    }

    RDRenderer::~RDRenderer() = default;
}
