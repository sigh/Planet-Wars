#!/usr/bin/python

import sys

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

if __name__ == "__main__":
    playback_string = sys.stdin.read()

    data = playback_string.split('|')

    planets = map(parse_planet, data[0].split(':'))

    output_map(planets)
