#!/usr/bin/perl -w

use strict;
use Image::Magick;

die "Usage: $0 <file-in> <file-out>\n" unless @ARGV == 2;

my $file_in = shift;
my $file_out = shift;

my $bin = "\x00" x (296 * 128 / 8);

my $p = Image::Magick->new;
$p->Read($file_in);

for (my $y=0; $y<296; $y++) {
	for (my $x=0; $x<128; $x++) {
		my @pix = $p->GetPixel(x => $y, y => $x);
		if ($pix[0] <= 0.5) {
			vec($bin, ($x + $y * 128)^7, 1) = 1;
		}
	}
}

open F, '>', $file_out or die;
print F $bin;
close F;
