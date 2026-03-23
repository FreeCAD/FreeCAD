#!/usr/bin/perl

use File::Find;
use File::Basename;

chdir(dirname($0));
chdir('..');

while (chomp($header=<>)) {
    $header=basename($header);
    $cppfile=$header;
    $cppfile=~s/(\.h$)/.cpp/;
    $cppfile=~s/SoVRML//;
    $found=0;
    find(\&wanted, '.');
    if (! $found) {
        print "${cppfile} not found\n";
        exit(1);
    }
}

sub wanted {
    if ($_ eq $cppfile) {
      $found=1;
      local $/;
      open(CPPFILE,"<$_");
      $lines=<CPPFILE>;
      close(CPPFILE);
      if (! ($lines=~/\/*!\s+\\file\s+/)) {
          $lines=~s/(#include\s+[<"].*?${header}[>"])/\/*! \\file ${header} *\/\n$1/g;
          open(CPPFILE,">$_");
          print CPPFILE $lines;
          close(CPPFILE);
      }
    }
}
