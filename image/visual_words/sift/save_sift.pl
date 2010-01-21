#!/usr/bin/perl

use strict;
use warnings;

use constant {
    SIFT_OPTION => " -x -o %s %s",
    SIFT_TMP    => 'sift.tmp',
};

my ($siftcmd, $siftmappath) = @ARGV;
if (!$siftcmd || !$siftmappath) {
    warn "Usage: $0 siftcmd siftmap < imgfiles\n";
    exit(1);
}
open my $siftmapfh, ">$siftmappath" or die "cannot open $siftmappath";

my $count_file = 0;
my $count_siftid = 0;
while (my $fn = <STDIN>) {
    chomp $fn;
    next if !$fn;
    # get SIFT features
    system $siftcmd . sprintf(SIFT_OPTION, SIFT_TMP, $fn);
    
    open my $fh, SIFT_TMP or next;
    my @siftids;
    my @vector;
    while (my $line = <$fh>) {
        chomp $line;
        next if !$line;
        if ($line =~ /^\s/) {
            map { push @vector, $_ if $_ ne '' } split /\s/, $line;
        }
        elsif (@vector) {
            if (scalar @vector != 128) {
                warn "dim of vector != 128\n";
                next;
            }
            print $count_siftid;
            for (my $i = 0; $i < 128; $i++) {
                printf "\t%d\t%d", $i, $vector[$i];
            }
            print "\n";
            push @siftids, $count_siftid;
            $count_siftid++;
            @vector = ();
        }
    }
    close $fh;
    printf $siftmapfh "%s\t%s\n", $fn, join("\t", @siftids);
    system 'rm '.SIFT_TMP;
    printf STDERR "(%d) %s\n", ++$count_file, $fn;
}
