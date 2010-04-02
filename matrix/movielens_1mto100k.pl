#!/usr/bin/perl

use strict;
use warnings;

my $cur_userid = 0;
my %rating;
while (my $line = <STDIN>) {
    chomp $line;
    my ($userid, $itemid, $rate, $timestamp) = split /::/, $line;
    if ($cur_userid != $userid) {
        foreach my $itemid (sort { $a <=> $b } keys %rating) {
            printf "%d\t%d\t%s\n", $cur_userid, $itemid, $rating{$itemid};
        }
        %rating = ();
        $cur_userid = $userid;
    }
    $rating{$itemid} = "$rate\t$timestamp";
}

foreach my $itemid (sort { $a <=> $b } keys %rating) {
    printf "%d\t%d\t%s\n", $cur_userid, $itemid, $rating{$itemid};
}
