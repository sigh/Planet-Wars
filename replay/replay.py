#!/usr/bin/python

import sys
from urllib import urlopen
import re

URL_BASE="http://www.ai-contest.com/visualizer.php?game_id="

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

def output_map(planets):
    for p in planets:
        print "P %s %s %s %s %s" %(p['x'], p['y'], p['owner'], p['ships'], p['growth'])

def get_data(game_id):
    page = urlopen(URL_BASE + str(game_id)).read()
    match = re.search(r'var data = "([^"]+)"', page)
    if not match:
        return
        
    data = {}
    for item in match.group(1).split('\\n'):
        parts = item.split('=',2)
        if len(parts) == 2:
            data[parts[0]] = parts[1]

    return data

if __name__ == "__main__":
    data = get_data(sys.argv[1])

    playback_string = data['playback_string']
    playback = playback_string.split('|')

    planets = map(parse_planet, playback[0].split(':'))

    output_map(planets)
