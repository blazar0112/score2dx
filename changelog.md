# Changelog of score2dx

- 5.0.0 [2023-11-04]:
    - Upgrade to IIDX 31.
    - Upgrade DB to 31048.
    - DB format upgrade to 4.0.0.
        - Remove "version" in "info".
    - Adjust clang-tidy config.

- 4.2.0 [2023-10-30]:
    - Update IIDX 30 final DB and prepare to upgrade to 31.

- 4.1.0 [2022-12-12]:
    - Update default DB to count 30058 [2022-12-11].
    - Fix CareedRecord incorrect set other best when best at active version.

- 4.0.0 [2022-12-09]:
    - Update score2dx IIDX data model to IIDX30 RESIDENT.
    - Add UpgradeMusicDatabase to upgrade DB version, no longer use python script.
    - Update default DB to count 30053 [2022-12-09].

- 3.3.0 [2022-11-30]:
    - Still in IIDX 29 mode, fix issues when using against IIDX 30 CSV and ME.
    - Fix loading IIDX30 CSV with 29 DB issues.
        - Title update in IIDX30 CSV
        - Correctly ignore IIDX30 version name.
    - Fix ExportIidxMeData not correctly handle iidx.me version = 30 problem.
    - Update default DB to 2022-11-29 with title fixes.

- 3.2.0 [2022-10-30]:
    - Update default DB to count 29098 [2022-10-19].
    - Change impl CheckChromeDriverVersion depend on popen which is not portable in MSVC.
        - Now use file-based solution.
    - Update cmake to follow ies cmake writing.
    - Update default DB to count 29092 [2022-08-31].

- 3.1.1 [2022-06-08]:
    - Update DB to count 29086 [2022-06-08].
        - Update for Arena 3 and 4 events.

- 3.1.0 [2022-05-30]:
    - Update for ME Json format change: added null data music.
    - Add regression tests.
    - Add ChromeDriver version check interface.
    
- 3.0.0 [2022-05-26]:
    - Redesign data structure.
        - Add VersionScoreTable and CareerRecord as new design instead of BestScoreData.
        - VersionDateTimeRange is now a proper class.
        - Score related class interface update.
    - Result: improve Analyze from 189 to 46ms.
    - Update DB to count 29086 [2022-05-26].

- 2.8.0 [2022-04-02]
    - Improve CSV parse line mechanism:
        - Avoid copy dbTitle.
        - MINGW: 4.109 ms
        - MSVC: 4.623 ms
    - Add test for ToRangeList and try to optimize FindContainingAvailableVersionRange.
        - Result not better or worse.
        - Note: run in VS debug console may be faster due to locking pages.
        - Only count performance run in qtcreator terminal or powershell.
    - Optimize FindScoreLevelDiff:
        - Reduce Import and Analyze time:
        - Import: 270ms -> 162ms.
        - Analyze: 235ms -> 189ms.
    - Improve CSV parse line mechanism:
        - Since CSV files are usually 300KB, read entire file is plausible.
        - MINGW: 4.5ms.
        - MSVC: 5.2ms.
    - Improve CSV parse line mechanism:
        - Rewrite as ParseCsvLine.
        - Improve from 7.8ms to 5.5ms (MINGW).
        - MSVC build is slower (8ms).
    - Improve CSV parse line mechanism:
        - Use array instead of vector.
            - Improve from 11ms to 9ms when loading CSV.
        - Remove unnecessary all date time tracking and only compare csv title mapping.
            - Improve from 9ms to 7.8ms when loading CSV.
    - MusicScore use array to hold ChartScore to reduce allocate time.
        - Improve from 12ms to 11ms when loading CSV.
    - DjLevel check now only enabled in debug mode checkWithDb when construct CSV.
        - Improve about 24ms to 12ms when loading CSV, depend on number of Chart with scores.
    - Update Music DB to count 29078 [2022-03-25].

