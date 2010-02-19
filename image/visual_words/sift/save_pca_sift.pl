#!/usr/bin/perl
#
# Requirements:
#  http://www.cs.cmu.edu/~yke/pcasift/
#    pcasift-0.01nd.tar.gz
#    mod_lowe_demoV2.tar.gz
#

use strict;
use warnings;

use constant PATH => '/home/fujisawa/src/pcasift-0.91nd';
use constant {
    CMD_CONVERT  => "convert %s tmp.pgm",
    CMD_KEYPOINT => PATH.'/mod_lowe_demo/keypoints < tmp.pgm > tmp.key',
    CMD_PCA      => PATH.'/recalckeys '.PATH.'/gpcavects.txt tmp.pgm tmp.key tmp.pkey',
};

my ($siftmappath, $outpath) = @ARGV;
if (!$siftmappath || !$outpath) {
    warn "Usage: $0 siftmap outpath < imgfiles\n";
    exit(1);
}
open my $siftmapfh, ">$siftmappath" or die "cannot open $siftmappath";
open my $outfh, ">$outpath" or die "cannot open $outpath";

my $count_file = 0;
my $count_siftid = 0;
while (my $fn = <STDIN>) {
    chomp $fn;
    next if !$fn;
    system sprintf(CMD_CONVERT, $fn);
    system CMD_KEYPOINT;
    system CMD_PCA;
    
    open my $fh, 'tmp.pkey' or next;
    my @siftids;
    my @vector;
    while (my $line = <$fh>) {
        chomp $line;
        next if !$line;
        if ($line =~ /^\s/) {
            map { push @vector, $_ if $_ ne '' } split /\s/, $line;
        }
        elsif (@vector) {
            if (scalar @vector != 36) {
                warn "dim of vector != 36\n";
                next;
            }
            print $outfh $count_siftid;
            for (my $i = 0; $i < 36; $i++) {
                printf $outfh "\t%d\t%d", $i, $vector[$i];
            }
            print $outfh "\n";
            push @siftids, $count_siftid;
            $count_siftid++;
            @vector = ();
        }
    }
    close $fh;
    printf $siftmapfh "%s\t%s\n", $fn, join("\t", @siftids);
    system 'rm tmp.pgm tmp.key tmp.pkey';
    printf STDERR "(%d) %s\n", ++$count_file, $fn;
}
