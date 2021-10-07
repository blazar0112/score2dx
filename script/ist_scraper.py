import json
from selenium import webdriver
import timeit
import time
from enum import Enum
from lxml import etree
from datetime import datetime, timedelta
import os
import pytz
import subprocess
import re

iidx_id_pattern = re.compile('^\d\d\d\d-\d\d\d\d$')
version_pattern = re.compile('\d\d')

class PlayStyle(Enum):
    SP = 0
    DP = 1

chrome_driver_path = 'chromedriver.exe'

test_404_url = 'https://score.iidx.app/users/5483-7390/scores?page=0&q%5Bchart_play_type_status_in%5D%5B%5D=1&q%5Bversion_eq%5D=28'
test_one_page_url = 'https://score.iidx.app/users/5483-7391/scores?q%5Bchart_play_type_status_in%5D%5B%5D=0&q%5Bscore_gteq%5D=1&q%5Bsorts%5D=updated_at+desc&q%5Bversion_eq%5D=28'
test_no_table_row_url = 'https://score.iidx.app/users/5483-7391/scores?q%5Bchart_play_type_status_in%5D%5B%5D=0&q%5Bversion_eq%5D=24'

difficulty_color_mapping = {
    'colorB': 'B',
    '#D6EAF8': 'N',
    '#FCF3CF': 'H',
    '#FADBD8': 'A',
    '#FF33CC': 'L'
}

clear_color_mapping = {
    '#ffffff': 'NO_PLAY',
    '#c0c0c0': 'FAILED',
    '#9595ff': 'ASSIST_CLEAR',
    '#98fb98': 'EASY_CLEAR',
    '#afeeee': 'CLEAR',
    '#ff6347': 'HARD_CLEAR',
    '#ffd900': 'EX_HARD_CLEAR',
    '#ff8c00': 'FULLCOMBO_CLEAR'
}

version_end_datetime = {
    27: '2020-10-27 23:59',
    26: '2019-10-15 23:59',
    25: '2018-11-06 23:59',
    24: '2017-12-20 23:59',
    23: '2016-10-25 23:59',
    22: '2015-11-10 23:29',
    21: '2014-09-16 23:59',
    20: '2013-11-12 23:59',
    19: '2012-09-24 23:59',
    18: '2011-09-14 23:59',
    17: '2010-09-14 23:59'
}

version_names = {
    17: 'SIRIUS',
    18: 'Resort Anthem',
    19: 'Lincle',
    20: 'tricoro',
    21: 'SPADA',
    22: 'PENDUAL',
    23: 'copula',
    24: 'SINOBUZ',
    25: 'CANNON BALLERS',
    26: 'Rootage',
    27: 'HEROIC VERSE',
    28: 'BISTROVER'
}

def generate_url(iidx_id, play_style, version_index, page):
    url = 'https://score.iidx.app/users/'+iidx_id+'/scores?page='+str(page) \
            +'&q%5Bchart_play_type_status_in%5D%5B%5D=' \
            +str(play_style.value)+'&q%5Bversion_eq%5D='+str(version_index)
    return url

def get_date():
    return datetime.now().strftime('%Y-%m-%d')

def get_time():
    return datetime.now().strftime('[%H:%M:%S]')

def get_jst_date_time():
    return datetime.now(tz=pytz.timezone('Asia/Tokyo')).strftime('%Y-%m-%d %H:%M:%S')

def log(message):
    print(get_time(), message)

def setup_chrome_driver_path():
    global chrome_driver_path
    if not os.path.isfile(chrome_driver_path):
        log('not found local chrome driver, use developer path instead.')
        chrome_driver_path = R'D:\install\chromedriver_win32\chromedriver.exe'

def get_chrome_browser_version():
    # only works for windows
    version = ''
    # where /r "C:\Program Files (x86)" chrome.exe
    chrome_browser_path = os.path.join(os.environ['ProgramFiles'], 'Google\Chrome\Application')
    subdirs = [f.name for f in os.scandir(chrome_browser_path) if f.is_dir()]
    for subdir in subdirs:
        if '.' in subdir:
            version = subdir
            break
    return version

def get_chrome_driver_version():
    version = ''
    output = subprocess.check_output([chrome_driver_path, '-v'], encoding='UTF-8')
    tokens = output.split(' ')
    if len(tokens)>2:
        version = tokens[1]
    return version

def check_chrome_driver_version():
    browser_version = get_chrome_browser_version()
    driver_version = get_chrome_driver_version()
    log('chrome browser version: '+browser_version)
    log('chrome driver version: '+driver_version)
    if browser_version[0:2]!=driver_version[0:2]:
        log('version mismatch, please download from:')
        log('    https://chromedriver.chromium.org/downloads')
        log('    and replace the bundled driver in directory.')
        return False

    return True

