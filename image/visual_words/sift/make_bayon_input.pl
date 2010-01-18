#!/usr/bin/perl

use strict;
use warnings;

my ($dirname, $siftmappath) = @ARGV;
if (!$dirname || !$siftmappath) {
    warn "Usage: $0 dir siftmap\n";
    exit(1);
}
open my $siftmapfh, ">$siftmappath" or die "cannot open $siftmappath";

my $fh;
my $cnt = 0;
foreach my $fn (glob "$dirname/*.sift") {
    open $fh, $fn;
    if (!$fh) {
        warn "cannot open $fn. skip it\n";
        next;
    }
    my @siftids;
    my @vector;
    while (my $line = <$fh>) {
        chomp $line;
        if ($line =~ /^\s/) {
            map { push @vector, $_ if $_ ne '' } split /\s/, $line;
        }
        elsif (@vector) {
            if (scalar @vector != 128) {
                warn "dim of vector != 128\n";
                next;
            }
            print $cnt;
            for (my $i = 0; $i < 128; $i++) {
                printf "\t%d\t%d", $i, $vector[$i];
            }
            print "\n";
            push @siftids, $cnt;
            $cnt++;
            @vector = ();
        }
    }
    close $fh;
    printf $siftmapfh "%s\t%s\n", $fn, join("\t", @siftids);
}
