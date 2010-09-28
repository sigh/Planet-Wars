#!/usr/bin/python

import signal
import subprocess
import sys
import getopt
import shutil
import re
import os
import os.path

OPTIONS = {
    'ip': '72.44.46.68',
    'port': '995',
    'name': 'sigh', 
    # This is not an important password
    'password': '1q2w3e4r5t6y7u8i9o0p',
    'version': ''
}

def run_game(bot_command):
    username = OPTIONS['name']+'-'+OPTIONS['version'];
    p = subprocess.Popen( 
        ['tcp/tcp', OPTIONS['ip'], OPTIONS['port'], username, '-p', OPTIONS['password']] + bot_command, 
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

    dir = 'tcp/log/' + OPTIONS['version']

    if not os.path.exists(dir):
        os.makedirs(dir)

    shutil.copy(log_file(bot_command), dir + '/' + log_name)

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

def log_file(bot_command):
    return bot_command[0].split('/')[-1] + ".log"

def parse_args():
    opts, args = getopt.getopt(sys.argv[1:], "", [o+"=" for o in OPTIONS] + ['once'])
    for o, a in opts:
        OPTIONS[o[2:]] = a
    return args

stop = False
def stop_handler(*args):
    global stop
    stop = True
    print "Stopping after the current game ends"

if __name__ == "__main__":
    bot_command = parse_args()

    if 'once' in OPTIONS:
        run_game(bot_command)
    else:
        signal.signal(signal.SIGINT, stop_handler)
        while not stop:
            run_game(bot_command)