- 2.7.0 [2022-02-21]:
    - Update dependent library `icl_s2` to rebrand and open-source version `ies`.
    - Update README.
    - Add user-specified music database functionality:
        - User can put music database name in `table/usingDB.txt`, which takes priority than hard-coded default path.
        - Fall back to default database if not exist file listed in `usingDB.txt` or have no such file.
        - For future music-DB-only-update, user can only download new music DB and use this method to reflect.
        - Music-DB-only-update will only increase patch version afterwards, and will not create release.
    - Update Music DB to count 29071 [2022-02-20].
        - Updated structure in `#meta`:
            - `score2dx` value updated to `2.7.0` to denote this structure.
            - Add `lastUpdateNewMusic` to denote last updated new music to ease count new music effort.
            - Add `lastUpdateEvent` to denote last updated event:
                - Event may only add revived music or new leggendaria charts, so no new music count change.

- 2.6.0 [2022-01-05]:
    - Add IIDX.ME support, download and export data using API.
        - Add CURL dependencies.
    - Check with IIDX.ME data and fix Music DB mismatch.
    - Update Music DB to count 29060 [2021-12-26].
        - Add 5 music from event `CastHour SPACE` and 1 from ULTIMATE MOBILE collab.
        - Add 4 music from 2021-12-23 update.

- 2.5.0 [2021-11-28]:
    - Add Activity Analyze.
    - Fix import data miss = 0 for non-FC cases.
    - Fix revival music inherit previous clear mark.
    - Add Music DB meta count check. (Previously called id.)
    - Update Music DB to count 29050 [2021-11-27]
        - Fix revival music available range:
            - `ハリツヤランデヴー`
            - `夕焼け ～Fading Day～`
        - Fix incorrect level:
            - `TOMAHAWK`
        - Add two music from event `WORLD TOURISM jubeat / pop'n music`.

- 2.4.0 [2021-11-17]:
    - Rework ScoreAnalysis to use BestScoreData as class and provide finding following:
        - BestExScore
        - SecondBestExScore
        - BestMiss
        - SecondBestMiss
    - Fix BestScoreData problem:
        - Included trivial score data, now only record non-trivial (has score or miss).
        - Not inherited previous clear type before active version.
        - Fix revival music problem by always use first active version score data.

- 2.3.0 [2021-11-12]:
    - Update MusicDatabase with 2021-11-11 unlocking default LV12 chart info.
    - Use CheckValidity to verify music database and fix several errors.
        - Policy: for early version with hidden another (wiki has note, but level is N/A):
            - Regard it as not available.
            - Only add chart info with both valid level and note.
    - GenerateActiveVersions change version range to [17, 29].
        - Also optimize it from 200ms to 100ms.
    - Handle import invalid data case, ignore NO_PLAY non-available charts.

- 2.2.0 [2021-11-10]:
    - Now detect DJ level and score mismatch when loading data.
        - Analyze also detect such mismatch.
        - Use calculated DJ level in above cases.
        - CSV should not have mismatch, print error message in this case, need check music DB.
    - Update MusicDatabase to 29048.
        - Add musics from event `WORLD TOURISM` (6/6).

- 2.1.0 [2021-11-08]:
    - Add Score Analysis function.
        - Set active version and analyze score at that version.
    - Update MusicDatabase to 29044.
        - Policy: only add music with note info known for each difficulty.
        - So only add two in this case (2/6 event musics).

- 2.0.0 [2021-10-21]:
    - Update for IIDX29 CastHour.
    - Json MusicDatabase change to v2 format.
        - Update Json MusicDatabase to include IIDX29 (42 musics).
        - Fix many incorrect info in Json MusicDatabase.
    - Refactor C++ interface.
        - In C++ `MusicDatabase` is also class now, separated from `Core`.

- 1.0.0 [2021-10-08]:
    - Initial commit with basic functionalities.
        - Load CSV.
        - Import/Export.
        - Calculate ScoreLevel.
