#!/usr/bin/octave -q

url='http://localhost/cgi-bin/xmlrpc/ArchiveDataServer.cgi';
key=1;
[ver, desc]=ArchiveData(url, 'info')
[keys,names]=ArchiveData(url, 'archives')
