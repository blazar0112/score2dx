import json

input_music_table = R'E:\projects\score2dx\table\MusicDatabase28_20210828.json'
output_music_table = R'E:\projects\score2dx\table\MusicDatabase28_20210919.json'

def to_version_string(version):
    return str(version).rjust(2, '0')

with open(input_music_table, encoding='utf-8') as json_file:
    music_database = json.load(json_file)

version = music_database['version']

for version_index, version_musics in version.items():
    version[version_index] = sorted(version_musics)

with open(output_music_table, 'w', encoding='utf-8') as output_file:
    json.dump(music_database, output_file, indent=4, sort_keys=True, ensure_ascii=False)