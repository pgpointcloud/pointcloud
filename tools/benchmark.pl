#!/usr/bin/env perl

# Example usage:
# ./benchmark.pl raw20h64_1000_p3_uncompressed_main | psql -etXA db1

use strict;

@ARGV || die "Usage: $0 <tabname> [<tabname> ...]\n";

foreach $a (@ARGV) {
  my $tn="${a}";
  print "select count(*) from \"${tn}\";\n";
  print "select pg_size_pretty(pg_total_relation_size('${tn}'));\n";
  # full decompression
  print "explain analyze select PC_FilterEquals(p,'z',PC_PatchMax(p,'z')) from \"${tn}\";\n";
  # header only
  print "explain analyze select PC_PatchAvg(p,'z') from \"${tn}\";\n";
}
