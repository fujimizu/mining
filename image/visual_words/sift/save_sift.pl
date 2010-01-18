#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;

use constant {
    SIFT_CMD => "/home/fujisawa/Programs/cpp/opencv/sift/sift/bin/siftfeat -x -o %s %s",
};

my ($fn, $outdir) = @ARGV;
if (!$fn || !$outdir) {
    warn "Usage: $0 txtfile outdir\n";
    exit(1);
}
open my $fh, $fn or die "cannot open $fn";

my @files;
while (my $line = <$fh>) {
    chomp $line;
    push @files, $line;
}

for (my $i = 0; $i < scalar @files; $i++) {
    my $fn = $files[$i];
    my $dir = dirname($fn);
    my $name = basename($fn);
    my $outfn = sprintf "%s/%s_%s.sift",
        $outdir, (split /\//, $dir)[-1], (split /\./, $name)[0];
    my $cmd = sprintf SIFT_CMD, $outfn, $fn;
    printf "(%d/%d) %s\n", $i+1, scalar @files, $outfn;
    system $cmd;
}
while (my $line = <$fh>) {
    chomp $line;
}
