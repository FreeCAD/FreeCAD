#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              * 
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

# NOT WORKING - for further implementation

# The forum has a "last posts" feed at
# https://forum.freecadweb.org/feed.php
# Javascript can't fetch it directly, because of cross-domain prohibition
# We can't fetch this from python at StartPAge load, becuase it could take
# several seconds or even fail
# So the idea is to have javascript call this script here, which would
# do it, then find a way to make the result available back to javascript
# a tempfile, for example...

url = "https://forum.freecadweb.org/feed.php"
from xml.etree.ElementTree import parse
xml = parse(urllib.urlopen(url)).getroot()
items = []
channel = xml.find('channel')
for element in channel.findall('item'):
    items.append({'title': element.find('title').text,
                  'description': element.find('description').text,
                  'link': element.find('link').text})
if len(items) > numitems:
    items = items[:numitems]
resp = '<ul>'
for item in items:
    descr = re.compile("style=\".*?\"").sub('',item['description'])
    descr = re.compile("alt=\".*?\"").sub('',descr)
    descr = re.compile("\"").sub('',descr)
    d1 = re.findall("<img.*?>",descr)[0]
    d2 = re.findall("<span>.*?</span>",descr)[0]
    descr = "<h3>" + item['title'] + "</h3>"
    descr += d1 + "<br/>"
    descr += d2
    resp += '<li><a onMouseover="show(\''
    resp += descr
    resp += '\')" onMouseout="show(\'\')" href="'
    resp += item['link']
    resp += '">'
    resp += item['title']
    resp += '</a></li>'
resp += '</ul>'
print(resp)
