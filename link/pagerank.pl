#!/usr/bin/perl
#
# PageRank
#
# Usage:
#  pagerank.pl -l linkdb -o pagerankdb
#

use strict;
use warnings;
use Getopt::Long;
use TokyoCabinet;

use constant {
    TMP_DBM   => '/tmp/pagerank_tmp_%d.hdb',
    PROB_JUMP => 0.15
};

sub usage_exit {
    print <<'USAGE';
Usage:
 pagerank.pl -l linkdb -o pagerankdb
   -l, --link linkdb         link structure DBM
   -o, --output pagerankdb   PageRank DBM
USAGE
    exit 1;
}

sub _initialize {
    my ($linkdb, $prdb) = @_;
    my $rnum = $linkdb->rnum();
    my $initval = 1 / sqrt($rnum);
    $linkdb->iterinit();
    while (my $id = $linkdb->iternext()) {
        $prdb->put($id, $initval);
    }
}

sub _fill_zero {
    my ($prevdb, $nextdb) = @_;
    $prevdb->iterinit();
    while (my $id = $prevdb->iternext()) {
        $nextdb->put($id, 0);
    }
}

sub _make_temp_dbm {
    my $count = shift;
    my $prdb = TokyoCabinet::HDB->new();
    if (!$prdb->open(sprintf(TMP_DBM, $count),
        $prdb->OWRITER | $prdb->OCREAT | $prdb->OTRUNC)) {
        die 'cannot open output dbm';
    }
    return $prdb;
}

sub _update_pagerank {
    my ($linkdb, $prevdb, $nextdb) = @_;

=comment
    _fill_zero($prevdb, $nextdb);
    my $jumppoint = 1 / $linkdb->rnum() * PROB_JUMP;
=cut
    $prevdb->iterinit();
    while (my $previd = $prevdb->iternext()) {
        my $prevval = $prevdb->get($previd);
        next if !$prevval;
        my $linkval = $linkdb->get($previd);
        next if !$linkval;
        my %link = unpack('(Nn)*', $linkval);
        next if !%link;
        my $point = $prevval / scalar(keys %link) * (1 - PROB_JUMP);
        foreach my $linkid (keys %link) {
            my $nextval = $nextdb->get($linkid) || 0;
            $nextdb->put($linkid, $nextval + $point);
        }
=comment
        # random jump
        $nextdb->iterinit();
        while (my $nextid = $nextdb->iternext()) {
            my $nextval = $nextdb->get($nextid) || 0;
            $nextdb->put($nextid, $nextval + $jumppoint);
        }
=cut
    }
}

sub pagerank {
    my ($linkdb, $num_loop) = @_;
    my $count = 0;

    print "Start PageRank Algorithm\n";

    print "Initialize PageRank values\n";
    my $prevdb = _make_temp_dbm($count++);
    _initialize($linkdb, $prevdb);

    my $nextdb;
    # 本当は更新前後で比較しないとダメ
    for (my $i = 0; $i < $num_loop; $i++) {
        print "PageRank Loop No.$i\n";

        print " Update PageRank values\n";
        $nextdb = _make_temp_dbm($count++);
        _update_pagerank($linkdb, $prevdb, $nextdb);
        my $prevpath = $prevdb->path();
        $prevdb->close();
        unlink $prevpath;
        $prevdb = $nextdb;
    }
    return if !$nextdb;

    my $nextpath = $nextdb->path();
    if (!$nextdb->close()) {
        die 'DBM error: '.$nextdb->errmsg($nextdb->ecode);
    }
    return $nextpath;
}

sub main {
    my ($opt_link, $opt_output);
    GetOptions(
        'link=s'   => \$opt_link,
        'output=s' => \$opt_output,
    );
    if (!$opt_link || !$opt_output) {
        usage_exit();
    }

    my $linkdb = TokyoCabinet::HDB->new();
    if (!$linkdb->open($opt_link, $linkdb->OREADER)) {
        die "cannot open $opt_link";
    }

    my $dbpath = pagerank($linkdb, 2);
    if ($dbpath) {
        rename $dbpath, $opt_output;
        print "PageRank Result was saved: $opt_output\n";
    }
    else {
        print "Calculating PageRank values failed...orz\n";
    }

    if (!$linkdb->close()) {
        die 'DBM error: '.$linkdb->errmsg($linkdb->ecode);
    }
}

main();

__END__

