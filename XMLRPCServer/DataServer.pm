package DataServer;

use English;
use Time::Local;
use Frontier::Client;
use Data::Dumper;
require Exporter;

@ISA    = qw(Exporter);
@EXPORT = qw(time2string
	     string2time
             show_values
	     show_values_as_sheet);

BEGIN
{
}

# Convert seconds & nanoseconds into string
sub time2string($$)
{
    my ($secs, $nano) = @ARG;

    ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	localtime($secs);
    return sprintf("%02d/%02d/%04d %02d:%02d:%02d.%09ld",
		   $mon+1, $mday, $year + 1900, $hour, $min, $sec, $nano);
}

# Parse (seconds, nanoseconds) from string, couterpart to time2string
sub string2time($)
{
    my ($text) = @ARG;
    my ($secs, $nano);
    ($mon, $mday, $year, $hour, $min, $sec, $nano) = split '[/ :.]', $text;
    $secs=timelocal($sec, $min, $hour, $mday, $mon-1, $year-1900);
    return ( $secs, $nano );
}

# Convert (status, severity) into string
sub stat2string($$)
{
    my ($stat, $sevr, $ss) = @ARG;
 
    return "ARCH_DISABLED" if ($sevr == 0x0f08);
    return "ARCH_REPEAT $stat" if ($sevr == 0x0f10);
    return "ARCH_STOPPED" if ($sevr == 0x0f20);
    return "ARCH_DISCONNECT" if ($sevr == 0x0f40);
    return "ARCH_EST_REPEAT $stat" if ($sevr == 0x0f80);
    $sevr = $sevr & 0xF;

    $ss = "" if ($sevr == 1);
    $ss = "Severity $sevr";
    $ss = "MINOR" if ($sevr == 1);
    $ss = "MAJOR" if ($sevr == 2);
    $ss = "INVALID" if ($sevr == 3);

    return "" if ($stat == 0);
    return "READ/$ss" if ($stat == 1);
    return "WRITE/$ss" if ($stat == 2);
    return "HIHI/$ss" if ($stat == 3);
    return "HIGH/$ss" if ($stat == 4);
    return "LOLO/$ss" if ($stat == 5);
    return "LOW/$ss" if ($stat == 6);
    return "UDF/$ss" if ($stat == 17);
    return "Status $stat/$ss";
}

# local: dump meta info (prefix, meta-hash reference)
sub show_meta($$)
{
    my ($pfx, $meta) = @ARG;
    if ($meta->{type} == 1)
    {
	print($pfx, "Display : $meta->{disp_low} ... $meta->{disp_high}\n");
	print($pfx, "Alarms  : $meta->{alarm_low} ... $meta->{alarm_high}\n");
	print($pfx, "Warnings: $meta->{warn_low} ... $meta->{warn_high}\n");
	print($pfx, "Units   : '$meta->{units}', Precision: $meta->{prec}\n");
    }
    elsif ($meta->{type} == 0)
    {   # Didn't test this case
	foreach my $state ( @{$meta->{states}} )
	{
	    print($pfx, "State: '$state'\n");
	}
    }
}
    
# Dump result of archiver.values()
sub show_values($)
{
    my ($results) = @ARG;
    my (%meta, $result, $time, $stat);

    foreach $result ( @{$results} )
    {
	print("Result for channel '$result->{name}':\n");
	show_meta("", $result->{meta});
	print("Type: $result->{type}, element count $result->{count}.\n");
	
	foreach $value ( @{$result->{values}} )
	{
	    $time = time2string($value->{secs}, $value->{nano});
	    $stat = stat2string($value->{stat}, $value->{sevr});
	    print("$time @{$value->{value}} $stat\n");
	}
    }
}

# Dump result of archiver.values(), works only
# for how = spreadsheet
sub show_values_as_sheet($)
{
    my ($results) = @ARG;
    my (%meta, $result, $stat, $channels, $vals, $c, $i);
    $channels = $#{$results} + 1;
    return if ($channels <= 0);
    $vals = $#{$results->[0]->{values}} + 1;
    # Dumping the meta information as a spreadsheet comment
    foreach $result ( @{$results} )
    {
	print("# Channel '$result->{name}':\n");
	show_meta("# ", $result->{meta});
	print("# Type: $result->{type}, element count $result->{count}.\n");
    }
    # Header: "Time" & channel names
    print("# Time\t");
    for ($c=0; $c<$channels; ++$c)
    {
	print("$results->[$c]->{name}\t");
	print("[$results->[$c]->{meta}->{units}]")
	    if ($results->[$c]->{meta}->{type} == 1);
	print("\t");
    }
    print("\n");
    # Spreadsheet cells
    for ($v=0; $v<$vals; ++$v)
    {
	for ($c=0; $c<$channels; ++$c)
	{
	    if ($c == 0)
	    {
		print(time2string($results->[$c]->{values}->[$v]->{secs},
				  $results->[$c]->{values}->[$v]->{nano}),
		      "\t");
	    }
	    $stat = stat2string($results->[$c]->{values}->[$v]->{stat},
				$results->[$c]->{values}->[$v]->{sevr});
	    print("@{$results->[$c]->{values}->[$v]->{value}}\t$stat");
	    if ($c == $channels-1)
	    {
		print("\n");
	    }
	    else
	    {
		print("\t");
	    }
	}
    }
}

