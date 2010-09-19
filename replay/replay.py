#!/usr/bin/python

import sys
from urllib import urlopen
import re
import getopt

OPTIONS = {
    'url': "http://www.ai-contest.com/visualizer.php",
    'param': 'game_id',
    'fleet_file': 'fleet_commands.txt',
    'player': 1
}

def game_data(playback_string):
    planets, playback = playback_string.split('|')
    planet_data = [ p.split(',') for p in planets.split(':') ]
    map_data = '\n'.join('P '+' '.join(p) for p in planet_data)

    if 'map_only' in OPTIONS:
        return map_data

    result = [map_data, 'go']
    num_planets = len(planet_data)
    
    for turn in playback.split(':'):
        parts = turn.split(',')

        # planets
        for i, p in enumerate(parts[:num_planets]):
            planet = list(planet_data[i])
            planet[2], planet[3] = p.split('.')
            result.append( 'P ' + ' '.join(planet) )

        # fleets
        for f in parts[num_planets:]:
            result.append('F '+' '.join(f.split('.')))

        # go
        result.append('go')

    return '\n'.join(result)

def get_data(game_id):
    page = urlopen(OPTIONS['url'] + '?' + OPTIONS['param'] + '=' + str(game_id)).read()
    match = re.search(r'var data = "([^"]+)"', page)
    if match:
        return dict(item.split('=',2) for item in match.group(1).split('\\n') if item)

def parse_args():
    opts, args = getopt.getopt(sys.argv[1:], "", [o+"=" for o in OPTIONS] + ['map_only', 'swap_players'])
    for o, a in opts:
        OPTIONS[o[2:]] = a
    return args

if __name__ == "__main__":
    args = parse_args()
    data = get_data(args[0])

    print game_data(data['playback_string'])
