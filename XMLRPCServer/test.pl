# perl script for testing the Archiver's XML-RPC server.

use DataServer;

# Setup URL
$server_url = 'http://localhost/cgi-bin/xmlrpc/ArchiveDataServer.cgi';
$server_url = 'http://bogart/cgi-bin/xmlrpc/ArchiveDataServer3.cgi';

print("Connecting to Archive Data Server URL '$server_url'\n");
print("==================================================================\n");
$server = Frontier::Client->new(url => $server_url);

print("Info:\n");
print("==================================================================\n");
# { int32 ver, string desc } = archdat.info()
$result = $server->call('archiver.info');
printf("Archive Data Server V %d\n", $result->{'ver'});
printf("Description:\n");
printf("-------------------------------\n");
printf("%s", $result->{'desc'});
printf("-------------------------------\n");

print("Archives:\n");
print("==================================================================\n");
$results = $server->call('archiver.archives', "");
foreach $result ( @{$results} )
{
    $key = $result->{key};
    print("Key $key: '$result->{name}' in '$result->{path}'\n");
}
$key = 2;

print("Channels:\n");
print("==================================================================\n");
if (0)
{
$results = $server->call('archiver.names', $key, "");
foreach $result ( @{$results} )
{
	$name = $result->{name};
	$start = time2string($result->{start_sec}, $result->{start_nano});
	$end   = time2string($result->{end_sec},   $result->{end_nano});
	print("Channel $name, $start - $end\n");
}
}
print("...IOC...\n");
$results = $server->call('archiver.names', $key, "IOC");
foreach $result ( @{$results} )
{
	$name = $result->{name};
	$start = time2string($result->{start_sec}, $result->{start_nano});
	$end   = time2string($result->{end_sec},   $result->{end_nano});
	print("Channel $name, $start - $end\n");
}

# result = archiver.get_values(string name[],
#                              int32 start_sec, int32 start_nano,
#                              int32 end_sec, int32 end_nano, int32 count,
#                              int32 how)
# result = (look at spec, too complex for this comment)
print("Get Values:\n");
print("==================================================================\n");
@names = ( "Test_HPRF:IOC1:Load", "Test_HPRF:IOC1:MemMax" );
($start, $startnano) = string2time("01/02/2003 02:00:00.000000000");
($end, $endnano)   = string2time("01/10/2003 02:06:12.000000000");
$count = 40;
$how = 1;
# note: have to pass ref. to the 'names' array,
# otherwise perl will turn it into a sequence of names:
$results = $server->call('archiver.values', $key, \@names,
			 $start, $startnano, $end, $endnano, $count, $how);
if ($how == 1)
{
    show_values_as_sheet($results);
}
else
{
    show_values($results);
}


