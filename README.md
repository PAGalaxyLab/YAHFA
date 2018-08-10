YAHFA
----------------

## Introduction

YAHFA is a hook framework for Android ART. It provides an efficient way for Java method hooking or replacement. Currently it supports:

- Android 5.0(API 21)
- Android 5.1(API 22)
- Android 6.0(API 23)
- Android 7.0(API 24)
- Android 7.1(API 25)
- Android 8.0(API 26)
- Android 8.1(API 27)
- Android 9.0(API 28)

with ABI:

- x86
- armeabi-v7a
- arm64-v8a

YAHFA is utilized by [VirtualHook](https://github.com/rk700/VirtualHook) so that applications can be hooked without root permission.

Please take a look at this [article](http://rk700.github.io/2017/03/30/YAHFA-introduction/) and this [one](http://rk700.github.io/2017/06/30/hook-on-android-n/) for a detailed introduction.

[更新说明](https://github.com/rk700/YAHFA/wiki/%E6%9B%B4%E6%96%B0%E8%AF%B4%E6%98%8E)

## Build

Import and build the project in Android Studio(__with Instant Run disabled__). There are three modules:

- `library`. This is the YAHFA library module, which compiles to `.aar` for use.
- `demoApp`. This is a demo app which would load and apply the plugin.
- `demoPlugin`. This is a demo plugin which contains the hooks and would be loaded by `demoApp`.

Please refer to [demoApp](https://github.com/rk700/YAHFA/tree/master/demoApp) and [demoPlugin](https://github.com/rk700/YAHFA/tree/master/demoPlugin) for more details on the demo.

## Usage

First please take a look at [demoPlugin](https://github.com/rk700/YAHFA/tree/master/demoPlugin) on how to create a hook plugin.

To apply hooks, first create a new `DexClassLoader` which loads the plugin file:

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
public void findAndBackupAndHook(Class targetClass, String methodName, String methodSig,
                                 Method hook, Method backup);
```

## Workaround for Method Inlining

Hooking would fail for methods that are compiled to be inlined. For example:

```
0x00004d5a: f24a7e81  movw    lr, #42881
0x00004d5e: f2c73e11  movt    lr, #29457
0x00004d62: f6495040  movw    r0, #40256
0x00004d66: f2c70033  movt    r0, #28723
0x00004d6a: 4641      mov     r1, r8
0x00004d6c: 1c32      mov     r2, r6
0x00004d6e: 47f0      blx     lr
```

Here the value of register `lr` is hardcoded instead of reading from entry point field of `ArtMethod`.

A simple workaround is to build the APP with debuggable option on, in which case the inlining optimization will not apply. However the option `--debuggable` of `dex2oat` is not available until API 23. So please take a look at machine instructions of the target when the hook doesn't work.

## License

YAHFA is distributed under GNU GPL V3.
