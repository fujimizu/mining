#!/usr/bin/perl
#
# count term frequency, document frequency
#
# Usage:
#  count_words.pl -d txtdir -o outdir
#

use strict;
use warnings;
use Getopt::Long;
use Text::MeCab;
use TokyoCabinet;

use constant {
    DBM_TF => 'tf.hdb',
    DBM_DF => 'df.hdb',
};

sub usage_exit {
    print <<'USAGE';
Usage:
 count_words.pl -t txtdir -o outdir
   -t, --textdir dir     directory including wikipedia texts
   -o, --output outdir   output directory
USAGE
    exit 1;
}

sub get_words {
    my ($mecab, $words, $text) = @_;
    for (my $node = $mecab->parse($text); $node; $node = $node->next) {
        my @features = split /,/, $node->feature;
        #if ($features[0] =~ /^(名詞|動詞|形容詞|形容動詞)$/) {
        if ($features[0] eq '名詞') {
            $words->{$node->surface}++;
        }
    }
}

sub add_df {
    my ($dbm, $words) = @_;
    return if !$dbm || !$words;

    foreach my $word (keys %{ $words }) {
        $dbm->addint($word, 1);
    }
}

sub add_tf {
    my ($dbm, $title, $words) = @_;
    return if !$dbm || !$title || !$words;

    my @keys = sort { $words->{$b} <=> $words->{$a} } keys %{ $words };
    my $val = join "\t", map { $_.":".$words->{$_} } @keys;
    $dbm->putasync($title, $val);  
}

sub main {
    my ($opt_txt, $opt_out);
    GetOptions(
        'textdir=s' => \$opt_txt,
        'output=s' => \$opt_out,
    );
    if (!$opt_txt || !$opt_out) {
        usage_exit();
    }

    my $tfdb = TokyoCabinet::HDB->new();
    my $dbpath = $opt_out.'/'.DBM_TF;
    if (!$tfdb->open($dbpath,
        $tfdb->OWRITER | $tfdb->OCREAT | $tfdb->OTRUNC)) {
        die 'cannot open TF dbm';
    }
    my $dfdb = TokyoCabinet::HDB->new();
    $dbpath = $opt_out.'/'.DBM_DF;
    if (!$dfdb->open($dbpath,
        $dfdb->OWRITER | $dfdb->OCREAT | $dfdb->OTRUNC)) {
        die 'cannot open DF dbm';
    }

    my $mecab = Text::MeCab->new() or die 'mecab error';

    opendir my $dh, $opt_txt or die "cannot open $opt_txt";
    my @files = grep /^jawiki-latest-pages-articles\.xml-\d+\.txt$/, readdir($dh);
    closedir $dh;
    my $cnt = 1;
    foreach my $file (@files) {
        my $path = "$opt_txt/$file";
        printf "(%d/%d) %s\n", $cnt++, scalar(@files), $path;
        open my $fh, $path or warn "cannot open $path";
        
        my ($title, $body, %words);
        while (my $line = <$fh>) {
            chomp $line;
            if ($line =~ /^\[\[(.+)\]\]$/) {
                my $newtitle = $1;
                if ($title && $body) {
                    my @sentences = split /。/, $body;
                    foreach my $sentence (@sentences) {
                        get_words($mecab, \%words, $sentence);
                    }
                    add_tf($tfdb, $title, \%words);
                    add_df($dfdb, \%words);
                }
                $title = $newtitle;
                $body = '';
                %words = ();
            }
            else {
                $body .= $line;
            }
        }
    }
}

main();

__END__

