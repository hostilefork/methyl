#include "methyl/context.h"
#include "methyl/engine.h"

namespace methyl {

shared_ptr<Context> Context::create() {
    return globalEngine->contextForCreate();
}

shared_ptr<Context> Context::lookup() {
    return globalEngine->contextForLookup();
}

} // end namespace methyl
