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
my $file_out = $opts{o};

my $bin = "\x00" x (296 * 128 / 8);

my $p = Image::Magick->new;
$p->Read($file_in // '/dev/stdin');

# FIXME: check dimensions of image

for (my $x=0; $x<296; $x++) {
	for (my $y=0; $y<128; $y++) {
		my @pix = $p->GetPixel(x => (295-$x), y => $y);
		if ($pix[0] > 0.5) {
			vec($bin, ($y + $x * 128)^7, 1) = 1;
		}
	}
}

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

if (defined $file_out) {
	open F, '>', $file_out or die;
	print F $bin;
	close F;
} else {
	die "not printing raw data to terminal.\n" if -t STDOUT;
	print $bin;
}
