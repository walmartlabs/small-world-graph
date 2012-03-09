#!/usr/bin/perl -w


$usage = "Usage:  <input xml>\n";

$f = shift(@ARGV) || die $usage;

open(F, $f) || die "Couldn't open $f\n";

sub invalid_title
{
  local($title) = ($_[0]);
  if ($title =~ m/[:#]/ or $title =~ m/disambiguation/g or $title =~ m/^\d{4}$/g or $title =~ m/^\d{4} in/g or $title =~ m/^((January)|(February)|(March)|(April)|(May)|(June)|(July)|(August)|(September)|(October)|(November)|(December))\b/g) {
    #print STDERR "skipped $title\n";
    return 1;
  }
}

$onpage = 0;
$is_redirect = 1;
while ($l = <F>) {
  if ($l =~ m/<title>(.+?)<\/title>/) {
    $newtitle = $1;
    if (!$is_redirect) {
      foreach $b (keys(%seenlinks)) {
        print "$title|$b\n";
      }
    }
    $title = $newtitle;
    if (invalid_title($title) == 1) {
      $title = undef;
      $is_redirect = 1;
      next;
    }
    %seenlinks = ();
    $is_redirect = 0;
#    if ($title =~ m/[:#]/) { 
#      $title = undef; 
#      next; 
#    }
    $onpage++;
    if ($onpage % 32384 == 0) { print STDERR "on page $onpage\n"; }
  }
  if (defined($title)) {
    if ($l =~ m/#REDIRECT\s+\[\[([^:#\]\|]+)[\|\]]/g) {
      if (invalid_title($1) != 1) {
        print "$title>$1\n";
        $is_redirect = 1;
      }
    } else {
      while ($l =~ m/\[\[([^:#\]\|]+)[\|\]]/g) {
        $b = $1;
        if (invalid_title($b) != 1) {
          if (!defined($seenlinks{$b})) {
            $seenlinks{$b} = 1;
            #print "$title|$b\n";
          }
        }
      }
    }
  }
}

close(F);
