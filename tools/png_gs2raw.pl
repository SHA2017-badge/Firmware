#!/usr/bin/perl -w

use strict;
use Image::Magick;
use Getopt::Std;

sub VERSION_MESSAGE { }
sub HELP_MESSAGE {
	die "Usage: $0 <input> <output>\n";
}

my %opts;
$Getopt::Std::STANDARD_HELP_VERSION = 1;
getopts('', \%opts);

my $file_in = shift // HELP_MESSAGE();
my $file_out = shift // HELP_MESSAGE();
@ARGV && HELP_MESSAGE();

my $bin = '';

my $p = Image::Magick->new;
$p->Read($file_in eq '-' ? '/dev/stdin' : $file_in);

# FIXME: check dimensions of image

for (my $x=0; $x<296; $x++) {
	for (my $y=127; $y>=0; $y--) {
		my @pix = $p->GetPixel(x => $x, y => $y);
		my $c = int($pix[0] * 255 + 0.5);
		$c = 0 if $c < 0;
		$c = 255 if $c > 255;
		$bin .= chr($c);
	}
}

if ($file_out ne '-') {
	open F, '>', $file_out or die;
	print F $bin;
	close F;
} else {
	die "not printing raw data to terminal.\n" if -t STDOUT;
	print $bin;
}
