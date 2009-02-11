#!/usr/bin/perl 
#
# k-means++
# (algorithm: http://www.stanford.edu/~darthur/)
#

use strict;
use warnings;
use Data::Dumper;
use List::Util qw(shuffle);

$| = 1;

sub choose_random_centers {
    my ($vectors, $num_center) = @_;
    my @ids = keys %{ $vectors };
    @ids = shuffle @ids;
    my @centers = map { $vectors->{$_} } @ids[0 .. $num_center-1];;
    return \@centers;
}

sub choose_smart_centers {
    my ($vectors, $num_center) = @_;
    my $cur_potential = 0;
    my @centers;

    # choose one random center
    my $vector = (shuffle values %{ $vectors })[0];
    push @centers, $vector;
    my %closest_dist;
    foreach my $id (keys %{ $vectors }) {
        $closest_dist{$id} = get_distance($vectors->{$id}, $vector);
        $cur_potential += $closest_dist{$id};
    }

    # choose each center
    for (my $i = 1; $i < $num_center; $i++) {
        my $randval = rand() * $cur_potential;
        my $center_id;
        foreach my $id (keys %{ $vectors }) {
            $center_id = $id;
            last if $randval <= $closest_dist{$id};
            $randval -= $closest_dist{$id};
        }
        my $new_potential = 0;
        foreach my $id (keys %{ $vectors }) {
            my $dist = get_distance($vectors->{$id}, $vectors->{$center_id});
            $closest_dist{$id} = $dist if $dist < $closest_dist{$id};
            $new_potential += $closest_dist{$center_id};
        }
        push @centers, $vectors->{$center_id};
        $cur_potential = $new_potential;
    }
    return \@centers;
}

# Format: id\tval1,val2,val3,...
sub read_vectors {
    my $fh = shift;
    my %vectors;
    while (my $line = <$fh>) {
        chomp $line;
        next if !$line;
        
        my ($id, $vecstr) = split /\t/, $line;
        if (!$id || !$vecstr) {
            warn "Illegal format: $line\n";
        }
        my @vector = split /,/, $vecstr;
        $vectors{$id} = \@vector;
    }
    return \%vectors;
}

sub get_distance {
    my ($vec1, $vec2) = @_;
    my $size = scalar @{ $vec1 };
    my $dist = 0;
    for (my $i = 0; $i < $size; $i++) {
        $dist += ($vec1->[$i] - $vec2->[$i]) * ($vec1->[$i] - $vec2->[$i]);
    }
    return $dist;
}

sub assign_clusters {
    my ($vectors, $centers) = @_;
    my %assign;
    foreach my $id (keys %{ $vectors }) {
        my $num_center = scalar @{ $centers };
        my $min_dist = -1;
        my $min_index;
        for (my $i = 0; $i < $num_center; $i++) {
            my $dist = get_distance($vectors->{$id}, $centers->[$i]);
            if ($min_dist < 0 || $min_dist > $dist) {
                $min_dist = $dist;
                $min_index = $i;
            }
        }
        $assign{$id} = $min_index;
    }
    return \%assign;
}

sub move_centers {
    my ($vectors, $assign, $centers) = @_;
    my @clusters;
    foreach my $id (keys %{ $assign }) {
        my $idx = $assign->{$id};
        $clusters[$idx] = [] if !defined $clusters[$idx];
        push @{ $clusters[$idx] }, $id;
    }
    for (my $i = 0; $i < scalar @{ $centers }; $i++) {
        my $cluster = $clusters[$i];
        next if !$cluster;
        my @new_center;
        foreach my $id (@{ $cluster }) {
            my $vector = $vectors->{$id};
            for (my $j = 0; $j < scalar @{ $vector }; $j++) {
                $new_center[$j] += $vector->[$j] / scalar(@{ $cluster });
            }
        }
        $centers->[$i] = \@new_center if @new_center;
    }
}

sub kmeans {
    my ($vectors, $centers, $num_iter) = @_;
    my $assign = assign_clusters($vectors, $centers);

    for (my $i = 0; $i < $num_iter; $i++) {
        print " .. k-means loop No.$i\n";
        move_centers($vectors, $assign, $centers);
        #$centers = move_centers($vectors, $assign, scalar @{ $centers });
        my $new_assign = assign_clusters($vectors, $centers);
        my $is_changed = 0;
        foreach my $id (keys %{ $assign }) {
            if ($assign->{$id} != $new_assign->{$id}) {
                $is_changed = 1;
                last;
            }
        }
        last if !$is_changed;
        $assign = $new_assign;
    }
    return $assign;
}

sub main {
    my ($path, $num_center) = @ARGV;
    if (!$path || !$num_center) {
        print "Usage: kmeanspp.pl inputfile num_center\n";
        exit 1;
    }
    open my $fh, "<$path" or die "cannot open: $path";
    my $vectors = read_vectors($fh);
    my $num_vectors = scalar keys %{ $vectors };
    if ($num_center <= 0 || $num_center > $num_vectors) {
        die 'number of centers must be >0 and <=number_of_vectors';
    }

    print "Choose initial centers\n";
    #my $centers = choose_random_centers($vectors, $num_center);
    my $centers = choose_smart_centers($vectors, $num_center);
    print "Do k-means clustering\n";
    my $assign = kmeans($vectors, $centers, 100);
    print "assign: ". Dumper($assign);
}

main()
