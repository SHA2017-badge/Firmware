#!/usr/bin/perl -w

use strict;
use Image::Magick;

die "Usage: $0 <file-in> <file-out>\n" unless @ARGV == 2;

my $file_in = shift;
my $file_out = shift;

open F, '<', $file_in or die;
my $bin = do { local $/; <F> };
close F;

my $p = Image::Magick->new( size => '296x128' );
$p->ReadImage('canvas:white');

for (my $x=0; $x<296; $x++) {
	for (my $y=0; $y<128; $y++) {
		my $bit = vec($bin, ($y + $x * 128)^7, 1);
		$p->Set("pixel[$x,$y]" => 'black') if $bit;
	}
}
$p->Write($file_out);
