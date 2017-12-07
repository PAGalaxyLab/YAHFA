YAHFA demoPlugin
----------------

Here is a demo plugin which contains the hook info.

## Usage

Build the plugin and you'll get an APK file. Push the APK to `/sdcard/demoPlugin-debug.apk` before running the [demoApp](https://github.com/rk700/YAHFA/tree/master/demoApp).

## How to write a plugin

To hook a method, please create a `Class` which has the following fields:

- A static `String` field named `className`. This is the name of the class where the target method belongs to
- A static `String` field named `methodName`. This is the name of the target method
- A static `String` field named `methodSig`. This is the signature string of the target method

The above fields are used for finding target method. Besides, the class should have the following methods:

- A __static__ method named `hook`. This is the hook method that would replace the target method. Please make sure that the arguments do match. For example, if you hook a virtual method, then the first argument of the `hook()` should be _this_ `Object`.
- A __static__ method named `backup`. This is the placeholder where the target method would be backed up before hooking. You can invoke `backup()` in `hook()` wherever you like. Since `backup()` is a placeholder, how the method is implemented doesn't matter. Also, `backup()` is optional, which means it's not necessary if the original method would not be called.

Now you have a class which contains all the information for hooking a method. We call this class _hook item_. You can create as many hook items as you like.

After creating all the hook items, put their names in the field `hookItemNames` of class `lab.galaxy.yahfa.HookInfo`. YAHFA would read this field for hook items when loading the plugin.

