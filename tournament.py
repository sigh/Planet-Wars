#!/usr/bin/python

import subprocess
import re

MAX_TURNS=1000
PLAY_GAME="tools/PlayGame.jar"
LOG="log.txt"

EXAMPLE_BOTS=[
    'java -jar example_bots/RandomBot.jar',
    'java -jar example_bots/DualBot.jar',
    'java -jar example_bots/BullyBot.jar',
    'java -jar example_bots/ProspectorBot.jar',
    'java -jar example_bots/RageBot.jar'
]

MAP_PATH = 'maps/map%d.txt'

def play_maps(maps,*players):
    """plays players on maps and returns the score"""
    scores = dict((p,0) for p in players)
    for map in maps:
        (winner, score) = play_map(map, *players)
        if winner:
            scores[winner] += score
    return scores

def play_map(map_id, *players):
    """plays players on map and returns the winner and the score"""
    p = subprocess.Popen(['java', '-jar', PLAY_GAME, map_path(map_id), str(MAX_TURNS), str(MAX_TURNS), LOG ] + list(players), stdout=subprocess.PIPE, stderr=subprocess.PIPE )
    (stdout, stderr) = p.communicate()
    lines = stderr.splitlines()
    match = re.match(r'^Player (\d+) Wins!$', lines[-1]);
    if match:
        winner = int(match.group(1))-1
        moves = len(lines)-1
        print "Map %d: %s => %s (%d moves)" %( map_id, " vs ".join( players ), players[winner], moves )

        return (players[winner], score(moves))
    else:
        return (None,0)

def map_path(map_id):
    """Return the file path given a map id"""
    return MAP_PATH %(map_id)

def round_robin(maps, players):
    scores = dict((p,0) for p in players)
    for p1 in players:
        for p2 in players:
            if p2 == p1:
                continue
            round_scores = play_maps(maps, p1, p2)
            scores[p1] += round_scores[p1]
            scores[p2] += round_scores[p2]
    return scores

def score(turns):
    """determine the score based on the length of the game"""
    if turns >= MAX_TURNS:
        return 1
    elif turns >= 500:
        return 2
    elif turns >= 200:
        return 3 
    elif turns >= 100:
        return 4 
    else:
        return 5

if __name__ == '__main__':
    print round_robin(range(1,4), ['./bot/MyBot'] + EXAMPLE_BOTS)
