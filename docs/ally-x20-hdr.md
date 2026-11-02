# Ally X20 PQ content-light interoperability fix

This branch starts from Anatase's Gamescope 3.16.23.3 patch stack,
reconstructed with:

```sh
ludos patch checkout cards/gaming/gamemode.yml:gamescope
```

The change targets the internal Samsung SDC4501 / AMS740LS02-0 panel on the
ROG Xbox Ally X20. PQ remains enabled.

## Evidence

A native Vulkan scRGB pattern reproduced the washed-out game colors with
Gamescope PQ output. The same picture was held on the same swapchain at
unity HDR input gain while only HDR metadata changed:

| Stage | Mastering fields | MaxCLL / MaxFALL | Result on the panel |
| --- | --- | --- | --- |
| A | Unknown, zero | Unknown, zero | Washed out |
| B | Unknown, zero | EDID values, 1405 / 800 nits on this unit | Good colors |
| C | Unknown, zero | Unknown, zero | Washout returned |

The Vulkan WSI log confirmed that all mastering primaries, white point,
and mastering luminances stayed zero in stage B. A prior comparison with
populated mastering data also improved the picture, but that data proved
unnecessary. Native HDR reproduction excludes a Proton-only explanation.

Unknown values are permitted by the standard. This is a panel-specific
interoperability workaround, not a new metadata validity requirement.
Whether the behavior originates in the sink or AMD's implementation is
not established.

## Change

For the internal SDC4501 panel only, populate the default PQ packet's
MaxCLL and MaxFALL from libdisplay-info's decoded EDID HDR luminance fields.
Read those fields directly rather than using the connector's synthesized
Steam Deck/default luminance values. Missing EDID values remain unknown.

No mastering metadata is fabricated. These display capability values are
a compatibility estimate of content light levels, not measured content
statistics. App-provided metadata still takes precedence, including partial
metadata; its handling and validation are unchanged. Other panels and
external outputs retain the existing empty default packet.

The 384-byte regression fixture includes the panel's HDR metadata nested
inside DisplayID 2.0. The regression test checks:

- panel identity and internal/external scope;
- actual EDID MaxCLL/MaxFALL conversion;
- all mastering fields remaining zero;
- absent and partly populated EDID fields staying unknown.

## Build and test

Use the existing development script, with build-only and parallelism options:

```sh
./sync.sh --build-only --jobs 2
meson test -C build hdr-metadata-edid --print-errorlogs
```

Build and deploy to an SSH-accessible test device through the same script:

```sh
./sync.sh --jobs 2 --appid 3375780 user@device
```

Deployment replaces the development Gamescope binary and WSI layer through
a temporary system overlay and restarts the display session. The target
needs working SSH and sudo access. Save work before deployment.

Keep the gamma 2.2 user override removed. After restart, the journal should
report `Using EDID content light levels` for the panel. With the original
game focused, check BT2020_RGB, PQ EOTF, and populated MaxCLL/MaxFALL in
HDR_OUTPUT_METADATA. Mastering fields should remain zero when the game
does not supply them.

The metadata payload has passed the native visual comparison. Validation
of this branch's rebuilt compositor with the original game is a separate
remaining test; do not infer it from the native comparison or compilation.
