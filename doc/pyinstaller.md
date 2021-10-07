# Generate ist_scraper.exe using pyinstall

* Copy following files `/script/` to a new directory.
    - config.txt
    - chromedriver.exe
    - ist_scraper.py
* `Shift` + `right-click` to open PowerShell.
* Run
```
pyinstaller --onefile ist_scraper.py
```
* Find `ist_scraper.exe` in `dist` and copy to above directory.
* Run `ist_scraper.exe` to test.
    - One file mode takes long time to start, be patient.
