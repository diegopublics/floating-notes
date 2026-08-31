# Floating Notes

[English](README.md) | [Español](README.es.md)

Floating Notes is a small, native desktop utility for keeping local notes close to the edge of the screen. It is designed to stay lightweight, launch quickly, and work entirely offline.

## Run on Windows

The `FloatingNotes.7z` release archive contains the complete portable application, including the executable and its required runtime files.

1. Install [7-Zip](https://www.7-zip.org/) if it is not already available.
2. Extract `FloatingNotes.7z` to `C:\FloatingNotes`, or to any folder you prefer.
3. Open the extracted folder and run `FloatingNotes.exe`.

No installer is required. Keep all extracted files together in the same folder while using the application.

## Current Features

- Floating edge dock on the primary screen.
- Compact note indicator and fan-style note deck.
- Editable notes with automatic local saving.
- SQLite persistence through Qt SQL.
- Note colors, tags, archive, delete, and undo delete.
- Pin mode to keep an open note visible.
- All Notes window with search and filters.
- Import and export for plain text, Markdown, and `.fnotes` files.
- System tray controls, settings, Windows startup integration, and global hotkeys.
- Light, dark, and system theme options.

## Screenshots

### Resting Dock

![Floating Notes resting dock](screenshots/rest_status.png)

### Fan Deck

![Floating Notes fan deck](screenshots/fan_mode.png)

### Note Editor

![Floating Notes note editor](screenshots/open_edit_mode.png)

## Requirements

- C++20 compiler.
- Qt 6.11 or newer with the `Widgets` and `Sql` modules.
- CMake 3.30 or newer.
- Ninja.

The current development target is Windows 10/11 x64 with MinGW. The application uses Qt APIs for its general UI and storage code so that Linux portability remains possible.

## Build

Configure Qt through your Qt Creator kit, environment, or local CMake configuration. The repository intentionally contains no machine-specific Qt paths.

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

The executable is generated at `build/FloatingNotes.exe` on Windows.

## Portable Windows Package

The repository includes a PowerShell script that builds a Release executable and creates a self-contained Windows folder with the required Qt DLLs, SQLite driver, and MinGW runtime.

```powershell
.\scripts\package-portable.ps1 -QtPrefix <path-to-Qt-mingw-kit>
```

For example, `<path-to-Qt-mingw-kit>` is the Qt kit directory containing `bin/windeployqt.exe`.

After the script completes, the ready-to-distribute portable application is in `dist/FloatingNotes/`. Keep the contents of that folder together when copying it to another Windows machine, or compress the folder as a ZIP for distribution. The `dist/` directory is generated locally and is excluded from Git.

### Regenerate `dist/`

1. Open a PowerShell terminal where CMake, Ninja, and the MinGW compiler from the selected Qt kit are available on `PATH`.
2. From the repository root, remove or move the previous `dist/FloatingNotes/` folder. The packaging script deliberately does not overwrite an existing package.
3. Run the packaging script with the Qt MinGW kit path:

```powershell
.\scripts\package-portable.ps1 -QtPrefix <path-to-Qt-mingw-kit>
```

4. Distribute the newly generated `dist/FloatingNotes/` folder as-is, or create a ZIP containing that folder.

## Project Layout

```text
src/
|-- app/          Application lifecycle, settings, import/export
|-- core/         Note data model
|-- persistence/  SQLite storage
|-- platform/     Desktop integration isolated by platform
`-- ui/           Qt Widgets windows and controls
```

## Data

Notes are stored locally using `QStandardPaths::AppDataLocation`. Floating Notes does not require an account, network access, telemetry, or cloud services.

## License

Floating Notes source code is released under the [MIT License](LICENSE).

Qt is a separate dependency and is not covered by this repository's MIT license. When distributing builds, comply with the Qt license selected for the Qt binaries used in that distribution.
