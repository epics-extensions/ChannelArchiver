#!/usr/bin/octave -q

url='http://localhost/cgi-bin/xmlrpc/ArchiveDataServer.cgi';
key=1;
[ver, desc]=ArchiveData(url, 'info')
[keys,names]=ArchiveData(url, 'archives')
[names]=ArchiveData(url, 'names', 1, 'IOC')
[names]=ArchiveData(url, 'names', 1, 'IOC.*Load')
