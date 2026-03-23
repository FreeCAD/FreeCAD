#! /usr/bin/perl -p -i.bak

sub cygpath {
  $path=`CYGWIN= cygpath -w "$_[0]"`;
  chop($path);
  $path;
}

s/\<PATH\>([^ ]*)/&cygpath($1)/eg;
s/^HTML_OUTPUT.*/HTML_OUTPUT = html/;
s/^MAN_OUTPUT.*/MAN_OUTPUT = man/;
