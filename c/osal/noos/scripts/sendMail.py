#!/usr/bin/python
# $Id: sendMail.py 18161 2013-02-22 11:24:45Z sfernan $
###############################################################################
# Copyright (c), 2009 - Analog Devices Inc. All Rights Reserved.
# 3 Technology Way, Norwood, MA, 02062, USA
# This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
# and its licensors.
###############################################################################
#
# Title: eMail sender
#
# Description:
#   This script sends the provided message by email
#   
#   
#   
###############################################################################

""" <sendMail.py>
This script sends the provided file message by email
"""

import sys
import os
import smtplib
from simple_wiki_renderer import RenderHTML, RenderText, RenderSVNStatus, RenderTestResults
from optparse import OptionParser

from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

##
# Constant: __version__
# Modify this to reflect the version of this module
#
# Format:
# major.minor.patch
#
__version__ = "1.0.0"


debug = False


DEFAULT_SERVER = "mailhost.analog.com"
DEFAULT_SUBJECT = "SSL Build results"
DEFAULT_FROM = "SSL Build machine <Ant_SSL_Builder@analog.com>"

def send(mail_to, mail_from=DEFAULT_FROM, mail_subject=DEFAULT_SUBJECT, mail_message="{no file provided}", server=DEFAULT_SERVER):
    
    msg = MIMEMultipart('alternative')
    msg['Subject'] = mail_subject
    msg['From'] = mail_from
    msg['To'] = mail_to

    mail_message = RenderSVNStatus(mail_message)
    mail_message = RenderTestResults(mail_message)
    
    messagetext = RenderText(mail_message)
    messagehtml = RenderHTML(mail_message)
    
    part1 = MIMEText(messagetext, 'plain')
    part2 = MIMEText(messagehtml, 'html')
    
    msg.attach(part1)
    msg.attach(part2)

    try:
        if (debug):
            print "Open connection with server '%s'\n"%server
            
        s = smtplib.SMTP(server)
        s.sendmail(mail_from, mail_to, msg.as_string())
        s.quit()
        
    except smtplib.SMTPException, v:
        print "\n[Error] Could not connect to server %s to send the email \n\n[%s]\n"%(server, v)
        
    

if __name__ == "__main__":

    #
    # Main
    #

    if sys.version_info < (2, 5, 0, 0):
        sys.exit("This script requires Python version > 2.5")
        
    print ""
    
    parser = OptionParser(usage = "usage: %prog", version="%prog "+"%s"%(__version__))
    parser.add_option('--smtp_servers', help='Comma-separated list of SMTP servers (optional)', dest='smtpservers', default=DEFAULT_SERVER)
    parser.add_option('-m', '--message_file', help='File with the HTML message text', dest='message_file', default='msg.txt')
    parser.add_option('-s', '--subject', help='Subject of the mail message', dest='subject', default=DEFAULT_SUBJECT)
    parser.add_option('--from', help='Senders Address', dest='sender', default=DEFAULT_FROM)
    parser.add_option('--to', help='List of recipients (e-mail addresses)', dest='recipients', default=None)
    
    (options, args) = parser.parse_args()

    if options.recipients is None:
        print("[Error] no recipient. Please provide the name of the recipient using the '--to' option'\n")
        parser.print_help()
        sys.exit(1)
        
        
    try:
        fd = open(options.message_file, 'r')
        message_text = fd.read()
        fd.close()
    except IOError:
        print("[Error] Could not open file '%s'. Please provide a message file using the option '-m'\n"%options.message_file)
        parser.print_help()
        sys.exit(1)
        
    send(mail_to=options.recipients, mail_from=options.sender, mail_subject=options.subject, mail_message = message_text,
         server=options.smtpservers)
