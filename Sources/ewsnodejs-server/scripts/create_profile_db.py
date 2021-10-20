#!/usr/bin/env python3

"""
Populate an initial profile key-value store. 

Requires ewsnodejs-server running on port 5000

Creates sqlite table 'profiles'
"""
import requests
import base64
import json

def create_profile_table():
    URL = "http://localhost:5000/api/db/table/create/profiles"
    res = requests.post(URL)
    if res.status_code != 200 and res.status_code != 404:
        print(res.text)
        return False
    return True

def load_config(filename, profilename):
    with open(filename, 'r') as f:
        config_json = json.loads(f.read())

    config_json = json.dumps(config_json)

    URL = "http://localhost:5000/api/db/profiles"
    headers = {"Content-Type": "application/json"}
    body = json.dumps({"key": profilename, "value": config_json})
    res = requests.post(URL, headers=headers, data=body)
    assert res.status_code == 200
    assert res.text == ""

def print_keys():
    print("Installed profiles:")
    print(requests.get("http://localhost:5000/api/db/profiles").text)


requests.delete("http://localhost:5000/api/db/table/delete/profiles")

if __name__ == '__main__':
    create_profile_table()
    for (pname, fname) in [('normal', 'config.json'), 
                           ('normal-10', 'config10.json'), 
                        #    ('normal-11', 'config11.json'),
                           ('marty', 'marty6.json'),
                           ('marty-10', 'marty10.json')]:
        load_config(fname, pname)

    print_keys()

"""
~/osp/ewsnodejs-server/scripts ❯❯❯ ipython                                                                                                                               release/Marty
Python 3.8.10 (default, Jun  4 2021, 15:09:15) 
Type 'copyright', 'credits' or 'license' for more information
IPython 7.22.0 -- An enhanced Interactive Python. Type '?' for help.

In [1]: import requests

In [2]: requests.get("http://localhost:5000/api/db/profiles").text
Out[2]: '["normal","normal-10","normal-11"]'

In [3]: requests.get("http://localhost:5000/api/db/profiles/normal").text
Out[3]: '{"num_bands": 6, "left": {"afc": 1, "afc_delay": 4.6875, "afc_mu": 0.004999999888241291, "afc_reset": 0, "afc_rho": 0.8999999761581421, "afc_type": 3, "alpha": 0.0, "amc_forgetting_factor": 0.800000011920929, "amc_thr": 2.0, "attack": [5.0, 5.0, 5.0, 5.0, 5.0, 5.0], "bf": 0, "bf_alpha": 0.0, "bf_amc_on_off": 0, "bf_beta": 150.0, "bf_c": 0.0010000000474974513, "bf_delay_len": 160, "bf_delta": 9.999999974752427e-07, "bf_fir_length": 319, "bf_mu": 0.009999999776482582, "bf_nc_on_off": 0, "bf_imu_on_off": 0, "bf_p": 1.2999999523162842, "bf_power_estimate": 0.0, "bf_rho": 0.9850000143051147, "bf_type": 3, "en_ha": 0, "freping": 0, "freping_alpha": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0], "g50": [9.5, 4.5, 15, 22.5, 32, 18], "g80": [-9.5, -14.5, -5, 2.5, 14, 2], "gain": 0, "global_mpo": 120.0, "knee_low": [45.0, 45.0, 45.0, 45.0, 45.0, 45.0], "mpo_band": [110, 110, 110, 110, 110, 110], "nc_thr": 1.0, "noise_estimation_type": 0, "release": [20.0, 20.0, 20.0, 20.0, 20.0, 20.0], "cal_mic": [23.3, 25.8, 17.5, 15.8, 15.7, 15.7], "cal_rec": [0.5, 1.0, 0.5, 1.0, -7.0, 4.5], "cal_mpo": [0.5, 1.0, 0.5, 1.0, -7.0, 4.5], "spectral_subtraction": 0.0, "spectral_type": 0}, "right": {"afc": 1, "afc_delay": 4.6875, "afc_mu": 0.004999999888241291, "afc_reset": 0, "afc_rho": 0.8999999761581421, "afc_type": 3, "alpha": 0.0, "attack": [5.0, 5.0, 5.0, 5.0, 5.0, 5.0], "en_ha": 0, "freping": 0, "freping_alpha": [0.0, 0.0, 0.0, 0.0, 0.0, 0.0], "g50": [14.5, 9.5, 17, 22.5, 34, 21], "g80": [-4.5, -9.5, -3, 2.5, 16, 4], "gain": 0, "global_mpo": 120.0, "knee_low": [45.0, 45.0, 45.0, 45.0, 45.0, 45.0], "mpo_band": [110, 110, 110, 110, 110, 110], "cal_mic": [23.3, 25.8, 17.5, 15.8, 15.7, 15.7], "cal_rec": [0.5, 1.0, 0.5, 1.0, -7.0, 4.5], "cal_mpo": [0.5, 1.0, 0.5, 1.0, -7.0, 4.5], "noise_estimation_type": 0, "release": [20.0, 20.0, 20.0, 20.0, 20.0, 20.0], "spectral_subtraction": 0.0, "spectral_type": 0}}'
"""
