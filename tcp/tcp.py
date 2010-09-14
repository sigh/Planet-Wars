#!/usr/bin/python

import signal
import subprocess
import sys
import getopt

OPTIONS = {
    'ip': '213.3.30.106',
    'port': '9999',
    'base_name': 'test',
    'name': '',
    'bot_file': './src/MyBot'
}

def run_game():
    p = subprocess.Popen( 
        ['tcp/tcp', OPTIONS['ip'], OPTIONS['port'], OPTIONS['base_name']+OPTIONS['name'], OPTIONS['bot_file']],
        stdout=sys.stdout, 
        stderr=sys.stderr
    )
    p.wait()

def parse_args():
    opts, args = getopt.getopt(sys.argv[1:], "", [o+"=" for o in OPTIONS])
    for o, a in opts:
        OPTIONS[o[2:]] = a

stop = False
def stop_handler(*args):
    global stop
    stop = True

if __name__ == "__main__":
    parse_args()

    signal.signal(signal.SIGINT, stop_handler)

    run_game()
    # while not stop:
    #     run_game()
