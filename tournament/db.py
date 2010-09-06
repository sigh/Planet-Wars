#!/usr/bin/python

import sqlite3

DB_FILE = 'tournament/db'

conn = sqlite3.connect(DB_FILE)

def create():
    c = conn.cursor()
    c.execute("""
        CREATE TABLE results (
            bot_1 text,
            bot_2 text,
            map   integer,
            move_limit integer,
            winner integer,
            moves integer,

            PRIMARY KEY (bot_1, bot_2, map, move_limit)
        )
    """);
    conn.commit()

if __name__ == "__main__":
    create()
    conn.close()
