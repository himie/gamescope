#pragma once

#include <cstring>
#include <drm_mode.h>

extern "C"
{
#include <libdisplay-info/edid.h>
#include <libdisplay-info/info.h>
}

#include "color_helpers.h"

namespace gamescope
{
    inline bool NeedsEdidContentLightFeedback( const di_edid_vendor_product &product, bool internal )
    {
        // The Ally X20's Samsung panel washes out with unknown content light levels.
        // Zero is valid; this is a sink-specific interoperability quirk.
        return internal && std::memcmp( product.manufacturer, "SDC", 3 ) == 0 && product.product == 0x4501;
    }

    inline void PopulateHdrContentLightLevelsFromEdid( hdr_metadata_infoframe &metadata,
                                                      const di_hdr_static_metadata &hdr )
    {
        // Use only values advertised by the sink. Missing EDID values stay zero;
        // do not feed the connector's synthetic luminance defaults back to it.
        metadata.max_cll = nits_to_u16( hdr.desired_content_max_luminance );
        metadata.max_fall = nits_to_u16( hdr.desired_content_max_frame_avg_luminance );
    }
}
