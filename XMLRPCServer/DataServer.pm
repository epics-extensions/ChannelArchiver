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

sub time2string($$)
{
    my ($secs, $nano) = @ARG;

    ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
	localtime($secs);
    return sprintf("%02d/%02d/%04d %02d:%02d:%02d.%09ld",
		   $mon+1, $mday, $year + 1900, $hour, $min, $sec, $nano);
}

sub string2time($)
{
    my ($text) = @ARG;
    my ($secs, $nano);
    ($mon, $mday, $year, $hour, $min, $sec, $nano) = split '[/ :.]', $text;
    $secs=timelocal($sec, $min, $hour, $mday, $mon-1, $year-1900);
    return ( $secs, $nano );
}

sub stat2string($)
{
    my ($stat) = @ARG;
    return "" if ($stat == 0);
    return "READ" if ($stat == 1);
    return "WRITE" if ($stat == 2);
    return "HIHI" if ($stat == 3);
    return "HIGH" if ($stat == 4);
    return "LOLO" if ($stat == 5);
    return "LOW" if ($stat == 6);
    return "UDF" if ($stat == 17);
    return "Status $stat";
}

sub sevr2string($)
{
    my ($sevr) = @ARG;
    return "" if ($sevr == 0);
    return "ARCH_DISABLED" if ($sevr == 0x0f08);
    return "ARCH_REPEAT" if ($sevr == 0x0f10);
    return "ARCH_STOPPED" if ($sevr == 0x0f20);
    return "ARCH_DISCONNECT" if ($sevr == 0x0f40);
    return "ARCH_EST_REPEAT" if ($sevr == 0x0f80);
    $sevr = $sevr & 0xF;
    return "MINOR" if ($sevr == 1);
    return "MAJOR" if ($sevr == 2);
    return "INVALID" if ($sevr == 3);
    return "Severity $sevr";
}

sub show_values($)
{
    my ($results) = @ARG;
    my (%meta, $result, $time, $stat, $sevr);

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
	    $stat = stat2string($value->{stat});
	    $sevr = sevr2string($value->{sevr});
	    print("$time @{$value->{value}} $stat $sevr\n");
	}
    }
}

sub show_values_as_sheet($)
{
    my ($results) = @ARG;
    my (%meta, $result, $stat, $sevr, $channels, $vals, $c, $i);
    $channels = $#{$results} + 1;
    return if ($channels <= 0);
    $vals = $#{$results->[0]->{values}} + 1;
    # Dumping the meta information as a spreadsheet comment
    foreach $result ( @{$results} )
    {
	print("# Channel '$result->{name}':\n");
	%meta = %{$result->{meta}};
	if ($meta{type} == 1)
	{
	    print("# Display : $meta{disp_low} ... $meta{disp_high}\n");
	    print("# Alarms  : $meta{alarm_low} ... $meta{alarm_high}\n");
	    print("# Warnings: $meta{warn_low} ... $meta{warn_high}\n");
	    print("# Units   : '$meta{units}', Precision: $meta{prec}\n");
	}
	elsif ($meta{type} == 0)
	{   # Didn't test this case
	    foreach $state ( @{$meta{states}} )
	    {
		print("# State: '$state'\n");
	    }
	}
	print("# Type: $result->{type}, element count $result->{count}.\n");
    }
    # Header: "Time" & channel names
    print("# Time\t");
    for ($c=0; $c<$channels; ++$c)
    {
	if ($results->[$c]->{meta}->{type} == 1)
	{
	    print("$results->[$c]->{name}\t[$results->[$c]->{meta}->{units}]\t");
	}
	else
	{
	    print("$results->[$c]->{name}\t\t");
	}
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
	    $stat = stat2string($results->[$c]->{values}->[$v]->{stat});
	    $sevr = sevr2string($results->[$c]->{values}->[$v]->{sevr});
	    print("@{$results->[$c]->{values}->[$v]->{value}}\t$stat $sevr");
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

