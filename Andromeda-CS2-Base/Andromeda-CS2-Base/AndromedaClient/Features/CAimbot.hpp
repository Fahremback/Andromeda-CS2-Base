#pragma once

// Compat shim:
// CAimbot was consolidated into CPhysicsAligner.
// Keeping this alias avoids modify/delete merge conflicts on local branches
// that still include the previous header name.
#include "CPhysicsAligner.hpp"

using CAimbot = CPhysicsAligner;
