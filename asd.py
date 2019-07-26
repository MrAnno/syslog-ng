import socket

print(socket.getfqdn(socket.gethostname()))
print(socket.getfqdn())
print(socket.gethostbyname_ex(socket.gethostname()))
