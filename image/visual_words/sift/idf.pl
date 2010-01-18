#!/usr/bin/perl

use strict;
use warnings;

my ($path, $max) = @ARGV;
if (!$path) {
    warn "Usage: $0 txtfile [max]\n";
    exit 1;
}
open my $fh, $path or die "cannot open $path";

my %df;
my %vectors;
my $ndoc;
while (my $line = <$fh>) {
    chomp $line;
    my @data = split /\t/, $line;
    my $id = shift @data;
    my %vec = @data;
    map { $df{$_}++ } keys %vec;
    $vectors{$id} = \%vec;
    $ndoc++;
}

foreach my $id (keys %vectors) {
    my %vec = %{ $vectors{$id} };
    map { $vec{$_} *= log($ndoc / $df{$_}) } keys %vec;
    print $id;
    my @keys = sort { $vec{$b} <=> $vec{$a} } keys %vec;
    @keys = @keys[0 .. $max-1] if $max && scalar(@keys) > $max;
    foreach my $key (@keys) {
        printf "\t%s\t%.4f", $key, $vec{$key};
    }
    print "\n";
}
