# Ember+ Integration Plan

For the current user-facing Ember+ tree and parameter list, see [../../../EMBER_TREE.md](../../../EMBER_TREE.md).

## Purpose

This document turns the initial Ember+ feasibility analysis into an implementation plan for CasparCG Server.

Target outcome:

- Ember+ runs inside the CasparCG process.
- Ember+ is exposed as another built-in controller, alongside AMCP.
- All useful runtime state currently exposed through OSC is available through Ember+.
- All control surfaces currently exposed through AMCP are reachable from Ember+, either as native Ember parameters/functions or through a compatibility bridge.

This document is based on the current code in:

- `src/shell/server.cpp`
- `src/protocol/amcp/*`
- `src/protocol/osc/*`
- `src/core/monitor/monitor.h`
- `src/core/video_channel.cpp`
- `src/modules/newtek/newtek.cpp`

## Summary

Ember+ should be implemented as a new protocol module under `src/protocol/ember`, backed by:

- `libs101` for S101 framing and keepalive/provider-state messages
- `libember` for Glow tree encoding/decoding

`tinyember` should not be embedded. It depends on Qt and is structured like a sample provider application, not a lightweight server-side library.

The recommended rollout is hybrid:

1. Ship a read-only Ember+ tree for runtime state and configuration.
2. Add native Ember parameters and functions for the stable, typed control surface.
3. Add a compatibility function layer that can execute arbitrary AMCP commands from Ember+ so parity is available early.
4. Gradually replace compatibility-only areas with first-class Ember nodes/functions where that adds value.

## Current CasparCG Shape

### What already fits Ember+

- AMCP command execution is centralized in `AMCPCommandsImpl.cpp`.
- AMCP command parsing and queueing are centralized in `AMCPProtocolStrategy.cpp` and `AMCPCommandQueue.cpp`.
- OSC telemetry already comes from a typed hierarchical state tree (`core::monitor::state`).
- Each channel produces a state snapshot every tick in `video_channel.cpp`.
- The server already supports multiple TCP protocol listeners through `AsyncEventServer`.

### What does not fit directly

- AMCP is mostly string-based and command-oriented.
- The AMCP repository stores command names and minimum argument counts, but not rich metadata such as:
  - parameter names
  - parameter types
  - valid ranges
  - read/write access
  - result schema
- Some AMCP replies are XML blobs or arbitrary text.
- Some AMCP features are session-specific rather than tree-specific:
  - `REQ <id>`
  - `BEGIN/COMMIT/DISCARD`
  - `MIXER ... DEFER`
  - socket-bound lock ownership
  - `BYE`
- Some commands are added by modules, not just core AMCP registration.

## Design Goals

### In scope

- Built-in Ember+ controller over TCP
- Read-only monitoring tree
- Read/write parameters where Caspar already has stable typed state
- Ember functions for action-style controls
- Compatibility path for hard-to-model AMCP commands
- Module extension points
- Proper subscription and change notification

### Out of scope for first release

- Perfect one-to-one remodeling of every AMCP string reply into elegant typed Ember structures
- Matrix modeling unless a real routing use case appears
- Exact preservation of AMCP request-id semantics
- Full client-specific transaction semantics beyond what Ember naturally supports

## Recommended Library Choice

### Use

- `libs101`
- `libember`

### Do not use

- `tinyember`

Reasoning:

- `libember` is C++ and maps naturally to Caspar's C++ codebase.
- `libember` includes Glow node/parameter/function support, which is the right abstraction for a live provider tree.
- `libs101` already handles transport-level framing concepts needed by Ember+.
- `tinyember` depends on Qt and carries provider-app assumptions that do not fit the current server architecture.

### Optional fallback

`libember_slim` is viable if binary size becomes a hard constraint, but it would make the provider implementation more procedural and harder to maintain in Caspar's codebase.

## Build and Source Layout

Recommended source layout:

- `src/protocol/ember/README.md`
- `src/protocol/ember/ember_protocol_strategy.h`
- `src/protocol/ember/ember_protocol_strategy.cpp`
- `src/protocol/ember/ember_session.h`
- `src/protocol/ember/ember_session.cpp`
- `src/protocol/ember/ember_provider.h`
- `src/protocol/ember/ember_provider.cpp`
- `src/protocol/ember/ember_tree.h`
- `src/protocol/ember/ember_tree.cpp`
- `src/protocol/ember/ember_monitor_bridge.h`
- `src/protocol/ember/ember_monitor_bridge.cpp`
- `src/protocol/ember/ember_command_bridge.h`
- `src/protocol/ember/ember_command_bridge.cpp`
- `src/protocol/ember/ember_registry.h`
- `src/protocol/ember/ember_registry.cpp`

