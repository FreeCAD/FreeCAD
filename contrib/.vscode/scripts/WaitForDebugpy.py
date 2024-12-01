import socket
from contextlib import closing
import time

TIMEOUT_TIME_S = 30
RETRY_DELAY_S = 0.1

MAX_ATTEMPTS = TIMEOUT_TIME_S / RETRY_DELAY_S

def check_socket(host, port):
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        sock.settimeout(RETRY_DELAY_S)
        return sock.connect_ex((host, port)) == 0

def main():
    # DO NOT CHECK the actual port 5678/5879 or debugpy will break
    # Check that the other port 39999 used as a flag is manually opened instead
    attempt_counter = 0
    while (not check_socket('localhost', 39999)) and attempt_counter < MAX_ATTEMPTS:
        time.sleep(RETRY_DELAY_S)
        attempt_counter += 1

    if attempt_counter >= MAX_ATTEMPTS:
        exit(1)
    else:
        exit(0)

if __name__ == "__main__":
    main()