def load_config():
    """ Return scrap_config = {
            'id': IIDX_ID,
            'scrap_versions': [],
            'scrap_styles': []
        }
        Return None if anything is wrong.
    """

    config_filename = 'config.txt'
    config_path = os.path.join(os.getcwd(), config_filename)

    if not os.path.isfile(config_path):
        log('missing config.txt file.')
        return None

    log('load from config file: '+config_path)

    # example format:
    # id = 5483-7391
    # version = [24, 26, 27, 28]
    # style = [SP, DP]
    config = {}
    with open(config_path, 'r') as file:
        lines = file.readlines()
        for line in lines:
            if line.startswith('id') or line.startswith('version') or line.startswith('style'):
                tokens = line.split('=')
                if len(tokens)!=2:
                    continue
                key = tokens[0].strip()
                value = tokens[1].strip()
                config[key] = value

    expect_keys = ['id', 'version', 'style']
    for expect_key in expect_keys:
        if not expect_key in config:
            log('config for '+expect_key+' is missing.')
            return None

    scrap_config = {}

    if not iidx_id_pattern.match(config['id']):
        log('config id ['+config['id']+'] is not a valid IIDX ID.')
        return None

    scrap_config['id'] = config['id']

    versions = config['version'].replace('[', '').replace(']', '').split(',')
    scrap_versions = [int(v) for v in versions if version_pattern.match(v.strip())]
    if not scrap_versions:
        log('config version list '+config['version']+' result in empty scrap_versions list.')
        return None
    scrap_config['scrap_versions'] = scrap_versions

    styles = config['style'].replace('[', '').replace(']', '').split(',')
    scrap_styles = [s.strip() for s in styles if s.strip()=='SP' or s.strip()=='DP']
    if not scrap_styles:
        log('config style list '+config['style']+' result in empty scrap_styles list.')
        return None
    scrap_config['scrap_styles'] = scrap_styles

    return scrap_config