Recommended vendoring approach:

- Vendor `libs101` and `libember` into the source tree and build them statically.
- Keep them under a dedicated third-party path, for example:
  - `src/third_party/ember-plus/libs101`
  - `src/third_party/ember-plus/libember`

This keeps Ember+ part of Caspar's codebase and avoids a second external service or packaging dependency.

## Controller Integration

`server.cpp` already creates protocol listeners from controller config entries. Ember+ should become another protocol name next to `AMCP`.

Expected config shape:

```xml
<controllers>
    <tcp>
        <port>5250</port>
        <protocol>AMCP</protocol>
    </tcp>
    <tcp>
        <port>9000</port>
        <protocol>EMBER_PLUS</protocol>
    </tcp>
</controllers>

<ember-plus>
    <provider-id>CasparCG</provider-id>
    <display-name>CasparCG Server</display-name>
    <telemetry-rate-hz>10</telemetry-rate-hz>
    <compatibility-amcp-function>true</compatibility-amcp-function>
</ember-plus>
```

Changes needed:

- Extend `server::impl::create_protocol()` in `src/shell/server.cpp`.
- Create `create_char_ember_strategy_factory(...)`.
- Initialize one shared `ember_provider` instance during server startup.
- Pass shared state registries and channel list into the provider the same way AMCP receives them today.

## Execution Model

### Transport

- Reuse `AsyncEventServer`.
- Ember strategy must consume raw TCP bytes, not delimiter-separated lines.
- The protocol strategy should:
  - decode S101 frames
  - handle keepalive
  - handle provider-state messages
  - decode Glow requests
  - forward decoded requests into the provider/session layer

### Session

Each Ember connection should get a session object that tracks:

- socket identity
- subscription state
- lock ownership objects bound to the connection lifecycle
- pending invocations if needed
- client-side throttling state

### Threading

Do not encode and transmit Glow updates on the channel tick thread.

Recommended model:

- Channel tick threads produce `monitor::state` snapshots as they do today.
- `ember_monitor_bridge` converts those snapshots into lightweight internal update events.
- `ember_provider` owns a dedicated worker/executor thread that:
  - updates cached Ember-facing state
  - computes diffs
  - emits notifications to subscribed sessions

This keeps realtime rendering isolated from network encoding overhead.

## Internal Control Model

Ember should not call AMCP protocol parsing for every native action if we can avoid it.

Recommended direction:

- Keep AMCP string parsing for compatibility functions.
- Add a reusable command bridge for Ember-native operations that directly invokes the same core operations as AMCP command implementations.
- Where practical, factor AMCP implementations into reusable helpers rather than duplicating business logic.

Examples:

- `PLAY`, `STOP`, `PAUSE`, `RESUME`, `CLEAR` already map directly to `stage` methods.
- `SET MODE` maps directly to `stage()->video_format_desc(...)`.
- `MIXER MASTERVOLUME` maps directly to `mixer().set_master_volume(...)`.
- `INFO`, `INFO CONFIG`, `INFO PATHS` should read state/config directly, not stringify AMCP responses and reparse them.

## Proposed Ember Tree

Top-level tree:

```text
/1  identity
/2  runtime
/3  config
/4  media
/5  compatibility
/100 channels
/200 modules
```

### `/1 identity`

Read-only metadata.

```text
/1/1 product                string   RO   "CasparCG Server"
/1/2 version                string   RO
/1/3 git_hash               string   RO
/1/4 company                string   RO   "CasparCG"
/1/5 ember_provider_id      string   RO
```

### `/2 runtime`

Process-wide state and actions.

```text
/2/1 log_level              enum     RW
/2/2 diag_enabled           bool     RW
/2/3 uptime_seconds         integer  RO
/2/4 active_amcp_clients    integer  RO
/2/5 active_ember_clients   integer  RO
/2/10 restart()             function
/2/11 kill()                function
/2/12 gl_gc()               function
/2/13 gl_info()             function -> result tuple/string
```

Notes:

- `diag_enabled` should replace the AMCP-only `DIAG` toggle shape with a proper parameter.
- `BYE` should not be modeled as a public function; client disconnect is a transport concern.

### `/3 config`

Read-mostly configuration and paths.

```text
/3/1 paths
/3/1/1 media_path           string   RO
/3/1/2 data_path            string   RO
/3/1/3 log_path             string   RO
/3/1/4 template_path        string   RO
/3/1/5 initial_path         string   RO

/3/2 controllers
/3/3 channels_config
```

