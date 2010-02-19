#!/usr/bin/perl

use strict;
use warnings;

use constant {
    MAX_VWORD => 5,
};

my ($siftmappath, $vwordpath) = @ARGV;
if (!$siftmappath || !$vwordpath) {
    warn "Usage: $0 siftmap vwords\n";
    exit 1;
}
open my $siftmapfh, $siftmappath or die "cannot open $siftmappath";
open my $vwordfh, $vwordpath or die "cannot open $vwordpath";

my %siftid2vword;
while (my $line = <$vwordfh>) {
    chomp $line;
    next if !$line;
    my @data = split /\t/, $line;
    my $siftid = shift @data;
    my %vword;
    for (my $i = 0; $i < scalar(@data) && $i < MAX_VWORD*2; $i += 2) {
        $vword{$data[$i]} = $data[$i+1];
    }
    if (!defined $siftid || !%vword) {
        warn "sift id or vword not defiend!\n";
        next;
    }
    $siftid2vword{$siftid} = \%vword;
}

while (my $line = <$siftmapfh>) {
    chomp $line;
    next if !$line;
    my @data = split /\t/, $line;
    my $filename = shift @data;
    my %histogram;
    foreach my $siftid (@data) {
        if ($siftid2vword{$siftid}) {
            foreach my $key (keys %{ $siftid2vword{$siftid} }) {
                $histogram{$key} += $siftid2vword{$siftid}{$key};
            }
        }
        else {
            warn "vword not found: $siftid\n";
            next;
        }
    }
    print $filename;
    foreach my $vword (sort { $histogram{$b} <=> $histogram{$a} } keys %histogram) {
        print "\t$vword\t$histogram{$vword}";
    }
    print "\n";
}
