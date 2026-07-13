# Battle SFX drop-in

Put an audio file here named after a **slot**, then run `python3 tools/mksfx.py`
(from repo root) and rebuild. Any format ffmpeg reads works (wav/mp3/aiff/flac...).
Files are converted to GBA format (8-bit / 13379 Hz mono, capped ~0.6s) and wired
in. Any slot without a file here keeps its synthesized default.

| Slot file        | Plays when                          | Aliases accepted        |
|------------------|-------------------------------------|-------------------------|
| `hit.*`          | an unblocked attack lands (thump)   | `attack.*`              |
| `slash.*`        | attack variety (alternates w/ hit)  | `swing.*`               |
| `clang.*`        | you gain Block / a hit is absorbed  | `block.*`, `shield.*`   |
| `coin.*`         | gold gained / shop purchase         | `gold.*`                |

Good sources: The Sounds Resource GBA rips (https://sounds.spriters-resource.com/game_boy_advance/).
Keep it personal-use — ripped game audio is copyrighted; don't distribute the ROM with it.

Example: drop `block.wav` here, `python3 tools/mksfx.py` → `clang: ... [imported block.wav]`.
