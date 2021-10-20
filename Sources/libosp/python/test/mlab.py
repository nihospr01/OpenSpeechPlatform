import os
import numpy as np
from scipy.io import loadmat, savemat
import plotly.graph_objects as go

freqs = ['250', '354', '500', '707', '1000', '1414', '2000', '2828', '4000', '5657', '8000']

def call_matlab(min_phase, align, inp, g50, g80, kneelow, bmpo, AT, RT):
    try: 
        os.remove('../test/eleven/data_out.mat') 
    except OSError: 
        pass
    savemat('../test/eleven/data.mat', {'min_phase': min_phase, 
                         'align': align,
                         'input': inp,
                         'g50': g50,
                         'g80': g80,
                         'kneelow': kneelow,
                         'Band_MPO': bmpo,
                         'AT': AT,
                         'RT': RT
                        })
    os.system("matlab -nodisplay -nodesktop -logfile test.log -r \"run('../test/eleven/test.m');exit;\"")

    try:
        return loadmat('../test/eleven/data_out.mat')
    except:
        with open('test.log') as f:
            print(f.read())

def test_downsample(inp, min_phase):
    try: 
        os.remove('../test/eleven/data_out.mat') 
    except OSError: 
        pass
    savemat('../test/eleven/data.mat', {'flg_min': min_phase, 'input': inp})
    os.system("matlab -nodisplay -nodesktop -logfile test.log -r \"run('../test/eleven/test_downsampler.m');exit;\"")

    try:
        return loadmat('../test/eleven/data_out.mat')
    except:
        with open('test.log') as f:
            print(f.read())

def test_poly_down(inp):
    try: 
        os.remove('../test/eleven/data_out.mat') 
    except OSError: 
        pass
    savemat('../test/eleven/data.mat', {'input': inp})
    os.system("matlab -nodisplay -nodesktop -logfile test.log -r \"run('../test/eleven/Polyphase_down.m');exit;\"")

    try:
        return loadmat('../test/eleven/data_out.mat')
    except:
        with open('test.log') as f:
            print(f.read())

def test_hilbert(data):
    try: 
        os.remove('../test/eleven/data_out.mat') 
    except OSError: 
        pass
    savemat('../test/eleven/data.mat', {'data': data})
    os.system("matlab -nodisplay -nodesktop -logfile test.log -r \"run('../test/eleven/test_hilbert.m');exit;\"")

    try:
        return loadmat('../test/eleven/data_out.mat')
    except:
        with open('test.log') as f:
            print(f.read())

def test_elevenband(inp, min_phase):
    try: 
        os.remove('../test/eleven/data_out.mat') 
    except OSError: 
        pass
    savemat('../test/eleven/data.mat', {'flg_min': min_phase, 'data': inp})
    os.system("matlab -nodisplay -nodesktop -logfile test.log -r \"run('../test/eleven/test_eleven_band.m');exit;\"")

    try:
        return loadmat('../test/eleven/data_out.mat')
    except:
        with open('test.log') as f:
            print(f.read())

def test_upsample(inp, min_phase, align):
    try: 
        os.remove('../test/eleven/data_out.mat') 
    except OSError: 
        pass
    savemat('../test/eleven/data.mat', {'min_phase': min_phase, 'data': inp, 'align': align})
    os.system("matlab -nodisplay -nodesktop -logfile test.log -r \"run('../test/eleven/test_upsample.m');exit;\"")

    try:
        return loadmat('../test/eleven/data_out.mat')
    except:
        with open('test.log') as f:
            print(f.read())


def plot_response(title, output, bands=None):
    fig = go.Figure()

    # Create x values. 32 samples per millisecond
    x = np.array(range(len(output[0])))/32.0

    if bands is None:
        bands = range(11)

    for i in bands:
        fig.add_trace(go.Scatter(x=x, 
                    y=output[i], 
                    opacity=.5,         
                    name=f'{freqs[i]} Hz'))

    fig.update_layout(title=title,
                    xaxis_title='Time(ms)',
                    yaxis_title='Output',
                    template='plotly_dark')
    fig.show()

def generate_sine_waves(freq_list, duration=1, sample_rate=32000):
    """Generates a signal with multiple sine waves

    Args:
        freq_list: List of frequencies
        duration: signal length in seconds (default 1)
        sample_rate: sample rate in Hz (default 32000)

    Returns:
        (t, y): t is time. y is value in range [-1,1]
    """
    x = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    y = np.zeros(len(x))
    for freq in freq_list:
        frequencies = x * freq
        y += np.sin((2 * np.pi) * frequencies)
    y /= len(freq_list)   # normalize
    return x, y

def plot_fft(title, output):

    ftrans = np.fft.fft(output)/len(output)
    outlen = len(output)
    values = np.arange(int(outlen/2))
    period = outlen/32000
    frequencies = values/period

    fig = go.Figure()
    fig.add_trace(go.Scatter(x=frequencies, 
                  y=np.abs(ftrans)))

    fig.update_layout(title=title,
                    xaxis_title='Frequency',
                    yaxis_title='Amplitude',
                    template='plotly_dark')
    fig.show()
    