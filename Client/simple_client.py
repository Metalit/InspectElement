import socket
import threading
from sys import stdout

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client.connect(("192.168.7.28", 3306))

def input_loop():
    while True:
        inp = input()
        client.send(inp.encode("utf-8"))

def receive_loop():
    while True:
        receive_message(client.recv(8))

def receive_message(message):
    stdout.write(message.decode("utf-8"))

t1 = threading.Thread(target=input_loop)
t2 = threading.Thread(target=receive_loop)

t1.start()
t2.start()

t1.join()
t2.join()