Notes:

- First release should expose this read-only.
- `INFO CONFIG` and `INFO PATHS` can be backed from this subtree instead of AMCP text generation.

### `/4 media`

Media-scanner backed queries and thumbnail helpers.

```text
/4/1 cls()                  function -> string/xml result
/4/2 fls()                  function -> string/xml result
/4/3 tls()                  function -> string/xml result
/4/4 cinf(name)             function -> string/xml result
/4/10 thumbnails
/4/10/1 list()              function
/4/10/2 retrieve(name)      function
/4/10/3 generate(name)      function
/4/10/4 generate_all()      function
```

Notes:

- Keep these as functions first.
- They already depend on the media-scanner proxy in current Caspar code.

### `/5 compatibility`

Guaranteed parity bridge for AMCP surfaces that do not yet have native Ember modeling.

```text
/5/1 amcp_execute(command : string) -> reply : string
/5/2 amcp_execute_many(commands : tuple<string>) -> tuple<string>
```

This is the main mechanism that allows us to ship Ember+ with full practical AMCP coverage before every command has a native Ember representation.

### `/100 channels`

Dynamic child node per channel index.

```text
/100/<channel> channel_<n>
```

Each channel subtree:

```text
/100/<n>/1  status
/100/<n>/2  video
/100/<n>/3  lock
/100/<n>/4  layers
/100/<n>/5  mixer
/100/<n>/6  outputs
/100/<n>/7  commands
```

#### Channel status

Backed from `video_channel::state()` and monitor snapshots.

```text
/status/1 format            string   RO
/status/2 framerate_num     integer  RO
/status/3 framerate_den     integer  RO
/status/4 online            bool     RO
/status/5 stage             node     RO
/status/6 output            node     RO
/status/7 mixer             node     RO
```

#### Channel video

```text
/video/1 mode               string   RW
/video/2 color_depth        integer  RO
/video/3 color_space        string   RO
```

#### Channel lock

```text
/lock/1 is_locked           bool     RO
/lock/2 acquire(phrase)     function
/lock/3 release()           function
/lock/4 clear(phrase)       function
```

Notes:

- Lock access must preserve current lifecycle-bound semantics from `lock_container`.
- Session ownership must remain per connection.

#### Channel layers

Dynamic child node per active layer number.

```text
/layers/<layer>/1 state
/layers/<layer>/2 transport
/layers/<layer>/3 template
/layers/<layer>/4 mixer
/layers/<layer>/5 compatibility
```

##### Layer state

Backed from `stage` and `layer` state.

```text
/state/1 foreground_producer    string   RO
/state/2 background_producer    string   RO
/state/3 paused                 bool     RO
/state/4 frames_left            integer  RO
```

##### Layer transport functions

```text
/transport/1 loadbg(args...)    function
/transport/2 load(args...)      function
/transport/3 play(args...)      function
/transport/4 pause()            function
/transport/5 resume()           function
/transport/6 stop()             function
/transport/7 clear()            function
/transport/8 call(args...)      function -> string
/transport/9 callbg(args...)    function -> string
/transport/10 swap(target)      function
```

Notes:

- `args...` means tuple-style invocation arguments. First release can use simple string tuples where necessary.
- `PLAY` and `LOADBG` have argument surfaces too wide to force into a tiny typed schema on day one.

##### Layer template functions

```text
/template/1 cg_add(args...)     function
/template/2 cg_play(layer)      function
/template/3 cg_stop(layer)      function
/template/4 cg_next(layer)      function
/template/5 cg_remove(layer)    function
/template/6 cg_clear()          function
/template/7 cg_update(args...)  function
/template/8 cg_invoke(args...)  function
```

##### Layer mixer

For stable typed values, prefer parameters over functions.

Simple RW parameters:

```text
/mixer/1 keyer                  bool     RW
/mixer/2 invert                 bool     RW
/mixer/3 opacity                real     RW
/mixer/4 brightness             real     RW
/mixer/5 saturation             real     RW
/mixer/6 contrast               real     RW
/mixer/7 rotation_degrees       real     RW
/mixer/8 volume                 real     RW
```

Composite function-or-tuple areas:

```text
/mixer/20 levels                function/tuple
/mixer/21 fill                  function/tuple
/mixer/22 clip                  function/tuple
/mixer/23 anchor                function/tuple
/mixer/24 crop                  function/tuple
/mixer/25 perspective           function/tuple
/mixer/26 chroma                function/tuple
/mixer/27 blend                 function/tuple/enum
/mixer/28 commit()              function
/mixer/29 clear()               function
```