def scrap_ist():
    start = timeit.default_timer()

    try:
        log('ist_scrap.py: score2dx data scraper for IIDX Score Table(IST).')

        setup_chrome_driver_path()
        if not os.path.isfile(chrome_driver_path):
            log('cannot locate chromedriver at path: '+os.path.abspath(chrome_driver_path))
            return

        log('chromedriver path: '+os.path.abspath(chrome_driver_path))

        if not check_chrome_driver_version():
            return

        config = load_config()
        if not config:
            log('load config error.')
            return

        iidx_id = config['id']
        scrap_styles = [PlayStyle[s] for s in config['scrap_styles']]
        scrap_versions = config['scrap_versions']

        for scrap_version in scrap_versions:
            if scrap_version<17:
                log('not support before version 17.')
                return

        log('scrap IST data of IIDX ID: '+iidx_id
            +', style: '+str(config['scrap_styles'])
            +', ver: '+str(scrap_versions)+'.')

        output_directory = os.path.join(os.getcwd(), 'IST')
        log('output directory: '+output_directory)
        if not os.path.exists(output_directory):
            log('create directory: '+output_directory)
            os.makedirs(output_directory, exist_ok=True)

        player_directory = os.path.join(output_directory, iidx_id)
        if not os.path.exists(player_directory):
            log('create directory: '+player_directory)
            os.makedirs(player_directory, exist_ok=True)

        options = webdriver.ChromeOptions()
        options.add_argument('--headless')
        options.add_experimental_option('excludeSwitches', ['enable-logging'])
        driver = webdriver.Chrome(options=options, executable_path=chrome_driver_path)

        for play_style in scrap_styles:
            style_start = timeit.default_timer()
            for scrap_version in scrap_versions:
                export_start = timeit.default_timer()

                datetime = get_jst_date_time()
                if scrap_version!=28:
                    datetime = version_end_datetime.get(scrap_version, datetime)

                log('+-- scrap version '+str(scrap_version)+' datetime set to '+datetime)

                export = {}

                export['metadata'] = {}
                metadata = export['metadata']
                metadata['id'] = iidx_id
                metadata['playStyle'] = 'SinglePlay'
                if play_style==PlayStyle.DP:
                    metadata['playStyle'] = 'DoublePlay'
                metadata['lastDateTime'] = datetime
                metadata['dateTimeType'] = 'scriptUpdateDate'
                metadata['scoreVersion'] = version_names[scrap_version]

                export['data'] = {}
                data = export['data']

                export_filename = 'score2dx_export_'+play_style.name+'_'+get_date()+'_IST_'+str(scrap_version)+'.json'
                export_filepath = os.path.join(player_directory, export_filename)
                log('+-- export path: '+export_filepath)

                first_page_url = generate_url(iidx_id, play_style, scrap_version, 1)

                # don't request too often
                time.sleep(2)

                log('+-- visit first page: '+first_page_url)
                driver.get(first_page_url)
                first_page_html = etree.HTML(driver.page_source)
                sign_error_elements = first_page_html.xpath("/html/body[@class='sign error-page-wrapper background-color background-image']")
                if sign_error_elements:
                    log('error: page 404, check IIDX ID')
                    return

                last_page = 1

                # '/html/body/section/div/nav[2]/ul/li[7]/a'
                last_page_xpath = '/html/body/section/div/nav[2]/ul/li[7]/a'
                last_page_elements = first_page_html.xpath(last_page_xpath)
                if last_page_elements:
                    last_page_url = last_page_elements[0].get('href')
                    #print('last page url:', last_page_url)
                    last_page_prefix = '/users/'+iidx_id+'/scores?page='
                    page_index = len(last_page_prefix)
                    # https://score.iidx.app/users/5483-7391/scores?page=7&q%5Bchart_play_type_status_in%5D%5B%5D=0&q%5Bversion_eq%5D=28
                    last_page = int(last_page_url[page_index:last_page_url.find('&q')])

                # it seems always has tbody
                tr_xpath = '/html/body/section/div/table/tbody/tr'
                first_tr_elements = first_page_html.xpath(tr_xpath)

                if not first_tr_elements:
                    log('find no '+play_style.name+' data for version '+str(scrap_version)+'.')
                    continue

                for page in range(1, last_page+1):
                    # don't request too often
                    time.sleep(2)

                    page_url = generate_url(iidx_id, play_style, scrap_version, page)
                    log('+--- visit page '+str(page))
                    driver.get(page_url)
                    html = etree.HTML(driver.page_source)

                    tr_elements = html.xpath(tr_xpath)

                    if not tr_elements:
                        log('warning: no data at page '+str(page)+'.')
                        break
                    if len(tr_elements)!=50 and page!=last_page:
                        log('warning: non-last page has data row less than 50 (has '+str(len(tr_elements))+').')

                    for tr in tr_elements:
                        # td_elements = [title, clear, version, level, djlevel, rate, score, diff last ver, miss]
                        td_elements = tr.xpath('.//td')
                        if not td_elements:
                            continue
                        if len(td_elements)!=9:
                            print('found', len(td_elements), 'td row.')

                        title = td_elements[0].xpath('string(./a)')
                        difficulty = difficulty_color_mapping.get(td_elements[0].get('bgcolor'), 'unknown')
                        clear = clear_color_mapping.get(td_elements[1].get('bgcolor'), 'unknown')
                        version = td_elements[2].text
                        djlevel = td_elements[4].text
                        score_str = td_elements[6].text
                        # score format: score(pgreat/great)
                        modified_score_str = score_str.replace('(', ' ').replace(')', '').replace('/', ' ')
                        score, pgreat, great = modified_score_str.split(' ')
                        miss = '---'
                        if score!='0':
                            miss = td_elements[8].text

                        if difficulty=='unknown' or clear=='unknown':
                            print('found unknown entry')
                            print('[', version, '] ', title, ': [', play_style.name, '][', difficulty, '] Clear: ', clear, ', Score: [', djlevel, '] ', score, ' (', pgreat, '/', great, ') Miss: ', miss, sep='')

                        # data[ver][title][datetime][play:0|score][diffAcroym][<chart_score>]
                        data.setdefault(version, {})
                        data[version].setdefault(title, {})
                        data[version][title].setdefault(datetime, {})
                        data[version][title][datetime].setdefault('play', 0)
                        data[version][title][datetime].setdefault('score', {})
                        # chart_score must be newly created, no difficulty should possibly exist.
                        data[version][title][datetime]['score'][difficulty] = []
                        chart_score = data[version][title][datetime]['score'][difficulty]
                        # chart_score: [score, pgreat, great, miss, clear, djLevel]
                        chart_score.append(score)
                        chart_score.append(pgreat)
                        chart_score.append(great)
                        chart_score.append(miss)
                        chart_score.append(clear)
                        chart_score.append(djlevel)

                with open(export_filepath, 'w') as output_file:
                    json.dump(export, output_file)

                log('+-- dump data to export file finished.')

                export_end = timeit.default_timer()
                log('+-- export time: '+str(timedelta(seconds=export_end-export_start)))

            style_end = timeit.default_timer()
            log('+- style all version time: '+str(timedelta(seconds=style_end-style_start)))

        driver.quit()
        end = timeit.default_timer()
        log('total time: '+str(timedelta(seconds=end-start)))
    except:
        print('error')
        driver.quit()
        end = timeit.default_timer()
        log('total time: '+str(timedelta(seconds=end-start)))
        raise

scrap_ist()
