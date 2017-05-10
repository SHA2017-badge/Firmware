#!/usr/bin/perl -w

use strict;
use Image::Magick;
use Getopt::Std;

sub VERSION_MESSAGE { }
sub HELP_MESSAGE {
	die "Usage: $0 [-b] [-w <bytes-per-line>] <input> <varname> <output.c>\n";
}

my %opts = ( w => 16 );
$Getopt::Std::STANDARD_HELP_VERSION = 1;
getopts('bw:', \%opts);

my $file_in = shift // HELP_MESSAGE();
my $varname = shift // HELP_MESSAGE();
my $file_out = shift // HELP_MESSAGE();
@ARGV && HELP_MESSAGE();

my $num = $opts{w};
die "invalid variable name.\n" unless $varname =~ /\A[A-Za-z_][0-9A-Za-z_]*\z/;
die "filename doesn't end with '.c'\n" unless $file_out =~ s/\.c\z//;

my $p = Image::Magick->new;
$p->Read($file_in eq '-' ? '/dev/stdin' : $file_in);

# check dimensions of image
die "Image is not 296x128.\n"
	unless $p->Get("Width") == 296 && $p->Get("Height") == 128;

my $bin = '';
for (my $y=0; $y<128; $y++) {
	for (my $x=0; $x<296; $x++) {
		my @pix = $p->GetPixel(x => $x, y => $y);
		my $c = int($pix[0] * 255 + 0.5);
		$c = 0 if $c < 0;
		$c = 255 if $c > 255;
		if ($opts{b}) {
			vec($bin, $y*296+$x, 1) = $c >> 7;
		} else {
			$bin .= chr($c);
		}
	}
}

my $out_h = '';
$out_h .= '#ifndef '.uc($varname)."_H\n";
$out_h .= '#define '.uc($varname)."_H\n";
$out_h .= "\n";
$out_h .= "#include <stdint.h>\n";
$out_h .= "\n";
$out_h .= "const uint8_t $varname\[".length($bin)."\];\n";
$out_h .= "\n";
$out_h .= '#endif // '.uc($varname)."_H\n";

my $out_c = '';
$out_c .= "#include \"$file_out\.h\"\n";
$out_c .= "\n";
$out_c .= "const uint8_t $varname\[".length($bin)."\] = {\n";
while ($bin ne '') {
	$out_c .= "    ".join('', map { "0x$_," } unpack '(H2)*', substr($bin, 0, $num, ''))."\n";
}
$out_c .= "};\n";

open my $f, '>', $file_out.'.h';
print $f $out_h;
close $f;

open $f, '>', $file_out.'.c';
print $f $out_c;
close $f;

