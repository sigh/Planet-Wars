#!/usr/bin/python

import subprocess
import re

MAX_TURNS=1000
PLAY_GAME="tools/PlayGame.jar"
LOG="log.txt"
BOT_FILE="bots.txt"

MAP_PATH = 'maps/map%d.txt'

def play_maps(maps,*players):
    """plays players on maps and returns the score"""
    scores = empty_score_matrix(len(players))
    for map in maps:
        (winner, score) = play_map(map, *players)
        if winner != None:
            for loser in (i for i in range(len(players)) if i != winner):
                scores[winner][loser][0] += 1
                scores[winner][loser][1] += score
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

        return (winner, score(moves))
    else:
        print "Map %d: %s => Draw" %( map_id, " vs ".join( players ))
        return (None,0)

def empty_score_matrix(n):
    scores = []
    for i in range(n):
        scores.append([[0,0]]*n)
    return scores

def combine_score_matrix(scores, new, *pos):
    for i1, i2 in enumerate(pos):
        for j1, j2 in enumerate(pos): 
            scores[i2][j2][0] += new[i1][j1][0]
            scores[i2][j2][1] += new[i1][j1][1]
    return scores

def empty_score_matrix(n):
    scores = []
    for i in range(n):
        row = []
        for j in range(n):
            row.append([0,0])
        scores.append(row)
    return scores

def map_path(map_id):
    """Return the file path given a map id"""
    return MAP_PATH %(map_id)

def round_robin(maps, players):
    scores = empty_score_matrix(len(players))
    for i1, p1 in enumerate(players):
        for i2, p2 in enumerate(players):
            if p2 == p1:
                continue
            round_scores = play_maps(maps, p1, p2)
            combine_score_matrix( scores, round_scores, i1, i2 )
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

def pretty_print_scores(scores, names):
    output = [[''] + names]

    for i,row in enumerate(scores):
        output_row = [names[i]] 
        for j,col in enumerate(row):
            output_row.append( "%d(%d)" %(col[0],col[1]) )
        output.append(output_row)

    width = max( len(x) for row in output for x in row )
    format = "%"+str(width)+"s"
    for row in output:
        for col in row:
            print format %(col),
        print ""

def get_bots(file):
    f = open(file, 'r')
    lines = f.readlines()
    parts = [line.split(None,1) for line in lines]
    names = [ p[0].strip() for p in parts if p ]
    bots = [ p[1].strip() for p in parts if p ]
    return (bots, names)

if __name__ == '__main__':
    (bots, names) = get_bots(BOT_FILE)
    scores = round_robin(range(1,2), bots)
    pretty_print_scores(scores, names)
