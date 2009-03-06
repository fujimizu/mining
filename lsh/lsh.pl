#!/usr/bin/perl
#
# Locality Sensitive Hash
#
# Usage:
#  % lsh.pl input_dbm output_txt
#    input_dbm  ... Tokyo Cabinet hash database
#                   [format] word => key1\tval1\tkey2\tval2..
#    output_txt ... text file
#                   [format] word1\tword2\tpoint
#

use strict;
use warnings;
use List::Util qw(shuffle);
use TokyoCabinet;

use constant {
    NUM_RANDOM_KEY => 10,
    NUM_SHUFFLE    => 5,
    NUM_NEIGHBOR   => 5,
    MIN_COSINE     => 0.7,
};

sub read_vector {
    my ($vecdb, $key) = @_;
    return if !$vecdb || !$key;

    my $val = $vecdb->get($key);
    return if !$val;
    my %vector = split /\t/, $val;
    return \%vector;
}

sub select_keys_randomly {
    my ($vecdb, $num) = @_;
    return if !$vecdb || !$num || $num < 1;

    my $rnum = $vecdb->rnum();
    my @index = (0 .. $rnum-1);
    @index = shuffle @index;
    @index = @index[0 .. $num-1] if $rnum > $num;
    my %index = map { $_ => 1 } @index;

    my @keys;
    my $cnt = 0;
    $vecdb->iterinit();
    while (my $key = $vecdb->iternext()) {
        push @keys, $key if $index{$cnt};
        $cnt++;
    }
    return \@keys;
}

sub lsh {
    my ($vecdb, $outfh) = @_;
    return if !$vecdb;

    print "Select random vectors\n";
    my $keys = select_keys_randomly($vecdb, NUM_RANDOM_KEY);
    return if !$keys;
    my @vectors;
    foreach my $key (sort { $a cmp $b } @{ $keys }) {
        my $vector = read_vector($vecdb, $key);
        push @vectors, $vector if $vector;
    }

    print "Calculate bits\n";
    my %bits;
    $vecdb->iterinit();
    while (my $key = $vecdb->iternext()) {
        my $vector = read_vector($vecdb, $key);
        next if !$vector;
        my $bit;
        my @bit;
        for my $v (@vectors) {
            push @bit, cosine($vector, $v) > 0 ? 1 : 0;
        }
        $bits{$key} = \@bit;
    }

    my %check;
    for (my $i = 0; $i < NUM_SHUFFLE; $i++) {
        print "Loop No.$i\n";

        print " Shuffle bits\n";
        my @index = shuffle (0 .. NUM_RANDOM_KEY-1);
        my %bits_shuffled;
        foreach my $key (keys %bits) {
            my $bitstr;   
            foreach my $i (@index) {
                $bitstr .= $bits{$key}[$i];
            }
            $bits_shuffled{$key} = $bitstr;
        }
        my @keys = sort { $bits_shuffled{$a} cmp $bits_shuffled{$b} }
            keys %bits_shuffled;

        print " Get similar keys\n";
        my $size = scalar @keys;
        for (my $j = 0; $j < $size; $j++) {
            my $vec = read_vector($vecdb, $keys[$j]);
            my $start = $j - NUM_NEIGHBOR > 0 ? $j - NUM_NEIGHBOR : 0;
            my $end = $j + NUM_NEIGHBOR < $size ? $j + NUM_NEIGHBOR : $size-1;
            foreach my $idx ($start .. $end) {
                next if $idx == $j;
                my $key = $idx > $j ? $keys[$j]."\t".$keys[$idx] : $keys[$idx]."\t".$keys[$j];
                next if $check{$key};
                $check{$key} = 1;
                my $vec_neighbor = read_vector($vecdb, $keys[$idx]);
                my $cos = cosine($vec, $vec_neighbor);
                print $outfh "$key\t$cos\n" if $cos > MIN_COSINE;
            }
        }
    }
}

sub cosine {
    my ($v1, $v2) = @_;
    my $len1 = vector_length($v1);
    my $len2 = vector_length($v2);
    return 0 if !$len1 || !$len2;
    return inner_product($v1, $v2) / ($len1 * $len2);
}

sub inner_product {
    my ($v1, $v2) = @_;
    return 0 if !$v1 || !$v2;
    my @keys = scalar(keys %{ $v1 }) < scalar(keys %{ $v2 }) ?
        keys %{ $v1 } : keys %{ $v2 };
    my $prod = 0;
    foreach my $key (@keys) {
        $prod += $v1->{$key} * $v2->{$key} if $v1->{$key} && $v2->{$key};
    }
    return $prod;
}

sub vector_length {
    my $v1 = shift;
    return 0 if !$v1;
    my $len = 0;
    foreach my $x (values %{ $v1 }) {
        $len += $x * $x;
    }
    return sqrt($len);
}

sub usage_exit {
    print "Usage: $0 input_dbm output_txt\n";
    exit 1;
}

sub main {
    my ($vecdbpath, $outpath) = @ARGV;
    usage_exit() if !$vecdbpath || !$outpath;
    
    my $vecdb = TokyoCabinet::HDB->new();
    if (!$vecdb->open($vecdbpath, $vecdb->OREADER)) {
        my $ecode = $vecdb->ecode();
        die 'open error: ' . $vecdb->errmsg($ecode);
    }
    open my $outfh, ">$outpath" or die "cannot open $outpath";

    lsh($vecdb, $outfh);
}

main();
