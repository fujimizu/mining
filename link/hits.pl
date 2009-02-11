#!/usr/bin/perl 
#
# HITS Algorithm
#
# Usage:
#  hits.pl -i linkdb -d outdir
#   => outdir/auth.hdb, outdir/hub.hdb
#

use strict;
use warnings;
use Getopt::Long;
use TokyoCabinet;

use constant {
    DBM_AUTHORITY => 'auth.hdb',
    DBM_HUB       => 'hub.hdb',
};

sub usage_exit {
    print <<'USAGE';
Usage:
 hits.pl -l linkdb -d outdir
   -l, --link linkdb        link structure DBM
   -d, --directory outdir   output directory
USAGE
    exit 1;
}

sub _update_authority {
    my ($linkdb, $authdb, $hubdb) = @_;
    _fill_zero($authdb);

    $linkdb->iterinit();
    while (my $id = $linkdb->iternext()) {
        my $hubval = $hubdb->get($id);
        next if !$hubval;
        my $val = $linkdb->get($id);
        next if !$val;
        my %link = unpack('(Nn)*', $val);
        foreach my $linkid (keys %link) {
            my $authval = $authdb->get($linkid) || 0;
            # 本当はadddoubleを使いたい
            $authdb->put($linkid, $authval + $hubval);
        }
    }
}

sub _update_hub {
    my ($linkdb, $authdb, $hubdb) = @_;
    $linkdb->iterinit();
    while (my $id = $linkdb->iternext()) {
        my $val = $linkdb->get($id);
        next if !$val;
        my %link = unpack('(Nn)*', $val);
        my $sum = 0;
        foreach my $linkid (keys %link) {
            my $authval = $authdb->get($linkid);
            next if !$authval;
            $sum += $authval;
        }
        $hubdb->put($id, $sum);
    }
}

sub _initialize {
    my ($linkdb, $pointdb) = @_;
    my $rnum = $linkdb->rnum();
    my $initval = 1 / sqrt($rnum);
    $linkdb->iterinit();
    while (my $id = $linkdb->iternext()) {
        $pointdb->put($id, $initval);
    }
}

sub _fill_zero {
    my $pointdb = shift;
    $pointdb->iterinit();
    while (my $id = $pointdb->iternext()) {
        $pointdb->put($id, 0);
    }
}

sub _normalize {
    my $pointdb = shift;
    my $sum = 0;
    $pointdb->iterinit();
    while (my $id = $pointdb->iternext()) {
        my $val = $pointdb->get($id);
        $sum += $val * $val;
    }
    $sum = sqrt($sum);
    $pointdb->iterinit();
    while (my $id = $pointdb->iternext()) {
        my $val = $pointdb->get($id);
        $pointdb->put($id, $val / $sum);
    }
}

sub hits {
    my ($linkdb, $authdb, $hubdb, $num_loop) = @_;
    return if !$linkdb || !$authdb || !$hubdb || !$num_loop;

    print "Start HITS Algorithm\n";

    print "Initialize authority and hub values\n";
    _initialize($linkdb, $authdb);
    _initialize($linkdb, $hubdb);

    for (my $i = 0; $i < $num_loop; $i++) {
        print "HITS Loop No.$i\n";

        print " Update authority values\n";
        _update_authority($linkdb, $authdb, $hubdb);

        print " Normalize authority values\n";
        _normalize($authdb);

        print " Update hub values\n";
        _update_hub($linkdb, $authdb, $hubdb);
        
        print " Normalize hub values\n";
        _normalize($hubdb);
    }
    print "Finish HITS Algorithm\n";
}

sub main {
    my ($opt_link, $opt_dir);
    GetOptions(
        'link=s'      => \$opt_link,
        'directory=s' => \$opt_dir,
    );
    if (!$opt_link || !$opt_dir) {
        usage_exit();
    }

    my $linkdb = TokyoCabinet::HDB->new();
    if (!$linkdb->open($opt_link, $linkdb->OREADER)) {
        die "cannot open $opt_link";
    }
    # Authority
    my $dbpath = "$opt_dir/".DBM_AUTHORITY;
    my $authdb = TokyoCabinet::HDB->new();
    if (!$authdb->open($dbpath,
        $authdb->OWRITER | $authdb->OCREAT | $authdb->OTRUNC)) {
        die 'cannot open authority dbm';
    }
    # Hub
    $dbpath = "$opt_dir/".DBM_HUB;
    my $hubdb = TokyoCabinet::HDB->new();
    if (!$hubdb->open($dbpath,
        $hubdb->OWRITER | $hubdb->OCREAT | $hubdb->OTRUNC)) {
        die 'cannot open hub dbm';
    }

    hits($linkdb, $authdb, $hubdb, 2);

    if (!$linkdb->close()) {
        die 'DBM error: '.$linkdb->errmsg($linkdb->ecode);
    }
    if (!$authdb->close()) {
        die 'DBM error: '.$authdb->errmsg($authdb->ecode);
    }
    if (!$hubdb->close()) {
        die 'DBM error: '.$hubdb->errmsg($hubdb->ecode);
    }
}

main();

__END__

