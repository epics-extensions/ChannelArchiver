use Frontier::Client;

# Setup URL
$server_url = 'http://bogart.ta53.lanl.gov/cgi-bin/xmlrpc/ArchiveServer.cgi';
$server = Frontier::Client->new(url => $server_url);

# { int32 ver, string desc } = archdat.info()
$result = $server->call('archdat.info');
printf("Archive Data Server V %d,\nDescription '%s'\n\n",
       $result->{'ver'},
       $result->{'desc'});

# string name[] = archdat.get_names(string pattern)
print("Request with pattern:\n");
$result = $server->call('archdat.get_names', "my pattern");
foreach $name ( @{$result} )
{
	print "Channel: '$name'\n";
}

print("\nRequest without pattern:\n");
$result = $server->call('archdat.get_names', "");
foreach $name ( @{$result} )
{
	print "Channel: '$name'\n";
}

