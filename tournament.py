#!/usr/bin/python

import subprocess
import re

MAX_TURNS=1000
PLAY_GAME="tools/PlayGame.jar"
LOG="log.txt"

# plays p1 and p2 on map and returns the number of the winner
def play_map(map, *players):
    p = subprocess.Popen(['java', '-jar', PLAY_GAME, map, str(MAX_TURNS), str(MAX_TURNS), LOG ] + list(players), stdout=subprocess.PIPE, stderr=subprocess.PIPE )
    (stdout, stderr) = p.communicate()
    last_line = stderr.splitlines()[-1]
    match = re.match(r'^Player (\d+) Wins!$', last_line);
    if match:
        return int(match.group(1))
    else:
        return None

if __name__ == '__main__':
    print play_map("maps/map1.txt", './bot/MyBot', "java -jar example_bots/DualBot.jar")
