#!/usr/bin/perl 
#
# Wikipedia のキーワード感のリンク構造を
# TokyoCabinetのHashDBに保存する
#
# Usage:
#  1) キーワード <-> IDの対応DBMの作成(keyword2id.hdb, id2keyword.hdb)
#   % bunzip2 -c jawiki-latest-pages-articles.xml.bz2 | \
#     perl wp_keyword_link.pl -t -d output/
#
#  2) キーワードリンクDBMの作成(keywordlink.hdb)
#   % bunzip2 -c jawiki-latest-pages-articles.xml.bz2 | \
#     perl wp_keyword_link.pl -l -d output/
#
# 参考ページ: 
#  [を] Wikipediaのキーワードリンクを使って関連語データを作ってみた
#  http://chalow.net/2007-06-09-3.html

use strict;
use warnings;
use Getopt::Long;
use Text::MeCab;
use TokyoCabinet;

use constant {
    DBM_KEYWORD2ID   => 'keyword2id.hdb',
    DBM_ID2KEYWORD   => 'id2keyword.hdb',
    DBM_KEYWORD_LINK => 'keywordlink.hdb',
};

sub usage_exit {
    print <<'USAGE';
Usage: 
 1) Make id <-> keyword table
  $ wp_keyword_link.pl -d directory -t < wikipedia_data
     -t, --table                 id <-> keyword table mode
     -d, --directory directory   output dir

 2) Make keyword-link DBM
  $ wp_keyword_link.pl -d directory -l < wikipedia_data
     -l, --link                  keyword link mode
     -d, --directory directory   output dir
USAGE
    exit 1;
}

sub word_ok {
    my $word = shift;
    return unless $word;
    return if $word =~ m{[:\|\n\t\[\]]};;
    return if $word =~ m{^[\#\s]};;
    $word =~ s/\s*\(.+?\)$//;
    return if $word =~ /一覧$/;
#    return if $word =~ /\d+月\d+日$/;
    return if $word =~ /^\d{4}年$/;
    return $word;
}

sub make_keyword_id_table {
    my $dir = shift;
    return if !$dir;

    # open dbms
    my $dbpath = "$dir/".DBM_KEYWORD2ID;
    my $kw2iddb = TokyoCabinet::HDB->new;
    if (!$kw2iddb->open($dbpath,
        $kw2iddb->OWRITER | $kw2iddb->OCREAT | $kw2iddb->OTRUNC)) {
        die "cannot open $dbpath";
    }
    $dbpath = "$dir/".DBM_ID2KEYWORD;
    my $id2kwdb = TokyoCabinet::HDB->new;
    if (!$id2kwdb->open($dbpath,
        $id2kwdb->OWRITER | $id2kwdb->OCREAT | $id2kwdb->OTRUNC)) {
        die "cannot open $dbpath";
    }

    my $flag = 0;
    my $content;
    while (my $line = <STDIN>) {
        chomp $line;
        if (!$flag && $line =~ m{<page>}) {
            $flag = 1
        }
        if ($flag) {
            $content .= $line;
        }
        if ($flag && $line =~ m{</page>}) {
            my ($title) = $content =~ m{<title>([^<]+)<};
            my ($id) = $content =~ m{<id>(\d+)<};
            $title = word_ok($title);
            if ($title) {
                print "$id\t$title\n";
                $kw2iddb->putasync($title, $id);
                #$kw2iddb->putasync($title, pack('N', $id));
                $id2kwdb->putasync($id, $title);
            }
            $flag = 0;
            $content = '';
        }
    }

    # close dbms
    if (!$kw2iddb->close()) {
        die 'DBM error: '.$kw2iddb->errmsg($kw2iddb->ecode);
    }
    if (!$id2kwdb->close()) {
        die 'DBM error: '.$id2kwdb->errmsg($id2kwdb->ecode);
    }
}

sub make_keyword_link {
    my $dir = shift;
    return if !$dir;

    my $dbpath = "$dir/".DBM_KEYWORD2ID;
    my $kw2iddb = TokyoCabinet::HDB->new;
    if (!$kw2iddb->open($dbpath, $kw2iddb->OREADER)) {
        die "cannot open $dbpath";
    }
    $dbpath = "$dir/".DBM_KEYWORD_LINK;
    my $kwlinkdb = TokyoCabinet::HDB->new;
    if (!$kwlinkdb->open($dbpath,
        $kwlinkdb->OWRITER | $kwlinkdb->OCREAT | $kwlinkdb->OTRUNC)) {
        die "cannot open $dbpath";
    }

    my $flag = 0;
    my $content;
    while (my $line = <STDIN>) {
        if (!$flag && $line =~ m{<page>}) {
            $flag = 1
        }
        if ($flag) {
            $content .= $line;
        }
        if ($flag && $line =~ m{</page>}) {
            my ($title) = $content =~ m{<title>([^<]+)<};
            my ($id) = $content =~ m{<id>(\d+)<};
            $title = word_ok($title);
            if ($title) {
                my %count;
                while ($content =~ m{\[\[(.+?)\]\]}g) {
                    my $word = word_ok($1);
                    next if !$word;
                    next if $word eq $title;
                    $count{$word}++;
                }
                next if !%count;
                my @words = sort { $count{$b} <=> $count{$a} } keys %count;
                my $buf;
                foreach my $word (@words) {
                    my $wid = $kw2iddb->get($word);
                    next if !$wid;
                    $buf .= pack('Nn', $wid, $count{$word});
                }
                $kwlinkdb->putasync($id, $buf);
                print "$id\t$title\n";
            }
            $flag = 0;
            $content = '';
        }
    }
   
    if (!$kwlinkdb->close()) {
        die 'DBM error: '.$kwlinkdb->errmsg($kwlinkdb->ecode);
    }
    if (!$kw2iddb->close()) {
        die 'DBM error: '.$kw2iddb->errmsg($kw2iddb->ecode);
    }
}

sub main {
    my ($opt_dir, $opt_table, $opt_link);
    GetOptions(
        'directory=s' => \$opt_dir,
        'table'       => \$opt_table,
        'link'        => \$opt_link,
    );
    if (!$opt_dir) {
        usage_exit();
    }
    elsif ($opt_table) {
        make_keyword_id_table($opt_dir);
    }
    elsif ($opt_link) {
        make_keyword_link($opt_dir);
    }
    else {
        usage_exit();
    }
}

main();

__END__

