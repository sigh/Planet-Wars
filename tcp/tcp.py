#!/usr/bin/python

import signal
import subprocess
import sys
import getopt

OPTIONS = {
    'ip': '213.3.30.106',
    'port': '9999',
    'name': 'sigh',
    'version': '',
    'bot_file': './src/MyBot'
}

def run_game():
    p = subprocess.Popen( 
        ['tcp/tcp', OPTIONS['ip'], OPTIONS['port'], OPTIONS['name']+'-'+OPTIONS['version'], OPTIONS['bot_file']],
        # stdout=sys.stderr,
        # stderr=sys.stdout
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE
    )
    (stdout, stderr) = p.communicate()
    # p.wait()
    print stdout

def parse_args():
    opts, args = getopt.getopt(sys.argv[1:], "", [o+"=" for o in OPTIONS] + ['once'])
    for o, a in opts:
        OPTIONS[o[2:]] = a
    OPTIONS['once'] = ( OPTIONS.get('once') != None )

stop = False
def stop_handler(*args):
    global stop
    stop = True
    print "Stopping after the current game ends"

if __name__ == "__main__":
    parse_args()

    if OPTIONS['once']:
        run_game()
    else:
        signal.signal(signal.SIGINT, stop_handler)
        while not stop:
            run_game()
