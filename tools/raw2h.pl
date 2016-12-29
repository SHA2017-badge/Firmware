#!/usr/bin/perl -w

use strict;

die "Usage: $0 <file-in> <file-out> <name>\n" unless @ARGV == 3;

# FIXME: convert to cmdline options
my $invert_xy = 1;
my $invert_bw = 1;

my $file_in = shift;
my $file_out = shift;
my $name = shift;

open F, '<', $file_in or die;
my $bin = do { local $/; <F> };
close F;

# invert (x, y)
if ($invert_xy) {
	my $orig = $bin;
	for (my $i=0; $i<length($bin)*8; $i++) {
		vec($bin, $i, 1) = vec($orig, length($bin)*8-1-$i, 1);
	}
}
# invert (color)
if ($invert_bw) {
	$bin ^= "\xff" x length($bin);
}

my $out = '';
$out .= "const uint8_t $name\[".length($bin)."\] = {\n";
while ($bin ne '') {
	$out .= "    ".join('', map { "0x$_," } unpack '(H2)*', substr($bin, 0, 16, ''))."\n";
}
$out .= "};\n";

open F, '>', $file_out or die;
print F $out;
close F;
