#CHIP BL602 Lighting Example

## Build

-   Install some additional tools(likely already present for CHIP developers):

    #Linux \$ sudo apt-get install git libwebkitgtk-1.0-0 ninja-build

    #Mac OS X \$ brew install ninja

*   Build the example application:

    `connectedhomeip$ ./scripts/build/build_examples.py --target bl602-lighting build`

Generated files

         connectedhomeip/out/lighting_app_bl602/chip-bl602-lighting-example.bin

-   To delete generated executable, libraries and object files use:

          $ cd ~/connectedhomeip/
          $ rm -rf out/

## Flash

-   Download [FLASH_TOOL](https://dev.bouffalolab.com/download/).

### Setting up Python Controller

Once BL602 is up and running, we need to set up a device controller to perform
commissioning and cluster control.

-   Set up python controller.

           $ cd {path-to-connectedhomeip}
           $ ./scripts/build_python.sh -m platform

-   Execute the controller.

           $ source ./out/python_env/bin/activate
           $ chip-device-ctrl

### Commissioning over BLE

-   Establish the secure session over BLE. BLE is the default mode in the
    application and is configurable through menuconfig.

         - chip-device-ctrl > ble-scan
         - chip-device-ctrl > set-pairing-wifi-credential TESTSSID P455W4RD
         - chip-device-ctrl > connect -ble 3904 20202021 135246

         Parameters:
         1. Discriminator: 3904
         2. Setup-pin-code: 20202021
         3. Node ID: Optional.
            If not passed in this command, then it is auto-generated by the controller and displayed in the output of connect.
            The same value should be used in the next commands.
            We have chosen a random node ID which is 135246.
         4. TESTSSID : Wi-Fi SSID
         5. P455W4RD : Wi-Fi Password

### Cluster control

-   After successful commissioning, use the OnOff cluster commands to control
    the OnOff attribute. This allows you to toggle a parameter implemented by
    the device to be On or Off.

    `chip-device-ctrl > zcl OnOff On 135246 1 1`
    `chip-device-ctrl > zcl OnOff Off 135246 1 1`

-   Use the LevelControl cluster commands to control the CurrentLevel attribute.
    This allows you to control the brightness of the led.

    `chip-device-ctrl > zcl LevelControl MoveToLevel 135246 1 1 level=10 transitionTime=0 optionMask=0 optionOverride=0`

-   Use the ColorControl cluster commands to control the CurrentHue and
    CurrentSaturation attribute.

    `chip-device-ctrl > zcl ColorControl MoveToHue 135246 1 1 hue=100 direction=0 transitionTime=0 optionsMask=0 optionsOverride=0`
    `chip-device-ctrl > zcl ColorControl MoveToSaturation 135245 1 1 saturation=200 transitionTime=0 optionsMask=0 optionsOverride=0`