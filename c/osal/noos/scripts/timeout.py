#!/usr/bin/python
# $Id: timeout.py 18161 2013-02-22 11:24:45Z sfernan $
###############################################################################
# Copyright (c), 2009-2010 - Analog Devices Inc. All Rights Reserved.
# 3 Technology Way, Norwood, MA, 02062, USA
# This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
# and its licensors.
###############################################################################
#
# Title: Timeout functions
#
# Description:
#   This script allows decorated function to timeout after a given number of seconds
#   it's implemented by running the function given in argument in a separate thread
#   and timing out on joining it.
#
#
""" <timeout.py>
    This script allows decorated function to timeout after a given number of seconds
"""

import sys
import threading


class TimeoutError(Exception): pass

def timed_out(timeout):
    """
        Decorator that gives an execption if the execution of the 
        decorated function takes more than "timeout" seconds.
    """
    def internal(function):
        def internal2(*args, **kw):
            class Calculator(threading.Thread):
                def __init__(self):
                    threading.Thread.__init__(self)
                    self.result = None
                    self.error = None
                
                def run(self):
                    try:
                        self.result = function(*args, **kw)
                    except Exception,v:
                        print "EXC:", v
                        sys.stdout.flush()
                        self.error = sys.exc_info()[0]
            
            c = Calculator()
            c.start()
            c.join(timeout)
            if c.isAlive():
                raise TimeoutError
            if c.error is not None:
                print "CERROR:", c.error
                sys.stdout.flush()
                raise c.error
            return c.result
        return internal2
    return internal
