YAHFA
----------------

## Introduction

YAHFA is a hook framework for Android ART. It provides an efficient way for method hooking or replacement.

Currently it supports Android 6.0 and 5.1, on either `x86` or `armeabi` platform.

## Build

Import and build the project in Android Studio. There are three modules:

- `library`. This is the YAHFA library module, which compiles to `.aar` for use.
- `demoApp`. This is a demo app which would load and apply the plugin.
- `demoPlugin`. This is a demo plugin which contains the hooks and would be loaded by `demoApp`.

Please refer to [demoApp](https://github.com/rk700/YAHFA/tree/master/demoApp) and [demoPlugin](https://github.com/rk700/YAHFA/tree/master/demoPlugin) for more details on the demo.

## Usage

First please take a look at [demoPlugin](https://github.com/rk700/YAHFA/tree/master/demoPlugin) on how to create a patch plugin.

To apply a patch, create a new `DexClassLoader` which loads the file:

```java
DexClassLoader dexClassLoader = new DexClassLoader("/sdcard/demoPlugin-debug.apk",
            getCodeCacheDir().getAbsolutePath(), null, classLoader);
```

Then initalize `HookMain` and call `doHookDefault()`:

```java
HookMain hookMain = new HookMain();
hookMain.doHookDefault(dexClassLoader, classLoader);
```

You can also omit the default helper and call the following function instead:

```java
public native void findAndBackupAndHook(Class targetClass, String methodName, String methodSig,
                                 Method hook, Method backup);
```

## License

YAHFA is distributed under GNU GPL V3.
