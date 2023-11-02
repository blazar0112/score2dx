import json

input_db = R'E:\projects\score2dx\table\MusicDatabase31_2023-10-31.json'
output_db = R'E:\projects\score2dx\table\MusicDatabase31_2023-11-03.json'

def delete_info_version(table_json):
    for version_index, version_musics in table_json.items():
        for title, music_data in version_musics.items():
            music_data['info'].pop('version', None)

with open(input_db, encoding='utf-8') as json_file:
    music_database = json.load(json_file)

delete_info_version(music_database['csMusicTable'])
delete_info_version(music_database['musicTable'])

music_database['#meta']['score2dx'] = '4.0.0'

with open(output_db, 'w', encoding='utf-8') as output_file:
    json.dump(music_database, output_file, indent=4, ensure_ascii=False)