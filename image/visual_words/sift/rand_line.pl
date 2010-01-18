#!/usr/bin/perl

use strict;
use warnings;

my ($fn, $num) = @ARGV;
if (!$fn || !$num) {
    warn "Usage: $0 txtfile num\n";
    exit 1;
}
open my $fh, $fn or die "cannot open $fn";
while (my $line = <$fh>) {
    next if int(rand($num)) != 0;
    print $line;
}
