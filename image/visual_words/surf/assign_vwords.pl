#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;

my ($idmappath, $vwordpath) = @ARGV;
if (!$idmappath || !$vwordpath) {
    warn "Usage: $0 idmap vwords\n";
    exit 1;
}
open my $idmapfh, $idmappath or die "cannot open $idmappath";
open my $vwordfh, $vwordpath or die "cannot open $vwordpath";

my %id2vword;
while (my $line = <$vwordfh>) {
    chomp $line;
    next if !$line;
    my @data = split /\t/, $line;
    my $id = shift @data;
    my $vword = shift @data;
    if (!defined $id || !defined $vword) {
        warn "original id or vword not defiend!\n";
        next;
    }
    $id2vword{$id} = $vword;
}

while (my $line = <$idmapfh>) {
    chomp $line;
    next if !$line;
    my @data = split /\t/, $line;
    my $filename = shift @data;
    my %histogram;
    foreach my $id (@data) {
        if ($id2vword{$id}) {
            $histogram{$id2vword{$id}}++;
        }
        else {
            warn "vword not found: $id\n";
            next;
        }
    }
    print $filename;
    foreach my $vword (sort { $a <=> $b } keys %histogram) {
        print "\t$vword\t$histogram{$vword}";
    }
    print "\n";
}
