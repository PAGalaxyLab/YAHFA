YAHFA demoApp
-------------

Here is a demo app which shows the basic usage of YAHFA.

## Prerequisite

Please build and push the [demoPlugin](https://github.com/rk700/YAHFA/tree/master/demoPlugin) APK to sdcard before running the demo app.

## Usage

Click the button and take a look at the app log.

## What happened

The demo app loads and applies the plugin APK, which hooks the following methods:

- Log.e()
- String.startsWith()
- ClassWithVirtualMethod.tac()
- ClassWithStaticMethod.tac()

After hooking, the arguments would be displayed and then the original method be called.
