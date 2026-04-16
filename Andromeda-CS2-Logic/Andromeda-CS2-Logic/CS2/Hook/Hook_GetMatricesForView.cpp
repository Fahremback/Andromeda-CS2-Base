#include "Hook_GetMatricesForView.hpp"

#include <CS2/SDK/Math/Math.hpp>

#include <AndromedaClient/Features/CVisual/CVisual.hpp>

auto Hook_GetMatricesForView( void* rcx , void* view ,
                              VMatrix* pWorldToView ,
                              VMatrix* pViewToProjection ,
                              VMatrix* pWorldToProjection ,
                              VMatrix* pWorldToPixels ) -> void
{
    GetMatricesForView_o( rcx , view , pWorldToView , pViewToProjection , pWorldToProjection , pWorldToPixels );

    if ( pWorldToProjection )
        g_ViewMatrix = *pWorldToProjection;

    // FIX: Removed CalculateBoundingBoxes() from this hot path.
    // This hook fires multiple times per frame. Bounding boxes are now
    // computed lazily in OnRender with throttling.
}
