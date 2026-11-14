# Local audioI2S library fixes

Journal of local patches for later sync with upstream (schreibfaul1 / yoRadio audioI2S).

| ID | Date | Where | What | Why |
|----|------|-------|------|-----|
| RGB-FL-VC1 | 2026-11-14 | `Audio.cpp` `read_FLAC_Header()`, VORBIS_COMMENT loop | Removed `char* album/artist/title`, `strdup`/`strcpy`; tags parsed with `strncasecmp(..., "ALBUM=" / "ARTIST=" / "TITLE=")` into `ps_ptr<char>`; `audio_id3album()` once after the loop as `ARTIST - TITLE` (or leftover single tag); abort the loop if a comment would read past `vendorLength + 3`. | `TITLE=` first left `album == NULL`, then `strcpy(album, " - ")` wrote to NULL (`StoreProhibited` at ~1784). `ALBUMARTIST=` matched `ALBUM`. `strdup` leaked and buffers were too small for artist/title. |
