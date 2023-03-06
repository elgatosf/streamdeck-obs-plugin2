# obs.frontend

## Functions

### obs.frontend.streaming.start

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if the stream was inactive and is now starting, otherwise `false`.

### obs.frontend.streaming.stop

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if the stream was active and is now stopping, otherwise `false`.

### obs.frontend.streaming.active

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if the stream is active, otherwise `false`.

### obs.frontend.recording.start

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if the recording was inactive and is now starting, otherwise `false`.

### obs.frontend.recording.stop

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if the recording was active and is now stopping, otherwise `false`.

### obs.frontend.recording.active

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if the recording is active, otherwise `false`.

### obs.frontend.recording.pause

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if the recording is paused, otherwise `false`.

### obs.frontend.recording.unpause

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if the recording is not paused, otherwise `false`.

### obs.frontend.recording.paused

##### Parameters
##### Returns
A <small>boolean</small> describing the current pause state of Recording.

### obs.frontend.replaybuffer.enabled

##### Parameters
##### Returns
A <small>boolean</small> which `true` if Replay Buffer is enabled (not active).

### obs.frontend.replaybuffer.start

##### Parameters
##### Returns
A <small>boolean</small> describing the current state of Replay Buffer.

### obs.frontend.replaybuffer.save

##### Parameters
##### Returns
A <small>boolean</small> which is `true` if saving the Replay Buffer was possible, otherwise `false`.

### obs.frontend.replaybuffer.stop

##### Parameters
##### Returns
A <small>boolean</small>, which is `true` is the Replay Buffer is stopped instantly, otherwise `false`.

### obs.frontend.replaybuffer.active

##### Parameters
##### Returns
A <small>boolean</small> describing the current state of Replay Buffer.

### obs.frontend.studiomode
Get or Set the state of Studio Mode

##### Parameters
* <small>boolean</small> `enabled` *(Optional)*
  `true` if Studio Mode should be enabled, otherwise `false`.

### obs.frontend.virtualcam
Get or Set the state of the virtualcam

##### Parameters
* <small>boolean</small> `enabled` *(Optional)*
  `true` if virtualcam should be started otherwise `false`.

##### Returns
A <small>boolean</small> which is `true` if virtualcam is enabled, otherwise `false`.

### obs.frontend.scenecollection
Get or Set the current Scene Collection

##### Parameters
* <small>string</small> `collection` *(Optional)*
  The name of the scene collection to load.

##### Returns
The name of the currently active scene collection.

### obs.frontend.scenecollection.list
Enumerate all scene collections known to the front-end of OBS Studio.

##### Parameters

##### Returns
An <small>Array</small> of <small>string</small>s containing the unique names of scene collections.

### obs.frontend.profile
Get or Set the current profile

##### Parameters
* <small>string</small> `profile` *(Optional)*
  The name of the profile to load.

##### Returns
The name of the currently active profile.

### obs.frontend.profile.list
Enumerate all profiles known to the front-end of OBS Studio.

##### Parameters

##### Returns
An <small>Array</small> of <small>string</small>s containing the unique names of profiles.

### obs.frontend.scene
Get or Set the current Preview/Program scene.

##### Parameters
* <small>boolean</small> `program` *(Optional)*
  `false` if you want to get/set the Preview scene, otherwise `true` to get the Program scene. Only useful in Studio Mode.
* <small>string</small> `scene` *(Optional)*
  The scene to which to transition to.

##### Returns
The name of the current Preview or Program scene.

### obs.frontend.scene.list
Enumerate all scenes known to the front-end of OBS Studio.

##### Parameters

##### Returns
An <small>Array</small> of <small>string</small>s containing the unique names of scenes.

### obs.frontend.transition
Get or Set the current Transition

##### Parameters
* <small>string</small> `collection` *(Optional)*
  The name of the scene transition to load.

##### Returns
The name of the currently active Transition.

### obs.frontend.transition.list
Enumerate all transitions known to the front-end of OBS Studio.

##### Parameters

##### Returns
An <small>Array</small> of <small>string</small>s containing the unique names of transitions.

### obs.frontend.screenshot
Take a screenshot of a source or the current program output. The behavior is undefined if the given source does not exist, or the given source is not a video source.

##### Parameters
* <small>string</small> `source` *(Optional)*
  The unique name of the source to take a screenshot of.

##### Returns
Always returns the <small>boolean</small> `true`.


### obs.frontend.stats
Fetch a snapshot of the current OBS stats

##### Parameters

##### Returns
An object containing all of data required to assemble a stats dialog similar to the built-in stats dock.

| Stat Name                          | Which Fields to Use                                                                                |
|------------------------------------|----------------------------------------------------------------------------------------------------|
| CPU Usage                          | .cpu (Percentage 0.0..100.0)                                                                       |
| Disk space available               | .recording.freeSpace (bytes)                                                                       |
| Disk full in                       | Calculate with .recording.freeSpace * 8 / recording bitrate, use average bitrate over 10+ seconds  |
| Memory Usage                       | .memRSS (bytes)                                                                                    |
| FPS                                | .fpsTarget                                                                                         |
| Average time to render frame       | .frameTimeNS (integer nanoseconds)                                                                 |
| Frames missed due to rendering lag | .framesSkipped / .framesEncoded                                                                    |
| Skipped frames due to encoding lag | .framesLagged / .framesTotal                                                                       |
| Output Name                        | .{type}.name                                                                                       |
| Output Status                      | .{type}.reconnecting ? "Connecting" : .{type}.paused ? "Paused" : .type.active ? "Active / Streaming / Recording" : "Inactive" |
| Output Dropped Frames              | .{type}.drops                                                                                      |
| Output Total Data Output           | .{type}.bytes                                                                                      |
| Output Bitrate                     | Calculate based on delta between stats calls using .{type}.bytes                                   |

