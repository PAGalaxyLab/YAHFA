YAHFA
----------------

## Introduction

YAHFA is a hook framework for Android ART. It provides an efficient way for method hooking or replacement.

Currently it supports Android 6.0 and Android 5.1, on either x86 or armeabi platform.

## Build

Import the project in Android Studio and make the module named `library`. 

## Demo

The project comes with a demo, which contains an application and a patch plugin. Please refer to [demoApp](https://github.com/rk700/YAHFA/) and [demoPlugin](https://github.com/rk700/YAHFA) for more details.

## Usage

First please take a look at demoPlugin on how to write a patch plugin.

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
