# CasparCG Ember+ Tree

This document describes the Ember+ tree exposed by the `ember-plus` branch of CasparCG Ember+.

The tree is dynamic:

- active channels are always present
- active layers appear only when a layer is actually on-air or loaded in the channel state
- clip lists are populated from the CasparCG media folder
- live state and diagnostics refresh on the configured Ember+ update interval

## Root Tree

```text
/1    identity
/2    runtime
/5    compatibility
/100  channels
/200  modules
```

## Example Tree

Example with one configured channel and two active layers:

```text
identity
  product
  protocol

runtime
  channel_count
  state_update_interval_ms
  System
    ProcessCpu%
    SystemCpu%
    ProcessResidentMB
    SystemTotalMemoryMB
    SystemAvailableMemoryMB
    SystemUsedMemory%

compatibility
  amcp_execute()

channels
  channel_1
    index
    format
    Play
      Layer
      Loop
      Clip
      Seek
      Length
      Filter
      Clear404
      Transition
      Duration
      Tween
      Direction
      Execute
      Reply
      Success
    LoadBg
      Layer
      Loop
      Clip
      Seek
      Length
      Filter
      Clear404
      Auto
      Transition
      Duration
      Tween
      Direction
      Execute
      Reply
      Success
      Sting
        Enable
        Mask
        Trigger
        Overlay
        AudioFadeStart
        AudioFadeDuration
    Load
      Layer
      Clip
      Loop
      Seek
      Length
      Filter
      Clear404
      Execute
      Reply
      Success
    Pause
      Layer
      Execute
      Reply
      Success
    Resume
      Layer
      Execute
      Reply
      Success
    Stop
      Layer
      Execute
      Reply
      Success
    Clear
      Layer
      Execute
      Reply
      Success
    Refresh
      Execute
      Count
      Reply
      Success
    Call
      Layer
      Args
      Execute
      Reply
      Success
    CallBg
      Layer
      Args
      Execute
      Reply
      Success
    Diagnostics
      VideoChannel
        produceTime
        mixTime
        consumeTime
        frameTime
        oscTime
    Layers
      Layer_1
        State
          Foreground
          Background
          Paused
          FramesLeft
          ForegroundFileName
          ForegroundFilePath
          ForegroundTimeCurrent
          ForegroundTimeDuration
          ForegroundClipStart
          ForegroundClipDuration
          ForegroundLoop
          ForegroundFileStreamsV0FpsNumerator
          ForegroundFileStreamsV0FpsDenominator
        Mixer
          Keyer
          Invert
          Opacity
          Opacity%
          Brightness
          Brightness%
          Saturation
          Saturation%
          Contrast
          Contrast%
          Rotation
          Volume
          Volume%
          Blend
          FillX
          FillX%
          FillY
          FillY%
          FillW
          FillW%
          FillH
          FillH%
          ClipX
          ClipX%
          ClipY
          ClipY%
          ClipW
          ClipW%
          ClipH
          ClipH%
          AnchorX
          AnchorX%
          AnchorY
          AnchorY%
          CropL
          CropL%
          CropT
          CropT%
          CropR
          CropR%
          CropB
          CropB%
        Diagnostics
          Ffmpeg
            decodeTime
            frameTime
            buffer
            underflowCount
      Layer_2
        State
        Mixer
        Diagnostics

modules
```

The exact diagnostic source names depend on what is active in CasparCG. Typical names are `VideoChannel`, `Ffmpeg`, `Route`, `Decklink`, `Html`, `Newtek`, `AudioMixer`, and similar.

## Node Details

## `/1 identity`

Read-only metadata:

- `product`
- `protocol`

## `/2 runtime`

Runtime provider values:

- `channel_count`
- `state_update_interval_ms`

`state_update_interval_ms` is writable. Changing it via Ember+ updates the running provider immediately and also writes the new value into `casparcg.config`.

### `/2/System`

Read-only runtime process and system statistics:

- `ProcessCpu%`
- `SystemCpu%`
- `ProcessResidentMB`
- `SystemTotalMemoryMB`
- `SystemAvailableMemoryMB`
- `SystemUsedMemory%`

## `/5 compatibility`

Compatibility bridge:

- `amcp_execute()`

This is used for AMCP parity where a native Ember node does not yet exist.

## `/100/channels/channel_<n>`

Each configured channel exposes:

- `index`
- `format`
- `Play`
- `LoadBg`
- `Load`
- `Pause`
- `Resume`
- `Stop`
- `Clear`
- `Refresh`
- `Call`
- `CallBg`
- `Diagnostics`
- `Layers`

