# obs.scene

## Events / Notifications
### obs.scene.event.item.add
* **{string} scene:** The name of the scene on which an item was added.
* **{string} item:** The name of the item which was added.
* **{bool} visible:** `true` if the item is now visible, `false` if not.

### obs.scene.event.item.remove
* **{string} scene:** The name of the scene on which an item was removed.
* **{string} item:** The name of the item which was removed.

### obs.scene.event.item.visible
* **{string} scene:** The name of the scene on which an item changed visibility.
* **{string} item:** The name of the item which changed visibility.
* **{bool} visible:** `true` if the item is now visible, `false` if not.

## Functions
### obs.scene.enumerate
#### Parameters

#### Returns
* An Array of strings of all scenes.

### obs.scene.items
#### Parameters
* **{string} scene:** The scene for which to enumerate all items.

#### Returns
* An Array of strings of all items in the scene.

### obs.scene.item.visible
#### Parameters
* **{string} scene:** The scene in which the item is.
* **{string} item:** The item to check or change.
* **[{bool} visible]:** `true` to set visible, `false` to set invisible.

#### Returns
* {bool} `true` if the item is visible, otherwise `false`.
