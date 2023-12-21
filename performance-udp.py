#!/usr/bin/python3
import os
import socket
import time
import curses
from curses import wrapper

TARGET_IP = "192.168.1.116"
TARGET_PORT = 1234
LOAD_LIMITS = (0, 8)
DOWN_LIMITS = (0, 8000000)
UP_LIMITS = (0, 3500000)


def send(load, down, up):
    out = ",".join(map(lambda x: str(round(x, 3)), [load, down, up]))
    socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(
        out.encode("utf-8"), (TARGET_IP, TARGET_PORT)
    )


def main(stdscr):
    load = 0.0
    down = 0.0
    up = 0.0

    curses.cbreak()
    curses.noecho()
    stdscr.keypad(True)

    stdscr.addstr(0, 10, "'q' to quit, 'd' for idle load")
    stdscr.addstr(1, 10, "'m' for all zero, 'M' for all maximum")
    stdscr.addstr(2, 10, "arrow keys to change values")
    stdscr.refresh()
    stdscr.nodelay(True)
    curses.init_pair(1, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_WHITE, curses.COLOR_BLACK)

    key = ""
    pulse = True
    position = 0  # 0=load, 1=down, 2=up
    last_send_at = time.time()
    while True:
        now = time.time()
        key = stdscr.getch()
        if key == curses.KEY_UP:
            match position:
                case 0:
                    load += 0.1
                case 1:
                    down += 100000
                case 2:
                    up += 100000
            load = min(load, LOAD_LIMITS[1])
            up = min(up, UP_LIMITS[1])
            down = min(down, DOWN_LIMITS[1])
        elif key == curses.KEY_DOWN:
            match position:
                case 0:
                    load -= 0.1
                case 1:
                    down -= 100000
                case 2:
                    up -= 100000
            load = max(load, LOAD_LIMITS[0])
            up = max(up, UP_LIMITS[0])
            down = max(down, DOWN_LIMITS[0])
        elif key == curses.KEY_LEFT:
            position = max(position - 1, 0)
        elif key == curses.KEY_RIGHT:
            position = min(position + 1, 2)
        elif key == ord("m"):
            load = min(LOAD_LIMITS)
            down = min(DOWN_LIMITS)
            up = min(UP_LIMITS)
        elif key == ord("M"):
            load = max(LOAD_LIMITS)
            down = max(DOWN_LIMITS)
            up = max(UP_LIMITS)
        elif key == ord("d"):
            load = 0.2
            down = 100000
            up = 200000
        elif key == ord("q"):
            break

        l = curses.color_pair(2)
        d = curses.color_pair(2)
        u = curses.color_pair(2)
        match position:
            case 0:
                l = curses.color_pair(1)
            case 1:
                d = curses.color_pair(1)
            case 2:
                u = curses.color_pair(1)

        # stdscr.addstr(3, 0, f"{l}: {load:.3f} | {d}: {down:.0f} | {u}: {up:.0f}")
        if last_send_at + 1 <= now:
            send(load, down, up)
            stdscr.addstr(0, 0, "\u21D2".encode("utf-8"))
            last_send_at = now
            pulse = not pulse
        elif pulse:
            stdscr.addch(0, 0, " ")

        stdscr.addstr(4, 0, f"Load ", l)
        stdscr.addstr(f"{load:.3f} ", curses.color_pair(2))
        stdscr.addstr(f"Down ", d)
        stdscr.addstr(f"{down:.3f} ", curses.color_pair(2))
        stdscr.addstr(f"Up ", u)
        stdscr.addnstr(f"{up:.3f}", curses.color_pair(2))
        stdscr.clrtoeol()

        stdscr.refresh()

    curses.endwin()


wrapper(main)
