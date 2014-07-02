#include "methyl/context.h"
#include "methyl/engine.h"

namespace methyl {

shared_ptr<Context> Context::contextForCreate() {
    return globalEngine->contextForCreate();
}

shared_ptr<Context> Context::contextForLookup() {
    return globalEngine->contextForLookup();
}

} // end namespace methyl
