YAHFA
----------------

## Introduction

YAHFA is a hook framework for Android ART. It provides an efficient way for Java method hooking or replacement. Currently it supports

- Android 5.0(API 21)
- Android 5.1(API 22)
- Android 6.0(API 23)
- __EXPERIMENTAL__ Android 7.0(API 24)
- __EXPERIMENTAL__ Android 7.1(API 25)

on either `x86` or `armeabi` platform.

YAHFA is utilized by [VirtualHook](https://github.com/rk700/VirtualHook) so that applications can be hooked without root permission.

Please take a look at this [article](http://rk700.github.io/2017/03/30/YAHFA-introduction/) for a detailed introduction.

## Build

Import and build the project in Android Studio(__with Instant Run disabled__). There are three modules:

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

## Workaround for Method Inlining

Hook would fail for methods that are compiled to be inlined. A simple workaround is to build the APP with debuggable option on, in which case the inlining optimization will not apply. However the option `--debuggable` of `dex2oat` is not available until API 23. So please take a look at machine instructions of the target by `oatdump` when a hook doesn't work.

## Android N Support

Support for Android N(7.0 and 7.1) is experimental and not stable.

## License

YAHFA is distributed under GNU GPL V3.