## Clip Control Nodes

### `Play`

- `Layer`
- `Loop`
- `Clip`
- `Seek`
- `Length`
- `Filter`
- `Clear404`
- `Transition`
- `Duration`
- `Tween`
- `Direction`
- `Execute`
- `Reply`
- `Success`

### `LoadBg`

Everything from `Play`, plus:

- `Auto`
- `Sting/Enable`
- `Sting/Mask`
- `Sting/Trigger`
- `Sting/Overlay`
- `Sting/AudioFadeStart`
- `Sting/AudioFadeDuration`

### `Load`

- `Layer`
- `Clip`
- `Loop`
- `Seek`
- `Length`
- `Filter`
- `Clear404`
- `Execute`
- `Reply`
- `Success`

### `Pause`, `Resume`, `Stop`

- `Layer`
- `Execute`
- `Reply`
- `Success`

### `Clear`

- `Layer`
- `Execute`
- `Reply`
- `Success`

`Layer = 0` means clear the whole channel.

### `Refresh`

- `Execute`
- `Count`
- `Reply`
- `Success`

`Refresh` rescans the CasparCG media folder and updates the clip enumeration used by `Play`, `LoadBg`, and `Load`.

### `Call`, `CallBg`

- `Layer`
- `Args`
- `Execute`
- `Reply`
- `Success`

## Layer Nodes

`Layers` is dynamic. A layer node appears when that layer exists in the live Caspar state.

Each active layer exposes:

- `State`
- `Mixer`
- `Diagnostics`

## Layer `State`

Always-present summary fields:

- `Foreground`
- `Background`
- `Paused`
- `FramesLeft` when Caspar provides it

Additional state is exported from CasparCG's monitor state. Common examples include:

- file name
- file path
- current clip time
- clip duration
- clip in/out values
- loop state
- stream fps values
- transition progress values

Examples of generated parameter names:

- `ForegroundFileName`
- `ForegroundFilePath`
- `ForegroundTimeCurrent`
- `ForegroundTimeDuration`
- `ForegroundClipStart`
- `ForegroundClipDuration`
- `ForegroundLoop`
- `ForegroundFileStreamsV0FpsNumerator`
- `ForegroundFileStreamsV0FpsDenominator`

The exact list depends on the active producer type.

## Layer `Mixer`

Read/write mixer controls:

- `Keyer`
- `Invert`
- `Opacity`
- `Opacity%`
- `Brightness`
- `Brightness%`
- `Saturation`
- `Saturation%`
- `Contrast`
- `Contrast%`
- `Rotation`
- `Volume`
- `Volume%`
- `Blend`
- `FillX`
- `FillX%`
- `FillY`
- `FillY%`
- `FillW`
- `FillW%`
- `FillH`
- `FillH%`
- `ClipX`
- `ClipX%`
- `ClipY`
- `ClipY%`
- `ClipW`
- `ClipW%`
- `ClipH`
- `ClipH%`
- `AnchorX`
- `AnchorX%`
- `AnchorY`
- `AnchorY%`
- `CropL`
- `CropL%`
- `CropT`
- `CropT%`
- `CropR`
- `CropR%`
- `CropB`
- `CropB%`

Notes:

- numeric mixer values use proper Ember ranges
- `%` variants are alternate views of the same control
- layer mixer values are read from live Caspar state, so the node reflects current values even when the change came from outside Ember+

## Diagnostics

Diagnostics are exported in two places:

- channel-wide diagnostics under `channel_<n>/Diagnostics`
- layer-specific diagnostics under `channel_<n>/Layers/Layer_<m>/Diagnostics`

These are read-only and update on the configured Ember+ refresh interval.

Examples of diagnostic values:

- `produceTime`
- `mixTime`
- `consumeTime`
- `frameTime`
- `oscTime`
- `decodeTime`
- `buffer`
- `volume`

Examples of diagnostic event counters:

- `underflowCount`
- `lateFrameCount`
- `droppedFrameCount`
- `audioClippingCount`
- `audioBufferOverflowCount`

## `/200 modules`

Reserved root for module-provided Ember nodes.

This area exists so future module-specific Ember+ extensions can be added without reshaping the core tree.

## Notes

- `Clip`, `Mask`, and `Overlay` are enumerations built from the CasparCG media folder
- descriptions are intentionally short and close to the identifier names
- update frequency is controlled by the Ember+ state update interval
- live changes from AMCP, OSC-related state, or other clients are reflected back into the Ember tree

