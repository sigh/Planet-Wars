#!/usr/bin/python

import subprocess
import re
import forkmap

MAX_TURNS=1000
PLAY_GAME="tools/PlayGame.jar"
LOG="log.txt"
BOT_FILE="bots.txt"

MAP_PATH = 'maps/map%d.txt'

def run_games(games):
    results = map( run_game, games )
    return results

def run_game(game):
    (args, map_id, players) = game
    p = subprocess.Popen( args, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
    (stdout, stderr) = p.communicate()
    lines = stderr.splitlines()
    match = re.match(r'^Player (\d+) Wins!$', lines[-1]);
    if match:
        winner = int(match.group(1))-1
        moves = len(lines)-1
        print "Map %d: %s => %s (%d moves)" %( map_id, " vs ".join( players ), players[winner], moves )

        return (winner, moves)
    else:
        print "Map %d: %s => Draw" %( map_id, " vs ".join( players ))
        return (None,0)

def generate_command(game, bots, names):
    map_id = game['map']
    players = [ bots[i] for i in game['players'] ]
    args = ['java', '-jar', PLAY_GAME, map_path(map_id), str(MAX_TURNS), str(MAX_TURNS), LOG ] + players

    return (args, map_id, [ names[i] for i in game['players'] ])

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
    games = []
    for i in range(len(players)):
        for j in range(len(players)):
            if i == j:
                continue
            games.extend( play_maps(maps, i, j) )
    return games

def play_maps(maps,*players):
    """generate games to match players on given maps"""
    games = []
    for map in maps:
        games.append({
            'map': map,
            'players': players
        })
    return games

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

def generate_scores(results, games, names):
    scores = empty_score_matrix(len(names))
    lookup = dict((name, i) for i,name in enumerate(names))
    for ((winner, moves), (args, map, players)) in zip(results, games):
        if winner == None:
            continue
        s = score(moves)
        winner = players[winner]
        for loser in (p for p in players if p != winner):
            scores[lookup[winner]][lookup[loser]][0] += 1
            scores[lookup[winner]][lookup[loser]][1] += s

    return scores

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
    games = [ generate_command( game, bots, names ) for game in round_robin(range(1,2), bots) ]
    results = run_games(games)
    scores = generate_scores(results, games, names)
    pretty_print_scores(scores, names)
