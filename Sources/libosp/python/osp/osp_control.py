import socket
import select
import json
import numpy as np
import plotly.graph_objects as go
from IPython.display import Image, display
from builtins import Exception

class OSPNotRunningException(Exception):
    def __str__(self):
        return f"Could not connect to the osp process on the given hostname/port"

class OSPJsonException(Exception):
    def __str__(self):
        return f"JSON message failed"


class OspControl:

    def __init__(self, hostname='localhost', port=8001):
        """OspControl

        Creates an object to control a running OSP (RTMHA) session.

        Parameters:
            hostname (str, optional. The hostname or IP to use.  Defaults to 'localhost'.
            port (int, optional): The port to use. Defaults to 8001.
        """
        self.hostname = hostname
        self.port = port

    def send(self, message):
        """ sends a JSON message to OSP (RTMHA)

        Parameters
        ----------
        message: dictionary, string or bytestring
            The message to send to RTMHA
        hostname: string
            hostname or IP address [localhost]
        port: integer
            port number to connect to [8001]

        Returns:
            JSON containing the parameters- for 'GET' command
            {"Result" : "success"} - if successful
            {"Result" : "FAILED"} if the JSON message was not recognized
        Raises:
            OSPNotRunningException: no OSP process was running on the host at the port
            OSPJsonException: something else went wrong
        """

        if type(message) == dict:
            message = json.dumps(message)
        if type(message) == str:
            message = message.encode()

        # Create a TCP/IP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Connect the socket to the port where the server is listening
        server_address = (self.hostname, self.port)

        try:
            sock.connect(server_address)
        except ConnectionRefusedError:
            raise OSPNotRunningException

        sent = False
        data = ""
        while True:
            readable, writable, exceptional = select.select([sock], [sock], [sock])
            if sock in writable and not sent:
                sock.sendall(message)
                sent = True
            if sock in readable:
                chunk = sock.recv(8096)
                if chunk:
                    data += chunk.decode()
                else:
                    sock.close()
                    if data == 'success' or data == '':
                        data = '{"Result": "success"}'
                    if data == 'FAILED':
                        data = '{"Result": "FAILED"}'
                    try:
                        return json.loads(data)
                    except:
                        raise OSPJsonException
            if sock in exceptional:
                sock.close()
                raise OSPJsonException

    def send_chan(self, sdict, channel=None):
        """ Set RTMHA channel parameters from a python dictionary

        Parameters
        ----------
        sdict: python dictionary
            A dictionary containing names and values to send to RTMHA
        channel: string
            Which channel(s) to set
            "left, "right", or None (for both channels)
    
        Returns:
            JSON containing the parameters- for 'GET' command
            {"Result" : "success"} - if successful
            {"Result" : "FAILED"} if the JSON message was not recognized
        Raises:
            OSPNotRunningException: no OSP process was running on the host at the port
            OSPJsonException: something else went wrong
        """
        send_data = {'method': 'set', 'data': {}}
        if channel ==  'left' or channel is None:
            send_data['data']['left'] = sdict
        if channel ==  'right' or channel is None:
            send_data['data']['right'] = sdict
        return self.send(send_data)

def plot_audio(title, out1, lab1, out2, lab2, rate=48000, name=None):
    fig = go.Figure()

    # Create x values.
    x = np.array(range(len(out1)))/rate

    fig.add_trace(go.Scatter(x=x,
                             y=out1,
                             name=lab1,
                             opacity=.5))
    fig.add_trace(go.Scatter(x=x,
                             y=out2,
                             name=lab2,
                             opacity=.5))
    fig.update_layout(title=title,
                      xaxis_title='Time(sec)',
                      yaxis_title='Amplitude',
                      template='plotly_white')
    if name is None:
        fig.show()
    else:
        fig.write_image(name, engine="kaleido")
        display(Image(name, width=4096))

def plot_mono(title, out1, lab1, rate=48000, name=None):
    fig = go.Figure()

    # Create x values.
    x = np.array(range(len(out1)))/rate

    fig.add_trace(go.Scatter(x=x,
                             y=out1,
                             name=lab1,
                             opacity=1))
    fig.update_layout(title=title,
                      xaxis_title='Time(sec)',
                      yaxis_title='Amplitude',
                      template='plotly_white')
    if name is None:
        fig.show()
    else:
        fig.write_image(name, engine="kaleido")
        display(Image(name, width=4096))
