# THE perl script for testing the Archiver's XML-RPC server.

use English;
use Time::Local;
use Frontier::Client;

# Setup URL
$server_url = 'http://localhost/cgi-bin/xmlrpc/ArchiveServer.cgi';
$server_url = 'http://localhost/cgi-bin/xmlrpc/DummyDataServer.cgi';
#$server_url = 'http://bogart.ta53.lanl.gov/cgi-bin/xmlrpc/DummyDataServer1.cgi';
#$server_url = 'http://bogart.ta53.lanl.gov/cgi-bin/xmlrpc/ArchiveDataServer1.cgi';

if ($#ARGV == 0)
{
    $server_url = $ARGV[0];
}

print("==================================================================\n");
print("Connecting to Archive Data Server URL '$server_url'\n");
print("==================================================================\n");
$server = Frontier::Client->new(url => $server_url);

print("==================================================================\n");
print("Info:\n");
print("==================================================================\n");
# { int32 ver, string desc } = archdat.info()
$result = $server->call('archiver.info');
printf("Archive Data Server V %d\n", $result->{'ver'});
printf("Description:\n");
printf("-------------------------------\n");
printf("%s", $result->{'desc'});
printf("-------------------------------\n");


print("==================================================================\n");
print("Archives:\n");
print("==================================================================\n");
$results = $server->call('archiver.archives', "");
foreach $result ( @{$results} )
{
    $key = $result->{key};
    print("Key $key: '$result->{name}' in '$result->{path}'\n");
}

# string name[] = archdat.get_names(string pattern)
if (0)
{
	print("==================================================================\n");
	print("Request without pattern:\n");
	print("==================================================================\n");
	$results = $server->call('archiver.names', $key, "");
	$count = 0;
	foreach $result ( @{$results} )
	{
		$name = $result->{name};
		$start = time2string($result->{start_sec}, $result->{start_nano});
		$end   = time2string($result->{end_sec},   $result->{end_nano});
		print("Channel $name, $start - $end\n");
		++$count;
	}
	print("Altogether $count names\n");
}

print("Using key=$key for the following\n");

print("==================================================================\n");
print("Request with pattern:\n");
print("==================================================================\n");
print("...IOC...\n");
$results = $server->call('archiver.names', $key, "IOC");
foreach $result ( @{$results} )
{
	$name = $result->{name};
	$start = time2string($result->{start_sec}, $result->{start_nano});
	$end   = time2string($result->{end_sec},   $result->{end_nano});
	print("Channel $name, $start - $end\n");
}
print("...fred...\n");
$results = $server->call('archiver.names', $key, "fred");
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
print("==================================================================\n");
print("Get Values:\n");
print("==================================================================\n");
#@names = ( "fred", "freddy", "Jimmy", "James" );
@names = ( "fred" );
#@names = ( "Test_HPRF:IOC1:Load", "Test_HPRF:IOC1:FDAv" );

($start, $startnano) = string2time("01/01/2000 00:00:00.000000000");
($end, $endnano)   = string2time("01/26/2004 17:15:00.000000000");
$count = 25;
$how = 0;
# note: have to pass ref. to the 'names' array,
# otherwise perl will turn it into a sequence of names:
$results = $server->call('archiver.values', $key, \@names,
			 $start, $startnano, $end, $endnano, $count, $how);
show_values($result);

sub time2string($$)
{
    my ($secs, $nano) = @_;

    ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	localtime($secs);
    return sprintf("%02d/%02d/%04d %02d:%02d:%02d.%09ld",
		   $mon+1, $mday, $year + 1900, $hour, $min, $sec, $nano);
}

sub string2time($)
{
    my ($text) = @_;
    my ($secs, $nano);
    ($mon, $mday, $year, $hour, $min, $sec, $nano) = split '[/ :.]', $text;
    $secs=timelocal($sec, $min, $hour, $mday, $mon-1, $year-1900);
    return ( $secs, $nano );
}

sub show_values($)
{
    my ($result) = @_;

    foreach $result ( @{$results} )
    {
	print("Result for channel '$result->{name}':\n");
	%meta = %{$result->{meta}};
	if ($meta{type} == 1)
	{
	    print("Display : $meta{disp_low} ... $meta{disp_high}\n");
	    print("Alarms  : $meta{alarm_low} ... $meta{alarm_high}\n");
	    print("Warnings: $meta{warn_low} ... $meta{warn_high}\n");
	    print("Units   : '$meta{units}', Precision: $meta{prec}\n");
	}
	elsif ($meta{type} == 0)
	{   # Didn't test this case
	    foreach $state ( @{$meta{states}} )
	    {
		print("State: '$state'\n");
	    }
	}

	print("Type: $result->{type}, element count $result->{count}.\n");
	
	foreach $value ( @{$result->{values}} )
	{
	    $time = time2string($value->{secs}, $value->{nano});
	    print("$time ($value->{secs} s) $value->{stat}/$value->{sevr} @{$value->{value}}\n");
	}
    }
}

