# Documents - Config

## Location of config files

Developers must place the config file place at the location '/loader.cfg'.
By doing this, the loader can set parameters according to your os design.

## Contents of Config Files

Developers must use the following template in the config file.

### Templates

``

kernel=PATH_OF_THE_KERNEL_FILE,
name=ENTRY_NAME,
image=IMAGE_FILE_PATH,
flags=BOOT_FLAGS,

``

#### Explanations of Parameters

##### Required Parameters

- PATH_OF_THE_KERNEL_FILE : Location of kernel file that becomes along rules of EFI file path.
- ENTRY_NAME : Entry name that display on the screen in the selectors.

- IMAGE_FILE_PATH : Location of an image, such as the image that contains the microkernel servers.
If your os doesn't have image file, write "none".  

##### Optioal Parameters

- BOOT_FLAGS : Options of booting that send through kernel main functions. There are rules.  
