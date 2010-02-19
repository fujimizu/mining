#!/usr/bin/perl

use strict;
use warnings;

my %count;
while (my $line = <STDIN>) {
    chomp $line;
    my @data = split /\t/, $line;
    my $filename = shift @data;
    my %vector = @data;
    my $category = (split /\//, $filename)[-2];
    foreach my $key (keys %vector) {
        $count{$category}{$key}++;
        $count{'all'}{$key}++;
    }
}

foreach my $category (keys %count) {
    my $count_one = 0;
    print $category, "\n";
    foreach my $key (keys %{ $count{$category} }) {
        $count_one++ if $count{$category}{$key} == 1;
#        print "\t$key\t$count{$category}{$key}\n";
    }
    my $num_all = scalar keys %{ $count{$category} };
    printf "%d / %d = %f\n", $count_one, $num_all, $count_one / $num_all;
}
