#!/usr/bin/env python
# -*- coding: utf-8 -*-
""""
Script Description:
"""
# ADD IMPORTS HERE
# standard l
# 3 party
import network
# local


__author__ = "nm"
__copyright__ = "Copyright 2022, The Current Project"
__credits__ = ["nm", "pg"]
__license__ = "GPL"
__version__ = "1.0.1"
__maintainer__ = "nmtestes"
__email__ = "nm.10000.testes@gmail.com"
__status__ = "Development"


# SOURCE: https://stackoverflow.com/questions/1523427/what-is-the-common-header-format-of-python-files

SSID = 'MEO-E41480'
WIFI_PASSWD = 'your Wi-Fi password'


def connect_to_home_router(ssid, wifi_password):
    # re implement
    ssid = SSID
    wifi_password = WIFI_PASSWD
    station_interface = network.WLAN(network.STA_IF)

    # The station interface is not active by default
    station_interface.active(True)

    station_interface.connect(ssid, wifi_password)

    if station_interface.isconnected():
        pass
    else:
        exit(1)


def make_web_request():
    import urequests
    response = urequests.get("http://xpto.com")
    response_status_code = response.status.code
    response.text()




if __name__ == '__main__':

    connect_to_home_router(SSID, WIFI_PASSWD)
