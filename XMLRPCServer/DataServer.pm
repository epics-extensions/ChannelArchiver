package DataServer;

use English;
use Time::Local;
use Frontier::Client;
require Exporter;

@ISA    = qw(Exporter);
@EXPORT = qw(time2string
	     string2time
             show_values);

BEGIN
{
}

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
    my ($results) = @_;

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
	    print("$time $value->{stat}/$value->{sevr} @{$value->{value}}\n");
	}
    }
}

