#!/usr/bin/perl -w

use strict;
use Image::Magick;
use Getopt::Std;

sub VERSION_MESSAGE { }
sub HELP_MESSAGE {
	die "Usage: $0 [-r] [-I] [-i <input>] [-o <output>]\n".
		"\n".
		"  -r  rotate image 180 degrees\n".
		"  -I  invert black/white\n";
}

my %opts;
$Getopt::Std::STANDARD_HELP_VERSION = 1;
getopts('rIi:o:', \%opts);

my $file_in = $opts{i};
my $file_out = $opts{o} // die "output file unknown.\n";

open F, '<', $file_in or die;
my $bin = do { local $/; <F> };
close F;

# rotate
if ($opts{r}) {
	my $orig = $bin;
	for (my $i=0; $i<length($bin)*8; $i++) {
		vec($bin, $i, 1) = vec($orig, length($bin)*8-1-$i, 1);
	}
}
# invert
if ($opts{I}) {
	$bin ^= "\xff" x length($bin);
}

my $p = Image::Magick->new( size => '296x128' );
$p->ReadImage('canvas:black');

for (my $x=0; $x<296; $x++) {
	for (my $y=0; $y<128; $y++) {
		my $bit = vec($bin, ($y + $x * 128)^7, 1);
		$p->Set("pixel[$x,$y]" => 'white') if $bit;
	}
}
$p->Write($file_out);
