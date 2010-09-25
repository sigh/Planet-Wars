#!/usr/bin/python

import sys
from urllib import urlopen
import re
import getopt

OPTIONS = {
    'url': "http://www.ai-contest.com/visualizer.php",
    'tcp-url': "http://72.44.46.68/canvas",
    'param': 'game_id',
}

BOOLEAN_OPTIONS = [
    'map_only',
    'swap_players',
    'raw',
    'tcp',
]

def game_data(playback_string):
    result = []
    planets, playback = playback_string.split('|')
    planet_data = [ p.split(',') for p in planets.split(':') ]
    for p in planet_data:
        p[2] = map_player(p[2])
        result.append('P '+' '.join(p))

    if 'map_only' in OPTIONS:
        return '\n'.join(result)

    result.append('go')
    num_planets = len(planet_data)
    
    for turn in playback.split(':'):
        parts = turn.split(',')

        # planets
        for i, p in enumerate(parts[:num_planets]):
            planet = list(planet_data[i])
            planet[2], planet[3] = p.split('.')
            planet[2] = map_player(planet[2])
            result.append( 'P ' + ' '.join(planet) )

        # fleets
        for f in parts[num_planets:]:
            fleet = f.split('.')
            fleet[0] = map_player(fleet[0])
            result.append('F '+' '.join(fleet))

        # go
        result.append('go')

    return '\n'.join(result)

def map_player(player):
    if player == '0' or 'swap_players' not in OPTIONS:
        return player
    else:
        return str(3-int(player))

def get_data(game_id):
    page = urlopen(OPTIONS['url'] + '?' + OPTIONS['param'] + '=' + str(game_id)).read()
    match = re.search(r'var data = "([^"]+)"', page)
    if match:
        return dict(item.split('=',2) for item in match.group(1).split('\\n') if item)

def parse_args():
    opts, args = getopt.getopt(sys.argv[1:], "", [o+"=" for o in OPTIONS] + BOOLEAN_OPTIONS)
    for o, a in opts:
        OPTIONS[o[2:]] = a
    return args

if __name__ == "__main__":
    args = parse_args()
    if 'tcp' in OPTIONS:
        OPTIONS['url'] = OPTIONS['tcp-url']

    data = get_data(args[0])

    if 'raw' in OPTIONS:
        print data['playback_string']
    else:
        print game_data(data['playback_string'])
