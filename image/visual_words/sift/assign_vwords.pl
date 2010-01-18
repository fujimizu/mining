#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;

my $basedir = '/var/www/bkrd/data/caltech101/101_ObjectCategories/';

sub convert_filepath {
    my ($path, $basedir) = @_;
    my $base_path = basename($path);
    $base_path =~ s/\.sift/.jpg/;
    $base_path =~ s/_image/\/image/;
    return $basedir =~ /\/$/ ? 
        "$basedir$base_path" : "$basedir/$base_path";
}

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
    my $vword = shift @data;
    if (!defined $siftid || !defined $vword) {
        warn "sift id or vword not defiend!\n";
        next;
    }
    $siftid2vword{$siftid} = $vword;
}

while (my $line = <$siftmapfh>) {
    chomp $line;
    next if !$line;
    my @data = split /\t/, $line;
    my $filename = shift @data;
    my %histogram;
    foreach my $siftid (@data) {
        if ($siftid2vword{$siftid}) {
            $histogram{$siftid2vword{$siftid}}++;
        }
        else {
            warn "vword not found: $siftid\n";
            next;
        }
    }
    print convert_filepath($filename, '/var/www/bkrd/data/caltech101/101_ObjectCategories/');
    foreach my $vword (sort { $a <=> $b } keys %histogram) {
        print "\t$vword\t$histogram{$vword}";
    }
    print "\n";
}
