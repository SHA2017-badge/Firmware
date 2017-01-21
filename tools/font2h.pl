#!/usr/bin/perl -w

use strict;
use Image::Magick;
use Getopt::Std;

sub VERSION_MESSAGE { }
sub HELP_MESSAGE {
	die "Usage: $0 [-h <height>] [-w <width>] [-v <varname>] [-d <defname>] <input> <output>\n"
}

my %opts;
$Getopt::Std::STANDARD_HELP_VERSION = 1;
getopts('d:h:w:v:', \%opts);

my $height = $opts{h} // 8;
my $width = $opts{w} // $height;
my $varname = $opts{v} // "font_".$height."px";
my $defname = $opts{d} // uc($varname);

my $file_in = shift // HELP_MESSAGE();
my $file_out = shift // HELP_MESSAGE();
@ARGV && HELP_MESSAGE();

my $file_h;
if ($file_out ne '-') {
	$file_h = $file_out;
	$file_h =~ s/\A.*\///s;
	$file_h .= '.h';
}

my $height_b = ($height+7) >> 3;
my $ch_first = 32;
my $ch_last = 126;

my $p = Image::Magick->new;
$p->Read($file_in eq '-' ? '/dev/stdin' : $file_in);

my $xbox = $p->Get('width') / 16; # 8;
my $ybox = $p->Get('height') / 6; # 10;

my $cpos = 0;
my @chars;

my $out_h = '';
$out_h .= "#ifndef ".$defname."_H\n";
$out_h .= "#define ".$defname."_H\n";
$out_h .= "\n";
$out_h .= "#include <stdint.h>\n";
$out_h .= "\n";
$out_h .= "#define ".$defname."_FIRST $ch_first\n";
$out_h .= "#define ".$defname."_LAST $ch_last\n";
$out_h .= "#define ".$defname."_WIDTH $width\n";
$out_h .= "#define ".$defname."_HEIGHT $height_b\n";
$out_h .= "\n";

my $out_c = '';
$out_c .= "#include <stdint.h>\n";
$out_c .= "\n#include \"$file_h\"\n" if defined $file_h;
$out_c .= "\n";
$out_c .= "const uint8_t ".$varname."_data[".($width*$height_b*($ch_last+1-$ch_first))."] = {\n";
$out_h .= "extern const uint8_t ".$varname."_data[".($width*$height_b*($ch_last+1-$ch_first))."];\n";
for (my $ch=$ch_first; $ch<=$ch_last; $ch++) {
	my $pos_x = ($ch & 15) * $xbox;
	my $pos_y = (($ch >> 4)-2) * $ybox;

	my @n;
	my $x;
	for ($x=0; $x<=$width; $x++) {
		my @pix = $p->GetPixel(x => ($pos_x+$x), y => ($pos_y));
		last if $pix[1] == 0;
		my $n=0;
		for (my $y=$height-1; $y>=0; $y--) {
			@pix = $p->GetPixel(x => ($pos_x+$x), y => ($pos_y+$y));
			$n *= 2;
			$n += $pix[0];
		}
		my @nn;
		for (my $i=0; $i<$height_b; $i++) {
			push @nn, sprintf('0x%02x', $n & 0xff);
			$n >>= 8;
		}
		push @n, reverse @nn;
	}
	die "character $ch too wide.\n" if @n > $width*$height_b;
	my $indent = ($width - $x) >> 1;
	unshift @n, ('0x00') x ($indent*$height_b);
	push @chars, [ $x, $indent ];
	while (@n < $width*$height_b) { push @n, '0x00' }
	$out_c .= "\t".join(', ', @n).",\n";
}
$out_c .= "};\n";
$out_c .= "\n";
$out_c .= "const uint8_t ".$varname."_width[".($ch_last+1-$ch_first)."] = {\n";
$out_h .= "extern const uint8_t ".$varname."_width[".($ch_last+1-$ch_first)."];\n";
my @n;
foreach my $c (@chars) {
	my ($clen, $indent) = @$c;
	push @n, sprintf('0x%02x', ($indent << 4) | $clen);
	if (@n >= 16) {
		$out_c .= "\t".join(', ', @n).",\n";
		splice @n, 0, 16;
	}
}
$out_c .= "\t".join(', ', @n).",\n" if @n;
$out_c .= "};\n";
$out_h .= "\n";
$out_h .= "#endif // ".$defname."_H\n";

if ($file_out eq '-') {
	print $out_c;
} else {
	open F, '>', $file_out.'.c' or die;
	print F $out_c;
	close F;
	open F, '>', $file_out.'.h' or die;
	print F $out_h;
	close F;
}
