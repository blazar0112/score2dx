# Changelog of score2dx

* 2.2.0 [2021-11-10]:
    * Now detect DJ level and score mismatch when loading data.
        * Analyze also detect such mismatch.
        * Use calculated DJ level in above cases.
        * CSV should not have mismatch, print error message in this case, need check music DB.
    * Update MusicDatabase to 29048.
        * Add musics from event `WORLD TOURISM` (6/6).

* 2.1.0 [2021-11-08]:
    * Add Score Analysis function.
        * Set active version and analyze score at that version.
    * Update MusicDatabase to 29044.
        * Policy: only add music with note info known for each difficulty.
        * So only add two in this case (2/6 event musics).

* 2.0.0 [2021-10-21]:
    * Update for IIDX29 CastHour.
    * Json MusicDatabase change to v2 format.
        * Update Json MusicDatabase to include IIDX29 (42 musics).
        * Fix many incorrect info in Json MusicDatabase.
    * Refactor C++ interface.
        * In C++ `MusicDatabase` is also class now, separated from `Core`.

* 1.0.0 [2021-10-08]:
    * Initial commit with basic functionalities.
        * Load CSV.
        * Import/Export.
        * Calculate ScoreLevel.
