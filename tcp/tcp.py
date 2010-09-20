#!/usr/bin/python

import signal
import subprocess
import sys
import getopt
import shutil
import re

OPTIONS = {
    'ip': '72.44.46.68',
    'port': '995',
    'name': 'sigh',
    'version': '',
    'bot_file': './src/MyBot'
}

def run_game():
    p = subprocess.Popen( 
        ['tcp/tcp', OPTIONS['ip'], OPTIONS['port'], OPTIONS['name']+'-'+OPTIONS['version'], OPTIONS['bot_file']],
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE
    )
    (stdout, stderr) = p.communicate()

    print stdout

    result = parse_output(stdout)

    log_name = ( 
        result.get('game_id','') + '-' + 
        result.get('map_id','') + '-' + 
        result.get('opponent','') + '-' + 
        result.get('result','') +
        '.log'
    )
    
    shutil.copy(log_file(), 'tcp/log/' + log_name)

def parse_output(stdout):
    """
    Example: 
        Your map is maps/map31.txt
        Your opponent is B with 92 Elo
        This is game_id=28640
        You WIN against B
    """

    result = {}

    for line in stdout.splitlines():
        # map
        match = re.match(r'^Your map is (\S+)$', line)
        if match:
            result['map'] = match.group(1)
            continue

        # opponent
        match = re.match(r'^Your opponent is (\S+)', line)
        if match:
            result['opponent'] = match.group(1)
            continue

        # game
        match = re.match(r'^This is game_id=(\d+)$', line)
        if match:
            result['game_id'] = match.group(1)
            continue

        # result
        match = re.match(r'^You (\S+) against', line)
        if match:
            result['result'] = match.group(1)
            continue

    # try to determine the map id
    if 'map' in result:
        match = re.match(r'^maps/map(\d+)\.txt$', result['map'])
        if match:
            result['map_id'] = match.group(1)

    return result

def log_file():
    return OPTIONS['bot_file'].split('/')[-1] + ".log"

def parse_args():
    opts, args = getopt.getopt(sys.argv[1:], "", [o+"=" for o in OPTIONS] + ['once'])
    for o, a in opts:
        OPTIONS[o[2:]] = a

stop = False
def stop_handler(*args):
    global stop
    stop = True
    print "Stopping after the current game ends"

if __name__ == "__main__":
    parse_args()

    if 'once' in OPTIONS:
        run_game()
    else:
        signal.signal(signal.SIGINT, stop_handler)
        while not stop:
            run_game()
