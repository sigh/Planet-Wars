#!/usr/bin/python

import subprocess
import re
import forkmap
import sqlite3
import getopt
import sys

OPTIONS = {
    'max_turns': 200,
    'timeout'  : 500,
    'engine'   : "tools/PlayGame.jar",
    'log_file' : "log.txt",
    'bot_file' : "tournament/bots.txt",
    'db'       : "tournament/db",
    'map_dir'  : "maps",
    'maps'     : ""
}

def run_games(games):
    c = conn.cursor()

    # We want all games for current bot to be rerun
    # TODO: Check timestamp to determine when current has been updated
    clear_results('Current')
    
    results = cached_results(games)

    # get the games that don't have a result
    new_games = [ g for g,r in zip(games, results) if r == None ]

    # run the tournament for the new games only
    new_results = forkmap.map( run_game, new_games )

    # update results in the db
    for ((winner, moves), (args, map, players)) in zip(new_results, new_games):
        c.execute(
            "REPLACE INTO results VALUES ( ?, ?, ?, ?, ?, ? )",
            (
                players[0],
                players[1],
                map,
                OPTIONS['max_turns'],
                winner,
                moves
            )
        )
    conn.commit()

    return cached_results(games)

def clear_results(bot):
    # clear results for bot
    c = conn.cursor()
    c.execute("""
            DELETE FROM results
            WHERE bot_1 == ?
            OR    bot_2 == ?
        """,
        ( bot, bot)
    )
    conn.commit()

def cached_results(games):
    c = conn.cursor()
    
    results = []
    # lookup if we already have result
    for i, (args, map, players) in enumerate(games):
        c.execute("""
            SELECT winner, moves FROM results WHERE
                bot_1 = ?
            AND bot_2 = ?
            AND map   = ?
            AND move_limit = ?
            """,
            (
                players[0],
                players[1],
                map,
                OPTIONS['max_turns']
            )
        )
        results.append(c.fetchone())

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
    args = ['java', '-jar', OPTIONS['engine'], map_path(map_id), str(OPTIONS['max_turns']), str(OPTIONS['max_turns']), OPTIONS['log_file'] ] + players

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
    return "%s/map%d.txt" %(OPTIONS['map_dir'],map_id)

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

def generate_scores(results, games, names):
    scores = empty_score_matrix(len(names))
    lookup = dict((name, i) for i,name in enumerate(names))
    for ((winner, moves), (args, map, players)) in zip(results, games):
        if winner == None:
            continue
        winner = players[winner]
        for loser in (p for p in players if p != winner):
            scores[lookup[winner]][lookup[loser]][0] += 1
            scores[lookup[winner]][lookup[loser]][1] += moves

    return scores

def pretty_print_scores(scores, names):
    output = [[''] + names]

    for i,row in enumerate(scores):
        output_row = [names[i]] 
        row_total = [0,0]
        for j,col in enumerate(row):
            output_row.append( score_string(col) )
            row_total[0] += col[0]
            row_total[1] += col[1]
        output_row.append( score_string( row_total ) )

        output.append(output_row)

    # column totals
    col_totals = ['']
    for i in range(len(names)):
        col_totals.append(score_string((
            sum( s[i][0] for s in scores ),
            sum( s[i][1] for s in scores ),
        )))
    output.append(col_totals)

    width = max( len(x) for row in output for x in row )
    format = "%"+str(width)+"s"
    for row in output:
        for col in row:
            print format %(col),
        print ""

def score_string(score):
    if score[0]:
        return "%d(%d)" %(score[0], score[1]/score[0])
    else:
        return "0(0)"

def get_bots(file):
    f = open(file, 'r')
    lines = f.readlines()
    lines = [ line.split('#')[0] for line in lines ]
    parts = [line.split(None,1) for line in lines]
    names = [ p[0].strip() for p in parts if p ]
    bots = [ p[1].strip() for p in parts if p ]
    return (bots, names)

def parse_args():
    opts, args = getopt.getopt(sys.argv[1:], "", [o+"=" for o in OPTIONS])
    for o, a in opts:
        OPTIONS[o[2:]] = a

def get_maps():
    if not OPTIONS['maps']:
        return all_maps()

    maps = OPTIONS['maps'].split(',')

    result = []
    for map in maps:
        print map
        if '..' in map:
            r = map.split('..')
            result.extend(range(int(r[0]),int(r[1])+1))
        else:
            result.append(int(map))
    return result

def all_maps():
    return range(1,101)

if __name__ == '__main__':
    parse_args()

    global conn
    conn = sqlite3.connect(OPTIONS['db'])

    (bots, names) = get_bots(OPTIONS['bot_file'])
    games = [ generate_command( game, bots, names ) for game in round_robin(get_maps(), bots) ]
    results = run_games(games)
    scores = generate_scores(results, games, names)
    pretty_print_scores(scores, names)
    conn.close()
