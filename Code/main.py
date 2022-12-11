#!/usr/bin/env python
# -*- coding: utf-8 -*-
""""
Script Description:
"""
# ADD IMPORTS HERE
# standard l
import time
# 3 party
import machine
import dht
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

def blinking():
    led = machine.Pin(2, machine.Pin.OUT)
    led2 = machine.Pin(16, machine.Pin.OUT)
    while True:
        led.on()
        led2.off()
        time.sleep(0.5)
        led.off()
        led2.on()
        time.sleep(0.5)


def get_temperature_and_humidity():
    d = dht.DHT11(machine.Pin(4))
    d.measure()
    temperature = d.temperature()
    humidity = d.humidity()

    return temperature, humidity


if __name__ == '__main__':

    # app blinking leds
    # blinking()

    # app temperature ...
    t, h = get_temperature_and_humidity()
    # do somethin