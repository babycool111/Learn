#!/usr/bin/python
# $Id: simple_wiki_renderer.py 18161 2013-02-22 11:24:45Z sfernan $
###############################################################################
# Copyright (c), 2009 - Analog Devices Inc. All Rights Reserved.
# 3 Technology Way, Norwood, MA, 02062, USA
# This software is PROPRIETARY & CONFIDENTIAL to Analog Devices, Inc.
# and its licensors.
###############################################################################
#
# Title: Simple Wiki Rederer
#
# Description:
#   This script renders the given string using the following Wiki syntax:
#
#  Headings:
#     ==Section==
#     ===Subsection===
#     ====Subsubsection====
#
#  Basic formating:
#     ''italic''
#     '''bold'''
#     '''''bolditalic'''''
#     
#     empty line --> new Paragraph
#     -----      --> horizontal line
#     
#  Colors:
#     <red> ... </red>      --> colors the included text
#     <blue> ... </blue>
#     
#     valid colors are: red, blue, green, yellow, black, white, pink, purple
#       
#  Links:
#     [http://somelink description]   --> HTML link (html mode) or just link text (text mode)
#       
#  
#  
#  Modes supported:
#     HTML and Text 
#   
#     in text mode:
#       * all basic formatting tags are removed except the horizontal line
#       * link contain only the link URL text
#       * all HTML tags are removed (including colors)
#   
#   
#   
###############################################################################

""" <simple_wiki_renderer.py>
   This script renders the given string using the following Wiki syntax:

  Headings:
     ==Section==
     ===Subsection===
     ====Subsubsection====

  Basic formating:
     ''italic''
     '''bold'''
     '''''bolditalic'''''
     
     empty line --> new Paragraph
     -----      --> horizontal line
     
  Colors:
     <red> ... </red>      --> colors the included text
     <blue> ... </blue>
     
     valid colors are: red, blue, green, yellow, black, white, pink, purple
       
  Links:
     [http://somelink description]   --> HTML link (html mode) or just link text (text mode)
       
  
  
  Modes supported:
     HTML and Text 
   
     in text mode:
       * all basic formatting tags are removed except the horizontal line
       * link contain only the link URL text
       * all HTML tags are removed (including colors)
"""

import re

retitleh4  = re.compile(r'====(.*)====')
retitleh3  = re.compile(r'===(.*)===')
retitleh2  = re.compile(r'==(.*)==')
recolor    = re.compile(r'<(red|blue|green|yellow|black|white|pink|purple)>(.*)</(red|blue|green|yellow|black|white|pink|purple)>')
reitalic   = re.compile(r"''(.*)''")
rebold     = re.compile(r"'''(.*)'''")
relinks    = re.compile(r'\[(https?://\S*)\s(.*)\]')
rehtmltags = re.compile(r'</?\w+((\s+\w+(\s*=\s*(?:".*?"|\'.*?\'|[^\'">\s]+))?)+\s*|\s*)/?>')
reparagraph = re.compile(r'^$', re.M)
reseparator = re.compile(r'^-----+$', re.M)

fsp        = r"&nbsp; &nbsp; &nbsp; &nbsp;"


def RenderSVNStatus(message):
    reSVNStatAdd      = re.compile(r"^A(.*)", re.M)
    reSVNStatDel      = re.compile(r"^D(.*)", re.M)
    reSVNStatUp       = re.compile(r"^U(.*)", re.M)
    reSVNStatReplace  = re.compile(r"^R(.*)", re.M)
    reSVNStatMerge    = re.compile(r"^G(.*)", re.M)
    reSVNStatConflict = re.compile(r"^C(.*)", re.M)

    rmsg = message.replace(r'    ', fsp)

    rmsg = reSVNStatAdd.sub(r'<green>A</green>\1', rmsg)
    rmsg = reSVNStatConflict.sub(r'<red>C</red>\1', rmsg)
    rmsg = reSVNStatDel.sub(r'<red>D</red>\1', rmsg)
    rmsg = reSVNStatMerge.sub(r'<blue>G</blue>\1', rmsg)
    rmsg = reSVNStatReplace.sub(r'<blue>R</blue>\1', rmsg)
    rmsg = reSVNStatUp.sub(r'<blue>U</blue>\1', rmsg)
    
    return rmsg
    
        
def RenderTestResults(message):
    reTestsPass     = re.compile(r"(\[PASS\])(.*)$", re.M)
    reTestsFail     = re.compile(r"(\[FAIL\])(.*)$", re.M)
    reTestsError    = re.compile(r"(\[Error\])(.*)$", re.M)
                    
    rmsg = reTestsError.sub(r'<red>\1 \2</red>', message)
    rmsg = reTestsFail.sub(r"<red>\1</red>'''\2'''", rmsg)
    rmsg = reTestsPass.sub(r'<green>\1</green> \2', rmsg)                    

    return rmsg


def RenderHTML(message):
    
    #converts 'some' wiki syntax to HTML, poor man's renderering
    
    rmsg = """\
    <html>
      <head></head>
      <body>"""
    rmsg += message
    rmsg += """\
      </body>
    </html>"""

    # order of rendering is important here! (otherwise things like '===' becomes '<h2>=' instead of '<h3>'
    rmsg = retitleh4.sub(r'<font color="#000099"><h4>\1</h4></font>', rmsg)
    rmsg = retitleh3.sub(r'<font color="#000099"><h3>\1</h3></font>', rmsg)
    rmsg = retitleh2.sub(r'<font color="#000099"><h2>\1</h2></font>', rmsg)
    rmsg = recolor.sub(r'<font color="\1">\2</font>', rmsg)
    rmsg = rebold.sub(r'<b>\1</b>', rmsg)    
    rmsg = reitalic.sub(r'<i>\1</i>', rmsg)
    rmsg = relinks.sub(r'<a href="\1">\2</a>', rmsg)
    rmsg = reparagraph.sub(r'<p>', rmsg)
    rmsg = reseparator.sub(r'<hr>', rmsg)
    rmsg = rmsg.replace('\n','<br>\n')

    return rmsg
    
def RenderText(message):
    # order of rendering is important here! (otherwise things like '===' becomes '<h2>=' instead of '<h3>'
    # removes all html tags
    message = rehtmltags.sub(r'', message)
    message = rebold.sub(r'\1', message)
    message = reitalic.sub(r'\1', message)
    message = relinks.sub(r'\1 (\2)', message)
    
    return message