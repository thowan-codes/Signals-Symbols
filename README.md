# Signals & Symbols
An accessibility mod for **Voices of the Void**.

Adds `L0`–`L3` text indicators to signal entries, so signal level can be read without relying solely on white, red, yellow, and green UI colors.

[Report a bug or suggest an improvement](https://github.com/thowan-codes/Signals-Symbols/issues)

## Features

- Adds `L0`–`L3` indicators to the computer Database signal list.
- Adds `L0`–`L3` indicators to the workstation Playback Panel.
- Keeps the original signal colors unchanged.
- Visual-only: does not alter signal data, behavior, progression, or add gameplay content.

## Compatibility

- Tested with Voices of the Void: `a090n`
- Requires UE4SS / VotV Shimloader for manual installation.

## Preview

<details>
<summary>Examples</summary>

<img
src="https://raw.githubusercontent.com/thowan-codes/Signals-Symbols/main/Media/1.0.0/Previews/ComputerScreen.png"
alt="Computer database signal list"
width="800">
<img
src="https://raw.githubusercontent.com/thowan-codes/Signals-Symbols/main/Media/1.0.0/Previews/PlaybackPanel.png"
alt="Computer database signal list"
width="800">

</details>

## Installation

Thunderstore Mod Manager installation is recommended.

### Manual installation

<details>
<summary>Install unreal shimloader</summary>

1. Copy `dwmapi.dll` into the `GAME/Binaries/Win64` directory. Its new path should be `GAME/Binaries/Win64/dwmapi.dll`.
2. Copy the contents of the `UE4SS` folder in the package into `GAME/Binaries/Win64`.

`GAME/Binaries/Win64` should now contain the following *new* files and folders:
- `GAME-Win64-Shipping.exe`
- `ue4ss.dll`
- `UE4SS-settings.ini`
- `dwmapi.dll` ← *This is the unreal-shimloader binary. It will load UE4SS for you.*
- `Mods/`
</details>

<details>

<summary>Install Signals & Symbols</summary>

1. Copy `Signals_and_Symbols.pak` from the `pak` folder to `GAME/Content/Paks/LogicMods` directory. 
</details>