#!/usr/bin/perl -w

use strict;
use Getopt::Std;

sub VERSION_MESSAGE { }
sub HELP_MESSAGE {
	die "Usage: $0 [-i <input>] [-o <output>] [-v <variable-name>] [-w <bytes-per-line>]\n";
}

my %opts;
$Getopt::Std::STANDARD_HELP_VERSION = 1;
getopts('i:o:v:w:', \%opts);

my $file_in = $opts{i};
my $file_out = $opts{o};
my $varname = $opts{v} // die "Variable name not set.\n";
my $num = $opts{w} // 16;
die "bytes per line is not a positive integer.\n" unless $num =~ /\A[1-9]\d*\z/;

my $bin;
if (defined $file_in) {
	open F, '<', $file_in or die;
	$bin = do { local $/; <F> };
	close F;
} else {
	$bin = do { local $/; <STDIN> };
}

my $out = '';
$out .= "const uint8_t $varname\[".length($bin)."\] = {\n";
while ($bin ne '') {
	$out .= "    ".join('', map { "0x$_," } unpack '(H2)*', substr($bin, 0, $num, ''))."\n";
}
$out .= "};\n";

if (defined $file_out) {
	open F, '>', $file_out or die;
	print F $out;
	close F;
} else {
	print $out;
}