Recommended rule:

- If the AMCP command naturally reads/writes a single scalar, model it as a parameter.
- If it is a grouped structure or has optional duration/tween/defer behavior, model it as a function in phase 1, then revisit later.

#### Channel mixer

Channel-wide mixer state:

```text
/mixer/1 master_volume          real     RW
/mixer/2 grid(n, duration, tween) function
```

#### Channel outputs

Backed from `output::state()` plus control functions.

```text
/outputs/1 ports
/outputs/2 add_consumer(args...)     function
/outputs/3 remove_consumer(args...)  function
/outputs/4 apply(args...)            function
/outputs/5 print(args...)            function
```

Each port subtree should expose:

- consumer name
- online state if available
- consumer-specific status values from the existing monitor tree

### `/200 modules`

Extension area for module-provided Ember nodes and functions.

Example:

```text
/200/ndi/1 list()              function
```

This keeps module-specific Ember surfaces out of the fixed core tree while still making them discoverable.

## Mapping Strategy by AMCP Category

### Basic Commands

- Native Ember functions:
  - `LOADBG`
  - `LOAD`
  - `PLAY`
  - `PAUSE`
  - `RESUME`
  - `STOP`
  - `CLEAR`
  - `CALL`
  - `CALLBG`
  - `SWAP`
  - `ADD`
  - `REMOVE`
  - `APPLY`
  - `PRINT`
  - `CLEAR ALL`
- Native Ember parameters:
  - `LOG LEVEL`
  - `SET MODE`
- Native lock subtree:
  - `LOCK`

### Data Commands

Keep as functions first:

- `DATA STORE`
- `DATA RETRIEVE`
- `DATA LIST`
- `DATA REMOVE`

Reason:

- They are file-oriented and often return free-form data.

### Template Commands

Keep as Ember functions under the layer template subtree.

### Mixer Commands

Split into:

- scalar RW parameters
- grouped tuple functions
- explicit `commit()` and `clear()` functions

### Thumbnail Commands

Keep as media functions first.

### Query Commands

Model as:

- direct RO parameters or nodes where stable
- functions where the result is remote or large

Examples:

- native RO:
  - `VERSION`
  - `INFO`
  - `INFO CONFIG`
  - `INFO PATHS`
- native functions:
  - `GL INFO`
  - `GL GC`
  - `CINF`
  - `CLS`
  - `FLS`
  - `TLS`
  - `KILL`
  - `RESTART`
- not needed as-is:
  - `OSC SUBSCRIBE`
  - `OSC UNSUBSCRIBE`

Reason:

- Ember subscriptions make the OSC subscribe commands unnecessary.

## Special Semantics

### AMCP batching

AMCP supports:

- `BEGIN`
- `COMMIT`
- `DISCARD`

Ember does not naturally provide the same generic command batching shape.

Recommendation:

- Do not block the project on generic batch equivalence.
- Preserve operational parity through `/compatibility/amcp_execute_many(...)`.
- For high-value typed operations, prefer purpose-built Ember functions that update all required fields in one invocation.

### `MIXER ... DEFER` and `MIXER COMMIT`

Current AMCP behavior stores deferred transforms per channel.

Recommendation:

- Preserve current semantics for compatibility execution.
- For native Ember operations, first release should expose:
  - normal immediate write behavior
  - `commit()` where current deferred state exists
- If clients truly need deferred staging through Ember, add a session-local deferred-transform staging model later.

### Request ids

`REQ <id>` is an AMCP reply-correlation feature.

Recommendation:

- Do not reproduce this in the native Ember tree.
- Function invocation results in Ember are already correlated by the protocol itself.

### Locks

Locks must remain bound to connection lifecycle. Ember sessions need the same lifecycle-bound object model currently used by AMCP.

## Provider Update Strategy

### Source of truth

Use existing Caspar state, not a second shadow state machine.

Sources:

- `video_channel::state()`
- `stage::state()`
- `output::state()`
- channel configuration
- direct core objects for writable parameters
- module callbacks for module-specific state

### Notification strategy

- Maintain a cached Ember-facing tree representation.
- On update, compute dirty paths and notify only relevant subscribers.
- Throttle high-frequency telemetry.

Recommended defaults:

- fixed telemetry cap, for example `10 Hz`
- force immediate push for user-driven write acknowledgements
- coalesce multiple changes on the same channel/layer in the same cycle

## Module Extension Strategy

