// Swift sees the engine through this C header. Nothing else from the C++ core
// is exposed to the app, so the UI cannot reach into engine internals.
#import "vx/c_api.h"