```js
{
  "fpsTarget": Double, // Configured FPS
  "obsFps": Double, // Actual FPS
  "cpu": Double, // 0 - 100
  "memRSS": Int, // Bytes of OBS resident set
  "frameTimeNS", Int, // Average nanoseconds per frame
  "framesEncoded", Int, // Video frames generated by OBS
  "framesSkipped", Int, // Video frames skipped by OBS
  "framesTotal", Int, // Total encoded frames
  "framesLagged", Int, // Frames skipped due to encoding lag
  "stream": {
    "id": String,
    "name": String,
    "bytes": Int, // Total bytes sent on the network
    "frames": Int, // Total frames sent to stream
    "drops": Int, // Total frames dropped
    "paused": Bool,
    "active": Bool,
    "reconnecting": Bool,
    "delay": Int, // Stream delay in seconds
    "congestion": Double, // Arbitrary congestion factor between 0.0 .. 1.0
    "connectTime": Int // Millisecconds taken to connect to the server
  }
  "recording": {
    "id": String,
    "name": String,
    "bytes": Int, // Total bytes recorded
    "frames": Int, // Total frames sent to the recording
    "drops": Int, // Total frames dropped
    "paused": Bool,
    "active": Bool,
    "delay": Int, // Recording delay in seconds
    "path": String, // Path to where recordings are saved (OBS 28+)
    "freeSpace": Int // Free bytes on the path where recordings are saved (OBS 28+)
  }
}
```


### obs.frontend.tbar
Sets and/or returns the state of the studio mode tbar

##### Parameters
* {int} **position** The position to set the tbar to, in the range 0..1023 *optional*
* {int} **offset** The offset to apply to the tbar, in the range -1023..1023 *optional*
* {boolean} **release** Used to release the tbar, which will execute a transition if the tbar is in the correct range for it *optional*

##### Returns
An object containing all of data required to assemble a stats dialog similar to the built-in stats dock.

## Events / Notifications
### obs.frontend.event.streaming
Called whenever the OBS Studio frontend changes the streaming status.

#### Parameters
* {string} **state** One of the following:
    * **"STARTING"**: Streaming is starting.
    * **"STARTED"**: Streaming has started.
    * **"STOPPING"**: Streaming is stopping.
    * **"STOPPED"**: Streaming has stopped.

### obs.frontend.event.recording
Called whenever the OBS Studio frontend changes the recording status.

#### Parameters
* {string} **state** One of the following:
    * **"STARTING"**: Recording is starting.
    * **"STARTED"**: Recording has started.
    * **"STOPPING"**: Recording is stopping.
    * **"STOPPED"**: Recording has stopped.
    * **"PAUSED"**: Recording is paused.
    * **"UNPAUSED"**: Recording is unpaused.

### obs.frontend.event.replaybuffer
Called whenever the OBS Studio frontend changes the replay buffer status.

#### Parameters
* {string} **state** One of the following:
    * **"STARTING"**: Replay Buffer is starting.
    * **"STARTED"**: Replay Buffer has started.
    * **"STOPPING"**: Replay Buffer is stopping.
    * **"STOPPED"**: Replay Buffer has stopped.

### obs.frontend.event.scene
The program scene has changed to another scene.
#### Parameters
* {string} **scene**: The name of the new active scene.

### obs.frontend.event.scene.preview
The preview scene has changed to another scene.
#### Parameters
* {string} **scene**: The name of the new active scene.

### obs.frontend.event.scenes
The list of scenes has changed.
#### Parameters
* {array} **scenes**: An array of strings of the new scene names.

### obs.frontend.event.scenecollectioncleanup

### obs.frontend.event.transition
Current transition was changed.
#### Parameters
* {string} **transition**: Name of the new transition.

### obs.frontend.event.transition.duration
Transition duration was changed
#### Parameters
* {number} **duration**: New duration.

### obs.frontend.event.transitions
The list of transitions has changed.
#### Parameters
* {array} **transitions**: An array of strings of the new transition names.

### obs.frontend.event.virtualcam
When the virtualcam is started or stopped
#### Parameters
* {string} **state**: One of the following:
    * **"STARTED"**: Virtualcam has started
    * **"STOPPED"**: Virtualcam has stopped

### obs.frontend.event.profile
The active profile has changed
#### Parameters
* {string} **profile**: The name of the now-active profile


### obs.frontend.event.profiles
The list of profiles has changed
#### Parameters
* {array} **profiles**: An array of strings containing the current list of profiles


### obs.frontend.event.tbar
The state of the studio mode tbar has changed
#### Parameters
* {int} **position**: The new position of the tbar