AMCP already allows modules to register commands through `module_dependencies`.

Ember needs a peer extension point.

Recommended API:

```cpp
struct ember_extension_registry;

dependencies.ember_registry->register_function(...);
dependencies.ember_registry->register_node(...);
dependencies.ember_registry->register_state_provider(...);
```

This should support:

- module functions
- module-owned read-only parameters
- module-owned live state subtree updates

Without this, AMCP and Ember parity will drift as soon as modules add commands.

## Phase Plan

### Phase 0: foundation

Estimated scope: 1 week

- Vendor `libs101` and `libember`
- Add build integration
- Create `src/protocol/ember`
- Add `EMBER_PLUS` controller parsing in `server.cpp`
- Add provider/session skeleton
- Add keepalive/provider-state handling

Exit criteria:

- Caspar starts with an Ember+ listener
- a consumer can connect
- provider state and keepalive work

### Phase 1: read-only provider

Estimated scope: 1 to 2 weeks

- Expose `/identity`
- Expose `/runtime`
- Expose `/config`
- Expose `/channels/.../status`
- Bridge `monitor::state` to Ember notifications
- Add throttled subscription updates

Exit criteria:

- TinyEmberPlus or another Ember consumer can browse the tree
- channel status updates arrive correctly
- no measurable regression on channel tick timing

### Phase 2: native writable core controls

Estimated scope: 2 to 3 weeks

- Add writable channel video mode
- Add channel and layer mixer controls
- Add basic layer transport functions
- Add lock subtree
- Add channel output control functions

Exit criteria:

- common playout operations are executable from Ember
- lock behavior matches AMCP expectations
- scalar parameter writes and function invocations behave predictably

### Phase 3: compatibility bridge

Estimated scope: 1 to 2 weeks

- Add `/compatibility/amcp_execute`
- Add `/compatibility/amcp_execute_many`
- Route through the existing AMCP parser/executor
- Preserve module-added AMCP commands through this path

Exit criteria:

- anything reachable through AMCP is at least reachable through Ember compatibility
- parity gaps no longer block rollout

### Phase 4: deeper parity and refinement

Estimated scope: 2 to 4 weeks

- add media and thumbnail query functions
- add data command functions
- add template command refinement
- add module Ember extension registry
- replace compatibility-only areas with native Ember nodes where useful

Exit criteria:

- most operational workflows no longer need the compatibility bridge
- module extensions are supported

### Phase 5: hardening

Estimated scope: 1 to 2 weeks

- interop testing with real Ember controllers
- soak tests under high update rate
- error and disconnect behavior validation
- documentation and example config

Exit criteria:

- acceptable stability under long-running playout
- no transport leaks
- no channel-thread stalls due to Ember traffic

## Testing Plan

### Unit tests

- tree path mapping
- monitor-state to Ember update conversion
- AMCP compatibility bridge
- lock lifecycle behavior
- scalar value mapping and range validation

### Integration tests

- connect Ember consumer and browse root tree
- subscribe to channel state and verify updates
- invoke `play/stop/pause/resume`
- write mixer scalar values
- execute compatibility AMCP commands
- verify disconnect releases locks

### Performance checks

- channel tick time before and after Ember enabled
- burst subscription load with multiple clients
- worst-case update rate on multi-channel systems

## Risks

### Main risks

- high-rate telemetry causing extra load on realtime threads
- metadata gaps forcing awkward command/function schemas
- module parity drifting without an Ember extension API
- overcommitting to elegant native modeling before compatibility coverage exists

### Mitigations

- isolate provider encoding on a worker thread
- ship compatibility AMCP execution early
- keep native modeling focused on high-value typed areas first
- add module extension registration before calling the feature complete

## Recommended First Milestone

If implementation starts immediately, the first milestone should be:

- Ember+ listener starts
- consumer can browse identity/runtime/config/channels
- subscriptions work for channel status
- native RW support for:
  - `runtime.log_level`
  - `channel.video.mode`
  - `channel.mixer.master_volume`
  - common layer mixer scalars
- native functions for:
  - `play`
  - `stop`
  - `pause`
  - `resume`
  - `clear`
- compatibility function for arbitrary AMCP execution

That milestone provides immediate value without waiting for full native parity.

## Decision

Proceed with Ember+ as a built-in TCP protocol using a hybrid model:

- native Ember tree for monitoring and stable typed controls
- compatibility AMCP execution for full command coverage
- module extension registry for long-term parity

This is the lowest-risk path that still produces a real Ember+ provider inside CasparCG rather than an external bridge.
