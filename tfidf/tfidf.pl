#!/usr/bin/perl
#
# TFIDF
#
# Usage:
#  tfidf.pl -t tfdb -d dfdb -o tfidfdb
#

use strict;
use warnings;
use Encode qw(encode decode);
use Getopt::Long;
use Text::MeCab;
use TokyoCabinet;

use constant {
    DBM_TFIDF => 'tfidf.hdb',
    MAX_WORD  => 50,
};

sub usage_exit {
    print <<'USAGE';
Usage:
 tfidf.pl -t tfdb -d dfdb -o tfidfdb
   -t, --tfdb dbm      Term frequency(TF) dbm
   -d, --dfdb dbm      Document frequency(DF) dbm
   -o, --output dbm    TFIDF dbm     
USAGE
    exit 1;
}

sub calc_tfidf {
    my ($tf, $df, $total) = @_;
    my $idf = log($total / ($df + 1));
    return $tf * $idf;
}

sub main {
    my ($opt_tf, $opt_df, $opt_tfidf);
    GetOptions(
        'tfdb=s'   => \$opt_tf,
        'dfdb=s'   => \$opt_df,
        'output=s' => \$opt_tfidf,
    );
    if (!$opt_tf || !$opt_df || !$opt_tfidf) {
        usage_exit();
    }

    my $tfdb = TokyoCabinet::HDB->new();
    if (!$tfdb->open($opt_tf, $tfdb->OREADER)) {
        die 'cannot open TF dbm';
    }
    my $dfdb = TokyoCabinet::HDB->new();
    if (!$dfdb->open($opt_df, $dfdb->OREADER)) {
        die 'cannot open DF dbm';
    }
    my $tfidfdb = TokyoCabinet::HDB->new();
    if (!$tfidfdb->open($opt_tfidf,
        $tfidfdb->OWRITER | $tfidfdb->OCREAT | $tfidfdb->OTRUNC)) {
        die 'cannot open output dbm';
    }

    my $total = $tfdb->rnum();
    $tfdb->iterinit();
    while (my $title = $tfdb->iternext()) {
        my $tfval = $tfdb->get($title);
        next if !$tfval;
        my %tfidf;
        foreach my $item (split /\t/, $tfval) {
            my $itemdec = decode 'utf8', $item;
            my ($word, $tf) = split /:/, $itemdec;
            $word = encode 'utf8', $word;
            my $dfval = $dfdb->get($word);
            my $df = $dfval ? unpack 'n', $dfval : 0;
            $tfidf{$word} = calc_tfidf($tf, $df, $total);
        }
        my @words = sort { $tfidf{$b} <=> $tfidf{$a} } keys %tfidf;
        @words = @words[0 .. MAX_WORD-1] if scalar(@words) > MAX_WORD;
        next if !@words;
        my $tfidfval = join "\t", map { $_."\t".int($tfidf{$_} * 100) } @words;
        #print "$title\t$tfidfval\n";
        $tfidfdb->putasync($title, $tfidfval);
    }
}

main();

__END__

