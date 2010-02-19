#!/usr/bin/perl
#
# Classification test using broomie
#

use strict;
use warnings;
use List::Util qw(shuffle);

use constant {
    TRAIN_RATIO => 0.9,
};

my %image;

my $dir = shift @ARGV || '.';
my $method = shift @ARGV || 'bayes';

my $train_file = $dir.'/train';
my $test_file = $dir.'/test';
my $model_dir = $dir.'/model';

open my $trainfh, '>'.$train_file or die;
open my $testfh, '>'.$test_file or die;

# read BOF data
while (my $line = <STDIN>) {
    chomp $line;
    my @data = split /\t/, $line;
    my $filename = shift @data;
    my %vector = @data;
    my $category = (split /\//, $filename)[-2];
    $image{$category}{$filename} = \%vector;
}

foreach my $category (keys %image) {
    my @filenames = keys %{ $image{$category} };
    #@filenames = shuffle @filenames;
    my $num_train = scalar(@filenames) * TRAIN_RATIO;
    #my @trains = @filenames;
    #my @tests = @filenames;
    my @trains = @filenames[0 .. $num_train-1];
    my @tests = @filenames[$num_train .. scalar(@filenames)-1];
    
    foreach my $filename (@trains) {
        print $trainfh $category;
        foreach my $key (sort { $a <=> $b } keys %{ $image{$category}{$filename} }) {
            print $trainfh "\t$key\t$image{$category}{$filename}{$key}"
        }
        print $trainfh "\n";
    }
    foreach my $filename (@tests) {
        print $testfh $category;
        foreach my $key (sort { $a <=> $b } keys %{ $image{$category}{$filename} }) {
            print $testfh "\t$key\t$image{$category}{$filename}{$key}"
        }
        print $testfh "\n";
    }

}

close $trainfh;
close $testfh;

my $cmd;
# train
$cmd = sprintf "brmtrain -m %s -t %s -c %s %s", $model_dir, $train_file, $method, join(' ', keys %image);
print $cmd, "\n";
system $cmd;

# classify
$cmd = sprintf "brmclassify accuracy -m %s -t %s", $model_dir, $test_file;
print $cmd, "\n";
system $cmd;
