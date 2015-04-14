#!/usr/bin/env perl

# Example usage:
# ./benchmark.pl raw20h64_1000_p3_uncompressed_main | psql -etXA db1

use strict;

@ARGV || die "Usage: $0 <tabname>[:<colname>] ...\n";

foreach $a (@ARGV) {
  my $tn="${a}";
  my $col="p";
  if ( $tn =~ /(.*):(.*)/ ) {
    $col = $2;
    $tn = $1;
  }
  print "select count(*) from \"${tn}\";\n";
  print "select min(pc_numpoints(\"${col}\")) from \"${tn}\";\n";
  print "select pg_size_pretty(pg_total_relation_size('${tn}'));\n";
  # full decompression
  print "explain analyze select PC_FilterEquals(\"${col}\",'z',PC_PatchMax(\"${col}\",'z')) from \"${tn}\";\n";
  # header only
  print "explain analyze select PC_PatchAvg(\"${col}\",'z') from \"${tn}\";\n";
}
