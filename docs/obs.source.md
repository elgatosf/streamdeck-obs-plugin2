# obs.source
This API adheres to the JSON-RPC standard, and all functions have the ability to return errors.

## Functions
### obs.source.enumerate
Enumerate all Sources with their current state.

##### Parameters
None.

##### Returns
An array of [Source State](#source-state)s.

### obs.source.state
Retrieve or change the state of a specific Source.

##### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  A valid reference to a Source.
- <small>boolean</small> `enabled` *(Optional)*  
  Set the enabled state of this Source.
- <small>number</small> `volume` *(Optional)*  
  Set the volume of this Source. Range `0.0` (silent) to `1.0` (full volume).
- <small>boolean</small> `muted` *(Optional)*  
  Mute or unmute this Source.
- <small>number</small> `balance` *(Optional)*  
  Set the balance of this Source. Range `0.0` (full left) to `1.0` (full right).

##### Returns
The [Source State](#source-state) object for the specified Source.

### obs.source.media
Retrieve or change the media state of the specific Source.

##### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  A valid reference to a Source.
- <small>string</small> `action` *(Optional)*  
  One of:  
	* `play`: Start or unpause playback.
	* `pause`: Pause playback
	* `restart`: Restart playback.
	* `stop`: Stop playback.
	* `next`: Skip to the next media in the playlist.
	* `previous`: Go back to the previous media in the playlist.
- <small>number</small> `time` *(Optional)*  
  The time as a floating point number in seconds to seek to. There is no guarantee that it will actually seek to the specified time.

##### Returns
The [Source State](#source-state) object for the specified Source.

### obs.source.filters
Enumerate all filters and their state of the specified Source.

#### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  A valid reference to a Source.

#### Returns
An array of [Source State](#source-state) objects which are filters of this Source.

### obs.source.properties
Enumerate all properties.

#### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  A valid reference to a Source.

#### Returns
An array of [Property](#property) objects.

### obs.source.settings
Retrieve or update the settings for a source.

##### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  A valid reference to a Source.
- <small>{array|object}</small> `settings` *(Optional)*  
  Either an RFC 6902 array, or a RFC 7386 patch to apply to the settings of the Source.

##### Returns
An object containing the current settings of the Source.

## Notifications
### obs.source.event.create
A new public Source was created.

#### Parameters
A [Source State](#source-state) object.

### obs.source.event.destroy
A Source was destroyed.

#### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  Reference to what Source was destroyed.

### obs.source.event.rename

#### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  Reference to which Source was renamed.
- <small>string</small> `from`  
  The old name, if any.
- <small>string</small> `to`  
  The new name, if any.

### obs.source.event.state
The overall state of a Source has changed.

#### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  Reference to which Source was updated.
- <small>[Source State](#source-state)</small> `state`  
  The new state of the Source.

### obs.source.event.media
The media state of a Source has changed.

#### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  Reference to which Source was updated.
- <small>[Source State](#source-state)</small> `state`  
  The new state of the Source.

### obs.source.event.filter.add
A private Filter was added to a Source.

#### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  Reference to which Source was updated.
- <small>[Source State](#source-state)</small> `filter`  
  The full state object of the new Filter.


### obs.source.event.filter.remove
A Filter was removed from a Source.

#### Parameters
An object containing:

- <small>[Source Reference](#source-reference)</small> `source`  
  Reference to which Filter was removed.

### obs.source.event.filter.reorder
The order of filters on a Source was changed.

#### Parameters
- <small>[Source Reference](#source-reference)</small> `source`  
  Reference to what Source was modified.
- <small>Array(string)</small> `order`  
  Array of filter names in the order they now appear.


## Structures
### Source Reference
A Source Reference may either be a <small>string</small> or an <small>Array(string)</small>. If it is a <small>string</small>, it should be treated as a reference to a public Source. If it is an <small>Array(string)</small>, the first element denotes the public Source, and the second element denotes the Filter that was referenced.

### Source State
* <small>string</small> `id`  
  Versioned Source Class Identifier.
* <small>string</small> `id_unversioned`  
  Source Class Identifier.
* <small>string</small> `name`  
  The unique name of the Source. Filter names are unique on the Source they've been added on.
* <small>string</small> `type`  
  Type of the source. May be one of the following:
  * `input`: A source that pulls data from somewhere into libOBS.
  * `filter`: A filter for data that is inside libOBS.
  * `transition`: A transition between two sources.
  * `scene`: A scene that combines many input sources into a combined view.
* <small>boolean</small> `enabled`  
  Is the Source enabled?
* <small>boolean</small> `active`  
  Is the Source currently active?
* <small>boolean</small> `visible`  
  Is the Source currently visible?
* <small>object</small> `output_flags` (was: `outputflags`)  
  Output flags that this Source has. Will contain all of the following:  
  * <small>boolean</small> `video`: Source has video output. If `async` is set, Source has asynchronous video output.
  * <small>boolean</small> `audio`: Source has audio output.
  * <small>boolean</small> `async`: Source is asynchronous.
  * <small>boolean</small> `custom_draw`: Source can't be directly rendered to a buffer and requires texture storage.
  * <small>boolean</small> `interaction`: Users can interact with this Source.
  * <small>boolean</small> `composite`: Source composites other sub-sources.
  * <small>boolean</small> `no_duplicate`: Never duplicate this Source.
  * <small>boolean</small> `deprecated`: Source is deprecated and users should avoid it.
  * <small>boolean</small> `no_monitor`: Never monitor this Source.
  * <small>boolean</small> `disabled`: Source is disabled and shouldn't be shown to the user.
  * <small>boolean</small> `auto_monitor`: Automatically monitor this Source.
  * <small>boolean</small> `submix`: The Source provides its own Audio submix.
  * <small>boolean</small> `controllable_media`: Source has media controls.
* <small>Array</small> `size`  
  Current and Base size of the Source.  
  * <small>number</small> `size[0]`: Width of the Source
  * <small>number</small> `size[1]`: Height of the Source
  * <small>number</small> `size[2]`: Base Width of the Source
  * <small>number</small> `size[3]`: Base Height of the Source
* <small>object</small> `flags`  
  Flags that this Source has. Will contain all of the following:  
  * <small>boolean</small> `unused_1`: Reserved flag that used to mean something, but is no longer used.
  * <small>boolean</small> `force_mono`: Audio output for this Source is forcefully mixed to Mono.
* <small>object</small> `audio`  
  If this source can play audio, will hold a [Audio State](#audio-state) object.
* <small>object</small> `media`  
  If this source can play media, will hold a [Media State](#media-state) object.

### Audio State
* <small>string</small> `layout`  
  Audio speaker layout that this Source uses. One of:  
  * `unknown`: No Audio or libOBS was unable to determine a proper layout.
  * `1.0`: Mono
  * `2.0`: Stereo
  * `2.1`: Stereo with LFE
  * `4.0`: Quadraphonic
  * `4.1`: Quadraphonic with LFE
  * `5.1`: Surround with LFE
  * `7.1`: Full Surround with LFE
* <small>boolean</small> `muted`  
  Is the source muted?
* <small>number</small> `volume`  
  The volume of the Source as a floating point number from `0.0` (mutedt) to `1.0` (full volume).
* <small>number</small> `balance`  
  The balance of the Source as a floating point number from `0.0` (full left) to `1.0` (full right).
* <small>number</small> `sync_offset`  
  Audio synchronization offset in milliseconds.
* <small>number</small> `mixers`  
  An integer describing which audio mixes this source is being routed to. Each bit defines a single audio mix.

### Media State
* <small>string</small> `status`  
  Status of the playback. May be one of the following:  
  * `none`: No media is playing back or this source does not support media playback.
  * `playing`: Media is currently playing.
  * `opening`: Media is being opened for playback.
  * `buffering`: Media is buffering.
  * `paused`: Media playback is paused.
  * `stopped`: Media has been stopped.
  * `ended`: Playback has ended.
  * `error`: An error occurred attempting to play the media.
* <small>Array</small> `time`  
  Time and Duration of the currently playing media, if any.  
  * <small>number</small> `time[0]`: Current time in seconds.
  * <small>number</small> `time[1]`: Total duration in seconds.

### Property
* <small>string</small> `setting`  
  The name used in the Source's settings.
* <small>string</small> `name`  
  Human-readable name which will be shown as the property name.
* <small>string</small> `description`  
  Human-readable description which will be shown as the hover text.
* <small>bool</small> `enabled`  
  Whether or not the property is enabled and interactive.
* <small>bool</small> `visible`  
  Whether or not the property is visible.
* <small>string</small> `type`  
  The underlying type of the property which is necessary to understand the actual settings. One of:  
  * ` `: Invalid or Unknown property type.
  * `bool`: Usually a checkbox that has boolean state only.
  * `int`: See [Int Property](#int-property)
  * `float`: See [Float Property](#float-property)
  * `text`: See [Text Property](#text-property)
  * `list`: See [Path Property](#path-property)
  * `color`: A simple color property for 32-bit XRGB color.
  * `color_alpha`: A color property for 32-bit ARGB color.
  * `button`: A clickable button.
  * `font`: Font selection.
  * `path`: See [List Property](#list-property)
  * `editable_list`: See [Editable List Property](#editable-list-property)
  * `framerate`: See [Frame Rate Property](#frame-rate-property)
  * `group`: See [Group Property](#group-property)

#### Int Property
* <small>string</small> `subtype`  
  One of:  
  * `scroller`: An input field which has +/- buttons on the right and can be scrolled.
  * `slider`: A slider.
* <small>object</small> `limits`  
  An object containing:  
  * <small>number</small> `min`: Minimum allowed value.
  * <small>number</small> `max`: Maximum allowed value.
  * <small>number</small> `step`: Required step value.
* <small>string</small> `suffix`  
  Additional text displayed after the number.

#### Float Property
* <small>string</small> `subtype`  
  One of:  
  * `scroller`: An input field which has +/- buttons on the right and can be scrolled.
  * `slider`: A slider.
* <small>object</small> `limits`  
  An object containing:  
  * <small>number</small> `min`: Minimum allowed value.
  * <small>number</small> `max`: Maximum allowed value.
  * <small>number</small> `step`: Required step value.
* <small>string</small> `suffix`  
  Additional text displayed after the number.

#### Text Property
* <small>string</small> `subtype`  
  One of:  
  * `default`: Standard text input.
  * `password`: A password text input.
  * `multiline`: Similar to `default`, but supports multiple lines of text.
* <small>bool</small> `monospace`  
  Whether or not the text field is monospace (Fixed Width).

#### List Property
* <small>string</small> `subtype`  
  One of:  
  * ` `: Unknown or invalid list property.
  * `editable`: An editable string list which the user can edit.
  * `list`: A list of values.
* <small>string</small> `format`  
  One of:  
  * ` `: Unknown or invalid format.
  * `int`: Value is an integer.
  * `float`: Value is a float.
  * `string`: Value is a string.
* <small>array</small> `items`  
  An array of <small>object</small>s containing:
  * <small>string</small> `name`  
    The human-readable name of this item.
  * <small>bool</small> `enabled`  
    Whether or not this item is enable and can be selected.
  * <small>int/float/string</small> `value`  
    The item value as stored in the settings.

#### Path Property
* <small>string</small> `subtype`  
  One of:  
  * `file`: A file selection property for a file to open.
  * `file_save`: A file selection property for a file to save.
  * `directory`: A directory selection property.
* <small>string</small> `filter`  
  If present, defines the file filter used for the `file` and `file_save` subtype.
* <small>string</small> `default_path`  
  If present, defines the default path for the selection dialog.

#### Editable List Property
* <small>string</small> `subtype`  
  One of:  
  * `strings`: A list of strings.
  * `files`: A list of files.
  * `files_and_urls`: A list of files and urls.
* <small>string</small> `filter`  
  If present, defines the file filter used for the `file` and `file_save` subtype.
* <small>string</small> `default_path`  
  If present, defines the default path for the selection dialog.
* <small>array</small> `items`  
  An array of <small>object</small>s containing:  
  * <small>string</small> `name`  
    Human-readable name of this list item.
  * <small>bool</small> `enabled`  
    Whether or not this list item is enabled.
  * <small>string</small> `value`  
    The item value as stored in the settings.

#### Frame Rate Property
* <small>array</small> `options`  
  An array of <small>object</small>s containing:  
  * <small>string</small> `name`  
    Human-readable name displayed in the UI.
  * <small>string</small> `value`  
    Value stored into the settings.
* <small>array</small> `ranges`  
  An array of <small>object</small>s containing:  
  * <small>array</small> `min`  
    Minimum Frame Rate range as an array containing:  
	* <small>int</small> `[0]`: Numerator
	* <small>int</small> `[1]`: Denominator
  * <small>array</small> `min`  
    Maximum Frame Rate range as an array containing:  
	* <small>int</small> `[0]`: Numerator
	* <small>int</small> `[1]`: Denominator

#### Group Property
Groups are a special property that contain multiple other properties.

* <small>string</small> `subtype`  
  One of:  
  * ` `: Invalid or Unknown group type.
  * `default`: A normal group.
  * `checkable`: A group with a checkbox that disables interactivity with all child elements in it.
* <small>[Array(Property)](#Property)</small> `content`  
  An array of [Property](#Property) objects which are (visually) children of this group.
