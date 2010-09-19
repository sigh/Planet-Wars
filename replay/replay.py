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

def parse_planet(input):
    input = input.split(',')
    # (x,y,owner,numShips,growthRate)
    return {
        'x': input[0],
        'y': input[1],
        'owner': input[2],
        'ships': input[3],
        'growth': input[4]
    }

def map_data(playback_string):
    playback = playback_string.split('|')
    return '\n'.join(
        "P %s %s %s %s %s" %(p['x'], p['y'], p['owner'], p['ships'], p['growth'])
        for p in map(parse_planet, playback[0].split(':'))
    )

def get_data(game_id):
    page = urlopen(OPTIONS['url'] + '?' + OPTIONS['param'] + '=' + str(game_id)).read()
    match = re.search(r'var data = "([^"]+)"', page)
    if match:
        return dict(item.split('=',2) for item in match.group(1).split('\\n') if item)

def parse_args():
    opts, args = getopt.getopt(sys.argv[1:], "", [o+"=" for o in OPTIONS])
    for o, a in opts:
        OPTIONS[o[2:]] = a
    return args

if __name__ == "__main__":
    args = parse_args()
    data = get_data(args[0])

    map = map_data(data['playback_string'])

    print map
