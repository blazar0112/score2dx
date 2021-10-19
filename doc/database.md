* Assume unchecked music version from debut version to last available.
* Note from remy wiki: 
    * beatmania IIDX difficulty rated from 1 to 7+ in 6th through 9th style
    * 1 to 8 in 10th style, 1 to 8+ in 11 IIDX RED
   * and 1 to 12 from 12 HAPPY SKY onwards.
* List all chart level and note change.
* Music is cs only: list in csMusicTable.
* If a chart of music is cs only: use label like "cs04".
* If a chart of music is both ac and cs available: ignore cs part, only list ac available versions.
* If a chart is changed but notes is unchanged: ignore this case, all chart variation with same level and notes is regard as same chart.
    * It's too much work to trace chart change.
* Ignore chart "upgrade" difficulty, like from previous Hyper to renewed Another.
    * Treat each difficulty as independent history.
* Database format change: score2dx v1.0.0->v2.0.0:
    * [#meta][version]: latest->29
    * add [#meta][score2dx]: "2.0.0"
    * [csMusicTable][ver][title][availableVersion] -> change to single string comma separated, instead of array.
    * [csMusicTable][ver][title][difficulty][latest] -> change latest to cs(ver). e.g. cs04
    * [musicTable][ver][title][availableVersion] -> change to single string comma separated, instead of array.
    * [mMusicTable][ver][title][difficulty][latest] -> change latest to availableVersionString.
    * difficulty available version can be a special label: "unknown" for unchecked version history.
    * Fix many incorrect info in db.
    
