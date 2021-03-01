YAHFA
----------------

[![Build Status](https://github.com/PAGalaxyLab/YAHFA/workflows/Android%20CI/badge.svg)](https://github.com/PAGalaxyLab/YAHFA/actions)
[![Download](https://badgen.net/github/release/PAGalaxyLab/YAHFA)](https://github.com/PAGalaxyLab/YAHFA/releases/latest/download/library-release.aar)
[![Maven](https://badgen.net/maven/v/maven-central/io.github.pagalaxylab/yahfa)](https://repo1.maven.org/maven2/io/github/pagalaxylab/yahfa/)

## Introduction

YAHFA is a hook framework for Android ART. It provides an efficient way for Java method hooking or replacement. Currently it supports:

- ~~Android 5.0(API 21)~~
- ~~Android 5.1(API 22)~~
- ~~Android 6.0(API 23)~~
- Android 7.0(API 24)
- Android 7.1(API 25)
- Android 8.0(API 26)
- Android 8.1(API 27)
- Android 9(API 28)
- Android 10(API 29)
- Android 11(API 30)
- Android 12(DP1)

(Support for version <= 6.0 is broken after commit [9824bdd](https://github.com/PAGalaxyLab/YAHFA/commit/9824bdd9d958fd0eca43537b6288bb04da191036).)

with ABI:

- x86
- x86_64
- armeabi-v7a
- arm64-v8a

YAHFA is utilized by [VirtualHook](https://github.com/rk700/VirtualHook) so that applications can be hooked without root permission.

Please take a look at this [article](http://rk700.github.io/2017/03/30/YAHFA-introduction/) and this [one](http://rk700.github.io/2017/06/30/hook-on-android-n/) for a detailed introduction.

[更新说明](https://github.com/rk700/YAHFA/wiki/%E6%9B%B4%E6%96%B0%E8%AF%B4%E6%98%8E)

## Setup

Add Maven central repo in `build.gradle`:

```
buildscript {
    repositories {
        mavenCentral()
    }
}

allprojects {
    repositories {
        mavenCentral()
    }
}
```

Then add YAHFA as a dependency:

```
dependencies {
    implementation 'io.github.pagalaxylab:yahfa:0.10.0'
}
```

YAHFA depends on [dlfunc](https://github.com/rk700/dlfunc) after commit [5b60df8](https://github.com/PAGalaxyLab/YAHFA/commit/5b60df8af85fab2b4901cf881c7e9362010c0472) for calling `MakeInitializedClassesVisiblyInitialized` explicitly on Android R, and Android Gradle Plugin version 4.1+ is required for that native library dependency.

## Usage

To hook a method:

```java
HookMain.backupAndHook(Method target, Method hook, Method backup);
```

where `backup` would be a placeholder for invoking the target method. Set `backup` to null or just use `HookMain.hook(Method target, Method hook)` if the original code is not needed.

Both `hook` and `backup` are static methods, and their parameters should match the ones of `target`. Please take a look at [demoPlugin](https://github.com/rk700/YAHFA/tree/master/demoPlugin) on how these methods are defined.

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
