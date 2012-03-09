#!/usr/bin/perl -w

use URI::Escape;


$usage = "Usage:  <pages/redirects file base> <output directory>\n";

$fbase = shift(@ARGV) || die $usage;
$odir = shift(@ARGV) || die $usage;

system("mkdir $odir") && die "Can't make $odir\n";

%bucket_to_titles = ();
%uc_titles = ();
$biggest_bucket = undef;
#$read = 0;

open(F, "$fbase.pages") || die "Couldn't open $fbase.pages\n";
while ($l = <F>) {
  #$read++;
  @p = split(/\|/, $l);
  $t = $p[1];
  consider_title($t);
}
close(F);

open(F, "$fbase.redirects") || die "Couldn't open $fbase.redirects\n";
while ($l = <F>) {
  $l =~ m/(.+)>/;
  consider_title($1);
}
close(F);

printf(STDERR "Number of buckets: %d\n", scalar(keys(%bucket_to_titles)));
printf(STDERR "Biggest bucket: $biggest_bucket (%d)\n", scalar(@{$bucket_to_titles{$biggest_bucket}}));

@buckets = sort(keys(%bucket_to_titles));
for ($i = 0; $i < @buckets; $i++) {
  mkdir("$odir/$buckets[$i]");
  @titles = sort(@{$bucket_to_titles{$buckets[$i]}});
  $n = 0;
  $j = 0;
  TITLES: while ($n < $#titles) {
    mkdir("$odir/$buckets[$i]/$j");
    for ($k = 0; $k < 1024; $k++) {
      $f = "$odir/$buckets[$i]/$j/$k.html";
      print STDERR "$f\n";
      $str = "<div style='padding:3px;border-top:1px solid #999999;border-bottom:1px solid #999999'>";
      for ($l = 0; $l < @buckets; $l++) {
        $str .= "<a href='/dir/$buckets[$l]/0/0.html'>$buckets[$l]</a>";
        if ($l < $#buckets) {
          $str .= " | ";
        }
      }
      $str .= "</div>\n";
      $str .= "<div style='margin-top:10px'><h2>$buckets[$i]";
      if ($j > 0 || $k > 0) {
        $str .= " (cont.)";
     }
      $str .= "</h2></div>\n";
      $str .= "<div style='margin:15px'>\n";
      for ($l = 0; $l < 50; $l++) {
        $str .= "<a href='/rc/topic/" . uri_escape($titles[$n]) . "'>$titles[$n]</a><br/>\n";
        $n++;
        if ($n == @titles) { last; }
      }
      $str .= "</div>\n<div style='margin-top:20px'>";
      if ($j > 0 || $k > 0) {
        $str .= "<a href='$prev'>Previous</a>";
        if ($n < $#titles) {
          $str .= ' | ';
        }
      }
      if ($n < $#titles) {
        $str .= "<a href='/dir/$buckets[$i]/";
        if ($k == 1023) {
          $str .= ($j+1) . "/0.html";
        }
        else {
          $str .= "$j/" . ($k+1) . ".html";
        }
        $str .= "'>Next</a>";
      }
      $str .= "</div>\n";
      system("cp index_page_start $f") && die "Couldn't cp to $f\n";
      open(O, ">>$f") || die "Couldn't open $f for writing\n";
      print O $str;
      close(O);
      if ($n == @titles) {
        last TITLES;
      }
      system("cat index_page_end >> $f") && die "Couldn't >> to $f\n";
      $prev = "/dir/$buckets[$i]/$j/$k.html";
    }
    $j++;
  }
}

#########

sub consider_title {
  my ($t) = @_;

  my $pt = uc($t);
  $pt =~ s/[^ A-Z0-9]//g;
  if (!defined($uc_titles{uc($pt)})) {
    $uc_titles{$pt} = 1;
    my $b = bucket($t);
    if (!defined($bucket_to_titles{$b})) {
      @{$bucket_to_titles{$b}} = ($t);
    }
    else {
      push(@{$bucket_to_titles{$b}}, $t); 
    }
    if (!defined($biggest_bucket) || @{$bucket_to_titles{$b}} > @{$bucket_to_titles{$biggest_bucket}}) {
      $biggest_bucket = $b;
    }
  }
}


sub bucket {
  my ($t) = @_;

  my $t_len = length($t);
  my $b = '';

  if (0) {
    for (my $i = 0; $i < 3; $i++) {
      my $c = undef;
      if ($t_len > $i) {
        $c = uc(substr($t, $i, 1));
        if ($c !~ m/[A-Z0-9]/) {
          $c = undef;
        }
      }
      if (!defined($c)) {
        $c = '_';
      }
      $b .= $c;
    }
  }
  else {
    $b = uc(substr($t, 0, 1));
    if ($b !~ m/[A-Z0-9]/) {
      $b = '_';
    }
  }

  return $b;
}
