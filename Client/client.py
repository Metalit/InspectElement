import socket
import threading

def receive_loop(receive_callback):
    while True:
        try:
            receive_callback(client.recv(1024).decode("utf-8"))
        except OSError:
            break

running = False

def start_client(ip, receive_callback):
    # accessible in stop
    global client, thread, running
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        idx = ip.find(":")
        trunc_ip = ip[:idx]
        port = int(ip[idx+1:])
        client.connect((trunc_ip, port))
    except:
        print("Could not connect with ip", ip)
        return False

    running = True

    thread = threading.Thread(target=receive_loop, args=(receive_callback,))
    thread.start()

    return True

def stop_client():
    if running:
        client.close()
        thread.join()