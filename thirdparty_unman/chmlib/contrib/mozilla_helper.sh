#!/bin/sh
# redirect to another port on localhost
# this is meant to run as a helper application under mozilla

host=127.0.0.1
port=8081
echo "unpacking :$*: on $host:$port" >/tmp/chm.log
chm_http --port=$port --bind=$host "$*" >chm.log 2>&1 &

HTTP=/tmp/chm2http.html
echo "<html><head><title>$*</title>" >$HTTP
echo "<meta http-equiv='refresh' content=\"5; url=http://$host:$port/\">" >>$HTTP
echo "<title>working</title></head>" >>$HTTP
echo "<body>please wait while I unpack :$*:<br>working...<br><br>" >>$HTTP
echo "chm_http reports:<br>" >>$HTTP
cat chm.log >>$HTTP
echo "</body></html>" >>$HTTP

mozilla -remote "openURL($HTTP)"
rm -f /tmp/chm.log /tmp/chm2http.html
