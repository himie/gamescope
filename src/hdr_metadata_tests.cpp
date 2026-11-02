#include "hdr_metadata.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main( int argc, char **argv )
{
    assert( argc == 2 );
    std::ifstream file( argv[1], std::ios::binary );
    assert( file );
    std::vector<char> edid( ( std::istreambuf_iterator<char>( file ) ), {} );
    di_info *info = di_info_parse_edid( edid.data(), edid.size() );
    assert( info );
    const di_edid *parsed = di_info_get_edid( info );
    const auto &product = *di_edid_get_vendor_product( parsed );
    assert( gamescope::NeedsEdidContentLightFeedback( product, true ) );
    assert( !gamescope::NeedsEdidContentLightFeedback( product, false ) );
    auto other = product;
    other.product++;
    assert( !gamescope::NeedsEdidContentLightFeedback( other, true ) );
    std::memcpy( other.manufacturer, "VLV", 3 );
    other.product = product.product;
    assert( !gamescope::NeedsEdidContentLightFeedback( other, true ) );

    hdr_metadata_infoframe metadata{};
    metadata.eotf = 2;
    gamescope::PopulateHdrContentLightLevelsFromEdid( metadata, *di_info_get_hdr_static_metadata( info ) );
    assert( metadata.eotf == 2 );
    // The real DisplayID-2 fixture advertises 1405/800 nits. Mastering stays unknown.
    assert( metadata.max_display_mastering_luminance == 0 );
    assert( metadata.max_cll == 1405 );
    assert( metadata.max_fall == 800 );
    assert( metadata.min_display_mastering_luminance == 0 );
    for ( const auto &primary : metadata.display_primaries )
        assert( primary.x == 0 && primary.y == 0 );
    assert( metadata.white_point.x == 0 && metadata.white_point.y == 0 );

    // Optional, absent fields must remain unknown, not become Deck defaults.
    di_hdr_static_metadata unknown_hdr{};
    hdr_metadata_infoframe unknown{};
    gamescope::PopulateHdrContentLightLevelsFromEdid( unknown, unknown_hdr );
    assert( unknown.max_display_mastering_luminance == 0 );
    assert( unknown.min_display_mastering_luminance == 0 );
    assert( unknown.max_cll == 0 );
    assert( unknown.max_fall == 0 );
    assert( unknown.display_primaries[0].x == 0 );
    assert( unknown.white_point.x == 0 );

    // A partly populated EDID must not acquire a made-up MaxFALL.
    unknown_hdr.desired_content_max_luminance = 600;
    gamescope::PopulateHdrContentLightLevelsFromEdid( unknown, unknown_hdr );
    assert( unknown.max_cll == 600 && unknown.max_fall == 0 );

    std::printf( "SDC4501 EDID feedback: MaxCLL 1405, MaxFALL 800; mastering and absent fields stay unknown\n" );
    di_info_destroy( info );
    return 0;
}
