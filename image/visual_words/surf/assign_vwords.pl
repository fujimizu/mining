#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;

my ($countpath, $vwordpath) = @ARGV;
if (!$countpath || !$vwordpath) {
    warn "Usage: $0 surf_count vwords\n";
    exit 1;
}
open my $countfh, $countpath or die "cannot open $countpath";
open my $vwordfh, $vwordpath or die "cannot open $vwordpath";

my %id2vword;
while (my $line = <$vwordfh>) {
    chomp $line;
    next if !$line;
    my @data = split /\t/, $line;
    my $id = shift @data;
    my $vword = shift @data;
    if (!defined $vword) {
        warn "vword not defiend!: $id\n";
        next;
    }
    $id2vword{$id} = $vword;
}

my $descid = 0;
while (my $line = <$countfh>) {
    chomp $line;
    next if !$line;
    my ($filename, $count) = split /\t/, $line;
    my %histogram;
    for (my $i = 0; $i < $count; $i++) {
        if ($id2vword{$descid}) {
            $histogram{$id2vword{$descid}}++;
        }
        else {
            warn "vword not found: $filename $descid\n";
        }
        $descid++;
    }
    print $filename;
    foreach my $vword (sort { $a <=> $b } keys %histogram) {
        print "\t$vword\t$histogram{$vword}";
    }
    print "\n";
}
