# CoolReader 3 — Apple Silicon / macOS fork

This fork keeps upstream CoolReader 3 as the reference and adds a native Apple
Silicon build plus direct support for Apple Books' expanded EPUB folders.

Upstream project: <https://github.com/buggins/coolreader>

## What is included

- Native arm64 Qt 6 build and a deployable `cr3.app` bundle.
- Working EPUB 3 import, including navigation documents, page lists, NCX
  fallback, CSS/images, metadata, and font obfuscation handling.
- Direct File → Open support for Apple Books `.epub` directories on macOS.
- A repeatable build script and an automated EPUB 3 / bundle test suite.

## Build and run

Requirements:

- macOS on Apple Silicon
- Xcode command line tools
- Homebrew Qt 6 and dependencies:
  `qt@6 zlib libpng jpeg-turbo freetype harfbuzz fribidi fontconfig utf8proc zstd ninja`
- `macdeployqt` from the Qt 6 installation

```sh
./build.sh
open dist/cr3.app
```

The script configures CMake for arm64, builds, assembles the app bundle,
deploys Qt frameworks and plugins, rewrites Homebrew rpaths, and ad-hoc signs
the bundle.

`build.sh` expects the repository layout used in this fork/top-level project:
the CMake sources are in `upstream/`, and the built bundle is written to
`dist/cr3.app`.

## Apple Books EPUB folders

Apple Books stores downloaded books as directories named `Book.epub`, with the
EPUB already unpacked inside. CoolReader's engine reads ZIP-based EPUBs, so
this fork bridges the two formats transparently.

On macOS, File → Open uses `NSOpenPanel` with directory selection enabled, so
an expanded `Book.epub` folder can be selected like any normal book. When one
is opened, CoolReader:

1. Verifies that `META-INF/container.xml` exists.
2. Packs the folder into a spec-correct ZIP EPUB under `~/.cr3/applebooks`.
   The `mimetype` entry is written first and stored uncompressed.
3. Skips Finder metadata such as `.DS_Store` and `._*` files.
4. Reuses the cached ZIP until the source folder changes.
5. Keeps the original Apple Books path in history, recent books, and position
   restore through CREngine's original-filename hook.

Nothing is written into the Apple Books library, and the user does not need to
run a conversion script. If a book still shows a download/cloud icon in Apple
Books, let Apple Books finish downloading it before opening it here.

DRM-protected Apple Books are still not readable. CoolReader shows the
standard unsupported-DRM warning when an `encryption.xml` contains an
unsupported scheme. The reflowable EPUB 3 feature set is covered by the test
suite; JavaScript, Media Overlays/audio, and complex fixed-layout publications
remain outside the supported scope.

## Why these changes were needed

1. **Qt 6 / arm64** — the legacy Qt 4 macOS path is x86-only and its bundled
   dependencies do not build for Apple Silicon. Qt 6 is now the default; the
   old path remains opt-in with `-DCR3_LEGACY_MAC_QT=ON`.
2. **Image headers** — CMake could pick stale PNG/JPEG headers from a system
   Mono framework instead of the Homebrew libraries actually being linked.
   Header search paths are now aligned with the selected libraries.
3. **libunibreak** — Homebrew libunibreak 8 removed an API CREngine uses, so a
   compatible libunibreak 7.0 is bundled and built locally.
4. **CREngine allocator** — the custom allocator was disabled on macOS desktop
   builds because it misbehaved with optimized string handling on Apple
   Silicon.
5. **App bundle** — Qt 6 no longer supplies the old Qt 4 menu nib. The nib,
   icon, and plist come from `packaging/macos`, followed by `macdeployqt`, rpath
   normalization, and ad-hoc codesigning.
6. **EPUB encryption attributes** — `encryption.xml` URI and algorithm
   attributes were parsed but discarded, causing unsupported DRM to be ignored.
   They are now retained so the standard DRM warning appears while Adobe RC4
   font obfuscation continues to work.
7. **Apple Books containers** — expanded EPUBs are not ZIP archives. The Qt
   layer now packages them into a stable cache while preserving the original
   source identity for reader history.

## Verification

From the top-level project:

```sh
file dist/cr3.app/Contents/MacOS/cr3
dist/cr3.app/Contents/MacOS/cr3 unittest
python3 tests/test_epub3.py
codesign --verify --verbose=2 dist/cr3.app
```

The test suite checks native architecture, startup, EPUB 2 regressions, EPUB 3
navigation and metadata, page maps, NCX fallback, DRM warnings, font
obfuscation, and the Apple Books expanded-directory path.

## License

The original CoolReader source remains under its existing GPL license. This
fork's changes are distributed under the same license.
