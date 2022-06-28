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

##### Returns
A <small>boolean</small> which is `true` if Studio Mode is enabled, otherwise `false`.

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
