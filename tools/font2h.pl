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

my $p = Image::Magick->new;
$p->Read($file_in // '/dev/stdin');

my $xbox = 8;
my $ybox = 10;

my $cpos = 0;
my @chars;
print "const unsigned char font_8px_data[] = {\n";
for (my $ch=32; $ch<127; $ch++) {
	my $pos_x = ($ch & 15) * $xbox;
	my $pos_y = (($ch >> 4)-2) * $ybox;

	my @n;
	for (my $x=0; $x<8; $x++) {
		my @pix = $p->GetPixel(x => ($pos_x+$x), y => ($pos_y));
		last if $pix[1] == 0;
		my $n=0;
		for (my $y=7; $y>=0; $y--) {
			@pix = $p->GetPixel(x => ($pos_x+$x), y => ($pos_y+$y));
			$n *= 2;
			$n += $pix[0];
		}
		push @n, sprintf('0x%02x', $n);
	}
	print "\t".join(', ', @n).",\n";
	push @chars, [ $cpos, scalar(@n) ];
	$cpos += @n;
}
print "};\n";
print "\n";
print "const unsigned short font_8px[127-32] = {\n";
foreach my $c (@chars) {
	my ($cpos, $clen) = @$c;
	print "\t$cpos + ($clen << 12),\n";
}
print "};\n";
