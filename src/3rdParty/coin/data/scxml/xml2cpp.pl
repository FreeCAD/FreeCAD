#!/usr/bin/perl

if (scalar(@ARGV) != 3) {
    print STDOUT "Usage: $0 <input.xml> <output.cpp> <buffername>\n";
    exit -1;
}

open(XML, "+<", "$ARGV[0]") || die "$0: error opening xml input: $_\n";
open(OUT, "> $ARGV[1]") || die "$0: error opening output for writing: $_\n";

$perline = 12;

$counter = 0;
printf(OUT "static const unsigned char " . $ARGV[2] . "[] = {\n");
$text = "";

while ( !eof(XML) ) {
    $char = getc(XML);
    if (($counter % $perline) == 0) {
        printf(OUT "  ");
    }
    printf(OUT "0x%02x,", ord($char));
    if ($char =~ m/\p{IsPrint}/ && $char !~ m/[\n\r]/) {
      $text = $text . $char;
    } else {
      $text = $text . "?";
    }
    if ((($counter % $perline) == ($perline - 1)) || ($char =~ m/[\n]/)) {
        printf(OUT " // '" . $text . "'\n");
        $text = "";
        $counter = $perline - 1;
    }
    $counter++;
}
if (($counter % $perline) == 0) {
    printf(OUT "  ");
}
printf(OUT "0x00 // '" . $text . "'\n");
printf(OUT "};\n");


close(OUT);
close(XML);
